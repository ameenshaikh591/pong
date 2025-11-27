#include <SPI.h>
#include <NimBLEDevice.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#if CONFIG_FREERTOS_UNICORE
  #define TASK_RUNNING_CORE 0
#else
  #define TASK_RUNNING_CORE 1
#endif

volatile char user_input;

void SetupBluetooth();
class WriteCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    String value = pCharacteristic->getValue();
    user_input = value[0];
  }
};

#define SCLK_PIN 4
#define MISO_PIN_UNUSED 5
#define MOSI_PIN 6
#define CS_PIN 7
#define SPI_CLK_HZ 100000

void TaskReadUserInput(void* pvParameters);
TaskHandle_t read_user_input_task_h;
void TaskGameLogic(void* pvParameters);
TaskHandle_t game_logic_task_h;
void TaskSPI(void* pvParameters);
TaskHandle_t spi_task_h;


SemaphoreHandle_t xMutex;


typedef struct {
  uint16_t ball_x;
  uint16_t ball_y;
  uint16_t paddle_l_y;
  uint16_t paddle_r_y;
  uint8_t score_l;
  uint8_t score_r;
} game_state_t;

game_state_t game_state;

typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER
} game_mode_t;

game_mode_t current_mode = GAME_STATE_MENU;
volatile bool start_requested = false;

typedef struct {
  game_state_t* game_state;
  volatile char* user_input;
  game_mode_t* game_mode;
  volatile bool* start_req;   
  SemaphoreHandle_t mutex;
} user_input_task_params_t;

typedef struct {
  game_state_t* game_state;
  game_mode_t* game_mode;
  volatile bool* start_req;
  SemaphoreHandle_t mutex;
} game_logic_task_params_t;

typedef struct {
  game_state_t* game_state;
  SemaphoreHandle_t mutex;
  SPIClass* spi_driver;
} spi_task_params_t;

user_input_task_params_t user_input_task_params;
game_logic_task_params_t game_logic_task_params;
spi_task_params_t spi_task_params;

#define MEDIUM_PRIORITY 2

SPIClass* vspi;

void StartingObjectPosition(game_logic_task_params_t* p);

void setup() {
    Serial.begin(115200);
    SetupBluetooth();

    SetupSPI();

    xMutex = xSemaphoreCreateMutex();

    // Read user input task params
    user_input_task_params.game_state = &game_state;
    user_input_task_params.user_input = &user_input;
    user_input_task_params.game_mode = &current_mode;    
    user_input_task_params.start_req = &start_requested; 
    user_input_task_params.mutex = xMutex;

    // Game logic task params
    game_logic_task_params.game_state = &game_state;
    game_logic_task_params.game_mode = &current_mode;
    game_logic_task_params.start_req = &start_requested;
    game_logic_task_params.mutex = xMutex;

    // SPI task params
    spi_task_params.game_state = &game_state;
    spi_task_params.mutex = xMutex;
    spi_task_params.spi_driver = vspi;

    xTaskCreate(
      TaskReadUserInput,
      "Read User Input",
      2048,
      (void*)&user_input_task_params,
      MEDIUM_PRIORITY,
      &read_user_input_task_h
    );

    xTaskCreate(
      TaskGameLogic,
      "Game Logic",
      2048,
      (void*)&game_logic_task_params,
      MEDIUM_PRIORITY,
      &game_logic_task_h
    );

    xTaskCreate(
      TaskSPI,
      "SPI",
      2048,
      (void*)&spi_task_params,
      MEDIUM_PRIORITY,
      &spi_task_h
    );

    Serial.printf("Basic Multi Threading Arduino Example\n");
}

void loop() {

}

#define PADDLE_LENGTH 60
#define MIN_PADDLE_Y 0
#define MAX_PADDLE_Y 420
#define PADDLE_DISPLACEMENT 5
#define READ_USER_INPUT_TASK_DELAY_MS 14

void TaskReadUserInput(void *pvParameters) {
    user_input_task_params_t* p = (user_input_task_params_t*)pvParameters;

    while (true) {

      char current_input = *(p->user_input);

      xSemaphoreTake(p->mutex, portMAX_DELAY);

      // Menu Mode
      if (*(p->game_mode) == GAME_STATE_MENU) {
          if (current_input == 'e') {
              *(p->start_req) = true; 
          }
          xSemaphoreGive(p->mutex);
          vTaskDelay(pdMS_TO_TICKS(READ_USER_INPUT_TASK_DELAY_MS));
          continue;
      }

      switch (current_input) {
        case 'w':
          p->game_state->paddle_l_y = max(MIN_PADDLE_Y, p->game_state->paddle_l_y - PADDLE_DISPLACEMENT);
          break;
        case 's':
          p->game_state->paddle_l_y = min(MAX_PADDLE_Y, p->game_state->paddle_l_y + PADDLE_DISPLACEMENT);
          break;
        case 'u':
          p->game_state->paddle_r_y = max(MIN_PADDLE_Y, p->game_state->paddle_r_y - PADDLE_DISPLACEMENT);
          break;
        case 'd':
          p->game_state->paddle_r_y = min(MAX_PADDLE_Y, p->game_state->paddle_r_y + PADDLE_DISPLACEMENT);
          break;
        default:
          break;
      }

      xSemaphoreGive(p->mutex);

      Serial.println(current_input);

      vTaskDelay(pdMS_TO_TICKS(READ_USER_INPUT_TASK_DELAY_MS));
    }
}

