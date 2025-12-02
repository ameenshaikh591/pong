/*
Author: Ameen Shaikh

Driver file.

Handles the bulk of game logic by implementing the FreeRTOS tasks.
*/

#include "spi_driver.h"
#include "bluetooth.h"
#include "game_types.h"
#include <stdlib.h>
#include <math.h>
#include <esp_system.h> 
#include <stdint.h>

#if CONFIG_FREERTOS_UNICORE
  #define TASK_RUNNING_CORE 0
#else
  #define TASK_RUNNING_CORE 1
#endif

void TaskReadUserInput(void* pvParameters);
TaskHandle_t read_user_input_task_h;
void TaskGameLogic(void* pvParameters);
TaskHandle_t game_logic_task_h;
void TaskSPI(void* pvParameters);
TaskHandle_t spi_task_h;

SemaphoreHandle_t xMutex;

game_state_t game_state;

game_mode_t current_mode = GAME_STATE_MENU;
volatile bool start_requested = false;

user_input_task_params_t user_input_task_params;
game_logic_task_params_t game_logic_task_params;
spi_task_params_t spi_task_params;

#define MEDIUM_PRIORITY 2

void StartingObjectPosition(game_state_t* gs);

void setup() {
  Serial.begin(115200);
  srand((unsigned)esp_random());

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
}

void loop() {
}

// Game constants

#define PADDLE_LENGTH 60
#define MIN_PADDLE_Y 0
#define MAX_PADDLE_Y 420
#define PADDLE_DISPLACEMENT 5
#define READ_USER_INPUT_TASK_DELAY_MS 14

#define GAME_LOGIC_TASK_DELAY_MS   16
#define BALL_SPEED 5.0f

#define BALL_SIZE 10
#define BALL_MAX_Y 470
#define BALL_MIN_Y 0

#define LEFT_BORDER_X 0
#define RIGHT_BORDER_X 640

#define PADDLE_WIDTH 10
#define PADDLE_LEFT_X 0
#define PADDLE_RIGHT_X (RIGHT_BORDER_X - PADDLE_WIDTH)

#define SPIN_FACTOR 1.5f

#define SCORE_DISPLAY_FLAG 0x80
#define SCORE_DISPLAY_TIME_MS 4000
#define ROUND_DELAY_TIME_MS 1000

#define BALL_STARTING_X 320
#define BALL_STARTING_Y 240
#define PADDLE_STARTING_Y 200


/*
Read User Input Task

Samples the "user_input" variable, which tells what key the user is pressing

Updates the paddle positions based on the pressed key
*/
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
          p->game_state->paddle_l_y =
            max(MIN_PADDLE_Y, p->game_state->paddle_l_y - PADDLE_DISPLACEMENT);
          break;
        case 's':
          p->game_state->paddle_l_y =
            min(MAX_PADDLE_Y, p->game_state->paddle_l_y + PADDLE_DISPLACEMENT);
          break;
        case 'u':
          p->game_state->paddle_r_y =
            max(MIN_PADDLE_Y, p->game_state->paddle_r_y - PADDLE_DISPLACEMENT);
          break;
        case 'd':
          p->game_state->paddle_r_y =
            min(MAX_PADDLE_Y, p->game_state->paddle_r_y + PADDLE_DISPLACEMENT);
          break;
        default:
          break;
      }

      xSemaphoreGive(p->mutex);

      vTaskDelay(pdMS_TO_TICKS(READ_USER_INPUT_TASK_DELAY_MS));
    }
}

#define M_PI_135 (M_PI_2 + M_PI_4)

float RandomBallAngle() {
  bool start_right = (rand() % 2) == 0;

  if (start_right) {
    return -M_PI_4 + (rand() / (float)RAND_MAX) * M_PI_2;
  } else {
    return M_PI_135 + (rand() / (float)RAND_MAX) * M_PI_2;
  }
}

void RandomBallVelocity(ball_velocity_t* p_bv) {
  float ball_angle_radians = RandomBallAngle();
  p_bv->x_vel = BALL_SPEED * cosf(ball_angle_radians);
  p_bv->y_vel = BALL_SPEED * sinf(ball_angle_radians);
}

