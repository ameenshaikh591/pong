/*
Author: Ameen Shaikh

Custom data types used primarily in "pong_esp32.ino".
*/

#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
class SPIClass;

typedef struct {
  uint16_t ball_x;
  uint16_t ball_y;
  uint16_t paddle_l_y;
  uint16_t paddle_r_y;
  uint8_t score_l;
  uint8_t score_r;
} game_state_t;

typedef struct {
    float x_vel;
    float y_vel;
} ball_velocity_t;

typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_SHOW_SCORE,
    GAME_STATE_GAME_OVER
} game_mode_t;

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

typedef enum {
    ONGOING,
    L_WINNER,
    R_WINNER
} match_status_t;