#define GAME_LOGIC_TASK_DELAY_MS 16

void TaskGameLogic(void* pvParameters) {
  game_logic_task_params_t* p = (game_logic_task_params_t*)pvParameters;

  while (true) {

    xSemaphoreTake(p->mutex, portMAX_DELAY);

    switch (*(p->game_mode)) {

      case GAME_STATE_MENU:
        StartingObjectPosition(p);

        // check if user pressed start
        if (*(p->start_req)) {
            *(p->start_req) = false;
            *(p->game_mode) = GAME_STATE_PLAYING;
        }

        break;
      case GAME_STATE_PLAYING:
        break;
      case GAME_STATE_GAME_OVER:
        break;
      default:
        break;
    }

    xSemaphoreGive(p->mutex);
    vTaskDelay(pdMS_TO_TICKS(GAME_LOGIC_TASK_DELAY_MS));
  }
}

#define BALL_STARTING_X 320
#define BALL_STARTING_Y 240
#define PADDLE_STARTING_Y 200

void StartingObjectPosition(game_logic_task_params_t* p) {
  p->game_state->ball_x = BALL_STARTING_X;
  p->game_state->ball_y = BALL_STARTING_Y;
  p->game_state->paddle_l_y = PADDLE_STARTING_Y;
  p->game_state->paddle_r_y = PADDLE_STARTING_Y;
}

void SetupBluetooth() {
  NimBLEDevice::init("Ameen ESP32");

  NimBLEServer* pServer = NimBLEDevice::createServer();
  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  NimBLECharacteristic* pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );

  pCharacteristic->setCallbacks(new WriteCallback());

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();
}

void SetupSPI() {
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, LOW);

  vspi = new SPIClass(SPI);
  vspi->begin(SCLK_PIN, MISO_PIN_UNUSED, MOSI_PIN, CS_PIN);
  delay(250); // Some time to set-up SPI
}

#define SPI_TASK_DELAY_MS 16
#define BYTES_PER_TRANSACTION 10
#define BALL_LOWER_X 0
#define BALL_UPPER_X 1
#define BALL_LOWER_Y 2
#define BALL_UPPER_Y 3
#define PADDLE_L_LOWER_Y 4
#define PADDLE_L_UPPER_Y 5
#define PADDLE_R_LOWER_Y 6
#define PADDLE_R_UPPER_Y 7
#define SCORE_L 8
#define SCORE_R 9

void TaskSPI(void* pvParameters) {
  spi_task_params_t* p = (spi_task_params_t*)pvParameters;
  game_state_t local_copy;

  while (true) {

    xSemaphoreTake(p->mutex, portMAX_DELAY);
    local_copy = *(p->game_state);
    xSemaphoreGive(p->mutex);

    SPITransaction(p->spi_driver, &local_copy);

    vTaskDelay(pdMS_TO_TICKS(SPI_TASK_DELAY_MS));
  }
}


void SPITransaction(SPIClass* spi_driver, game_state_t* g) {
  spi_driver->beginTransaction(SPISettings(SPI_CLK_HZ, MSBFIRST, SPI_MODE0));

  digitalWrite(CS_PIN, HIGH); // ASSERT CS (active high)
  delayMicroseconds(1); // allow FPGA to detect CS high

  spi_driver->transfer((uint8_t)(g->ball_x & 0xFF));
  spi_driver->transfer((uint8_t)(g->ball_x >> 8));

  spi_driver->transfer((uint8_t)(g->ball_y & 0xFF));
  spi_driver->transfer((uint8_t)(g->ball_y >> 8));

  spi_driver->transfer((uint8_t)(g->paddle_l_y & 0xFF));
  spi_driver->transfer((uint8_t)(g->paddle_l_y >> 8));

  spi_driver->transfer((uint8_t)(g->paddle_r_y & 0xFF));
  spi_driver->transfer((uint8_t)(g->paddle_r_y >> 8));

  spi_driver->transfer(g->score_l);
  spi_driver->transfer(g->score_r);

  delayMicroseconds(1);
  digitalWrite(CS_PIN, LOW);

  spi_driver->endTransaction();
}