void UpdateBallPosition(ball_velocity_t* p_bv, game_state_t* p_gs) {
  int16_t new_x = (int16_t)p_gs->ball_x + (int16_t)p_bv->x_vel;
  int16_t new_y = (int16_t)p_gs->ball_y + (int16_t)p_bv->y_vel;

  if (new_y < BALL_MIN_Y) {
    new_y = BALL_MIN_Y;
    p_bv->y_vel = -p_bv->y_vel;
  }
  else if (new_y + BALL_SIZE > BALL_MAX_Y) {
    new_y = BALL_MAX_Y - BALL_SIZE;
    p_bv->y_vel = -p_bv->y_vel;
  }

  if (new_x < LEFT_BORDER_X) {
    new_x = LEFT_BORDER_X;
  } else if (new_x + BALL_SIZE > RIGHT_BORDER_X) {
    new_x = RIGHT_BORDER_X - BALL_SIZE;
  }

  p_gs->ball_x = (uint16_t)new_x;
  p_gs->ball_y = (uint16_t)new_y;
}

match_status_t LeftRightCollision(const game_state_t* p_gs) {
  if (p_gs->ball_x <= LEFT_BORDER_X) {
    return R_WINNER;
  } else if (p_gs->ball_x + BALL_SIZE >= RIGHT_BORDER_X) {
    return L_WINNER;
  } else {
    return ONGOING;
  }
}

#define MAX_BOUNCE_ANGLE (M_PI / 3.0f)

void PaddleCollision(ball_velocity_t* p_bv, const game_state_t* p_gs) {

  // Left Paddle
  if (p_gs->ball_x <= (PADDLE_LEFT_X + PADDLE_WIDTH)) {

    bool y_overlap =
      (p_gs->ball_y + BALL_SIZE) >= p_gs->paddle_l_y &&
      p_gs->ball_y <= (p_gs->paddle_l_y + PADDLE_LENGTH);

    if (y_overlap) {
      float ball_center   = p_gs->ball_y + BALL_SIZE / 2.0f;
      float paddle_center = p_gs->paddle_l_y + PADDLE_LENGTH / 2.0f;

      float relative = (ball_center - paddle_center) / (PADDLE_LENGTH / 2.0f);
      relative = constrain(relative, -1.0f, 1.0f);

      float bounce_angle = relative * MAX_BOUNCE_ANGLE;

      p_bv->x_vel = BALL_SPEED * cosf(bounce_angle);
      p_bv->y_vel = BALL_SPEED * sinf(bounce_angle);
      p_bv->x_vel = fabsf(p_bv->x_vel);
      return;
    }
  }

  // Right Paddle
  if ((p_gs->ball_x + BALL_SIZE) >= PADDLE_RIGHT_X) {

    bool y_overlap =
      (p_gs->ball_y + BALL_SIZE) >= p_gs->paddle_r_y &&
      p_gs->ball_y <= (p_gs->paddle_r_y + PADDLE_LENGTH);

    if (y_overlap) {
      float ball_center   = p_gs->ball_y + BALL_SIZE / 2.0f;
      float paddle_center = p_gs->paddle_r_y + PADDLE_LENGTH / 2.0f;

      float relative = (ball_center - paddle_center) / (PADDLE_LENGTH / 2.0f);
      relative = constrain(relative, -1.0f, 1.0f);

      float bounce_angle = relative * MAX_BOUNCE_ANGLE;

      p_bv->x_vel = BALL_SPEED * cosf(bounce_angle);
      p_bv->y_vel = BALL_SPEED * sinf(bounce_angle);
      p_bv->x_vel = -fabsf(p_bv->x_vel);
      return;
    }
  }
}


