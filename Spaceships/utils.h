#include <stdint.h>
#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <queue.h>
#include <semphr.h>
#include "src/board/board.h"
#include "point.h"

typedef struct point point;


#define FLAG 0x7E
#define ESCCHAR 0x1B
#define ACK 1
#define NACK 2

#define POLYNOMIAL 263

static uint16_t frame_buf[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t seq_num;

TimerHandle_t xTimerControllersAndDraw;
TimerHandle_t xTimerCollisions;

SemaphoreHandle_t xTimerSemaphoreJoystick;
SemaphoreHandle_t xTimerSemaphoreDrawShips;
SemaphoreHandle_t xTimerSemaphoreDrawBullets;
SemaphoreHandle_t xTimerSemaphoreBullets;
SemaphoreHandle_t xTimerSemaphoreCollisions;
SemaphoreHandle_t xTimerSemaphorePCConn;
SemaphoreHandle_t xTimerSemaphoreMoveShip;


SemaphoreHandle_t xFramebufMutex;
uint8_t results[2];


int power(uint16_t base, uint16_t p);
uint8_t getCRC(uint8_t message[], uint8_t length);
uint8_t byte_stuff(uint8_t* data_frame, uint8_t data_frame_size, uint8_t* stuffed_frame, uint8_t stuffed_frame_size);
uint8_t byte_unstuff(uint8_t* stuffed_frame, uint8_t stuffed_frame_size, uint8_t* data_frame, uint8_t data_frame_size);
uint8_t get_size_after_stuffing(uint8_t* data_frame, uint8_t data_frame_size);

int bullet_ship_collision(point *bullet, point *ship);
void init_timers();
void init_semaphores();
void start_timers();
void wait();

void vTimerCallbackBullets(TimerHandle_t pxTimer);
void vTimerCallbackCollisions(TimerHandle_t pxTimer);
void vTimerCallbackControllersAndDraw(TimerHandle_t pxTimer);




