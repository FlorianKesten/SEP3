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

static const uint8_t _COM_RX_QUEUE_LENGTH = 30;
static QueueHandle_t _x_com_received_chars_queue = NULL;

void prepare_shiftregister();
void clock_shift_register_and_prepare_for_next_col();
void load_col_value(uint16_t col_value);
void handle_display(void);
void vApplicationIdleHook( void );
int joystick_func();
int button_func();
int key_received();