/*
Game Logic Task

1. Determines the current game state
2. Updates the ball's position
3. Handles the ball's collision with the borders or paddles
4. Keep's track of each user's score
5. Handles the event of a player winning a round

These steps are repeated at 60 Hz
*/
void TaskGameLogic(void* pvParameters) {
  game_logic_task_params_t* p = (game_logic_task_params_t*)pvParameters;

  static ball_velocity_t ball_velocity = {0.0f, 0.0f};
  static bool ongoing_round = false;

  // Score display state
  static uint32_t score_timer_ms = 0;
  static uint32_t round_delay_ms = 0;
  static bool score_display_active = false;
  static bool end_game = false;

  while (true) {

    game_state_t gs_local;
    game_mode_t mode_local;
    bool start_req_local;

    xSemaphoreTake(p->mutex, portMAX_DELAY);
    gs_local = *(p->game_state);
    mode_local = *(p->game_mode);
    start_req_local = *(p->start_req);
    xSemaphoreGive(p->mutex);

    switch (mode_local) {

      case GAME_STATE_MENU:
        StartingObjectPosition(&gs_local);
        ongoing_round = false;
        end_game = false;
        score_display_active = false;
        score_timer_ms = 0;
        round_delay_ms = 0;

        if (start_req_local) {
            start_req_local = false;
            mode_local = GAME_STATE_PLAYING;
        }
        break;

      case GAME_STATE_PLAYING: {

        uint8_t sl = gs_local.score_l & 0x0F;
        uint8_t sr = gs_local.score_r & 0x0F;

        if (!end_game && (sl >= 9 || sr >= 9)) {

          if (sl >= 9) {
            gs_local.score_l = 9 | SCORE_DISPLAY_FLAG;
          }
          if (sr >= 9) {
            gs_local.score_r = 9 | SCORE_DISPLAY_FLAG;
          }

          score_display_active = true;
          score_timer_ms = SCORE_DISPLAY_TIME_MS;
          round_delay_ms = 0;
          ongoing_round = false;
          end_game = true;

          //mode_local = GAME_STATE_MENU;

          StartingObjectPosition(&gs_local);

          break;
        }

        // Score display
        if (score_display_active) {
          if (score_timer_ms > GAME_LOGIC_TASK_DELAY_MS) {
            score_timer_ms -= GAME_LOGIC_TASK_DELAY_MS;
          } else if (score_timer_ms > 0) {
            score_timer_ms = 0;
            gs_local.score_l &= (uint8_t)(~SCORE_DISPLAY_FLAG);
            gs_local.score_r &= (uint8_t)(~SCORE_DISPLAY_FLAG);
            round_delay_ms = ROUND_DELAY_TIME_MS;
          } else if (round_delay_ms > GAME_LOGIC_TASK_DELAY_MS) {
            round_delay_ms -= GAME_LOGIC_TASK_DELAY_MS;
          } else if (round_delay_ms > 0) {
            round_delay_ms = 0;
            score_display_active = false;
            if (end_game) {
              gs_local.score_l = 0;
              gs_local.score_r = 0;
              mode_local = GAME_STATE_MENU;
            }
          }

          if (score_display_active || round_delay_ms > 0) {
            break;
          }
        }

        // No active round — start a new one
        if (!ongoing_round && !score_display_active && round_delay_ms == 0) {
          if (gs_local.ball_x == 0 && gs_local.ball_y == 0) {
            StartingObjectPosition(&gs_local);
          }
          RandomBallVelocity(&ball_velocity);
          ongoing_round = true;
        }

        // If round is ongoing
        if (ongoing_round) {
          UpdateBallPosition(&ball_velocity, &gs_local);

          PaddleCollision(&ball_velocity, &gs_local);

          match_status_t match_status = LeftRightCollision(&gs_local);
          if (match_status == L_WINNER || match_status == R_WINNER) {

            ongoing_round = false;

            if (match_status == L_WINNER) {
              uint8_t s = gs_local.score_l & 0x0F;
              if (s < 9) {
                s++;
              }
              gs_local.score_l = s | SCORE_DISPLAY_FLAG;
            } else {
              uint8_t s = gs_local.score_r & 0x0F;
              if (s < 9) {
                s++;
              }
              gs_local.score_r = s | SCORE_DISPLAY_FLAG;
            }

            gs_local.ball_x = BALL_STARTING_X;
            gs_local.ball_y = BALL_STARTING_Y;
            ball_velocity.x_vel = 0.0f;
            ball_velocity.y_vel = 0.0f;

            score_display_active = true;
            score_timer_ms = SCORE_DISPLAY_TIME_MS;
            round_delay_ms = 0;
          }
        }

        break;
      }

      default:
        break;
    }

    // ---- Write back under mutex ----
    xSemaphoreTake(p->mutex, portMAX_DELAY);
    *(p->game_mode) = mode_local;
    *(p->start_req) = start_req_local;
    *(p->game_state) = gs_local;
    xSemaphoreGive(p->mutex);

    vTaskDelay(pdMS_TO_TICKS(GAME_LOGIC_TASK_DELAY_MS));
  }
}

void StartingObjectPosition(game_state_t* gs) {
  gs->ball_x = BALL_STARTING_X;
  gs->ball_y = BALL_STARTING_Y;
  gs->paddle_l_y = PADDLE_STARTING_Y;
  gs->paddle_r_y = PADDLE_STARTING_Y;
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


/*
SPI Task

Sends the game state to the FPGA via SPI at 60 Hz
*/
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








