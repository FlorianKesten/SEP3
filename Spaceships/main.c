#include "utils.h"
#include "RTOS_utils.h"

static point ship1 = {(uint8_t)0,(uint8_t)0};
static point ship2 = {(uint8_t)0,(uint8_t)0};
static point bullet2 = {(uint8_t)0,(uint8_t)0};
static point bullet1 = {(uint8_t)0,(uint8_t)0};
	
QueueHandle_t pc_ship_queue;

//tasks definitions
static void check_collisions_task(void *pvParameters);
static void joystick_task(void *pvParameters);
static void update_bullets_cords_task(void *pvParameters);
static void PCconn_task(void *pvParameters);
static void draw_ships_task(void *pvParameters);
static void draw_bullets_task(void *pvParameters);

//methods definitions
init_variables();
void clear_frame();
void handle_display(void);
void display_points();




///////////TASKS START HERE///////////
static void check_collisions_task(void *pvParameters)
{
	(void)pvParameters;
	
	vTaskSetApplicationTaskTag( NULL, ( void * ) 1);
	
	uint8_t ship1shot = 0;
	uint8_t ship2shot = 0;
	while(1)
	{
		if(xSemaphoreTake(xTimerSemaphoreCollisions, portMAX_DELAY))
		{

			//check for bullet - bullet collision
			if(((getX(&bullet1) == getX(&bullet2)) && (getY(&bullet1) == (getY(&bullet2) - 1))) || ((getX(&bullet1) == getX(&bullet2)) && (getY(&bullet1) == (getY(&bullet2)))))
			{
				setY(&bullet1,20);
				setY(&bullet2,0);
				setX(&bullet2,20);

			}
			
			ship1shot = bullet_ship_collision(&bullet2,&ship1);
			ship2shot = bullet_ship_collision(&bullet1,&ship2);
			

			if(ship1shot || ship2shot)
			{
				if(ship1shot && !ship2shot)
				{
					results[1]++;
					display_points();
					com_send_bytes((uint8_t *)results, 2); //change it so it's showing the score
								
				}
				else if(ship2shot && !ship1shot)
				{
					results[0]++;
					display_points();
					com_send_bytes((uint8_t *)results, 2); //change it so it's showing the score

				}
				else 
				{
					com_send_bytes((uint8_t *)results, 2);
				}
				

 				xTimerStop(xTimerControllersAndDraw,10);
				init_variables();
				wait();
				xTimerStart(xTimerControllersAndDraw,0);
 				
				if(results[0] > 2 || results[1] > 2)
 				{
 					vTaskSuspendAll();
 				}

			}
		}
	}
}

static void joystick_task(void *pvParameters)
{
	(void)pvParameters;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 2);
	while(1)
	{
		if(xSemaphoreTake(xTimerSemaphoreJoystick, portMAX_DELAY))
		{


			if(joystick_func() == 194) //right
			{
				if(getX(&ship1)> 7)
				{
					setX(&ship1,8);
				}
				else
				setX(&ship1,getX(&ship1) + 1);

			}
			else if(joystick_func() == 131)//left
			{
				if(getX(&ship1) < 2)
				{
					setX(&ship1,1);
				}
				else
				setX(&ship1,getX(&ship1) - 1);
			}
			else if(joystick_func() == 67)//down
			{
				if(getY(&ship1) < 1)
				{
					setY(&ship1,0);
				}
				else
				setY(&ship1,getY(&ship1) - 1);

				
			}
			else if(joystick_func() == 193) //up
			{
				if(getY(&ship1) > 4)
				{
					setY(&ship1,5);
				}
				else
				setY(&ship1,getY(&ship1) + 1);

			}
		}
	}
}

static void update_bullets_cords_task(void *pvParameters)
{
	(void)pvParameters;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 3);
	while(1)
	{
		if(xSemaphoreTake(xTimerSemaphoreBullets, portMAX_DELAY))
		{
			//update first bullet
			if(getY(&bullet1) > 12)
			{
				setX(&bullet1,getX(&ship1));
				setY(&bullet1,getY(&ship1));

			}
			setY(&bullet1,getY(&bullet1)+1);

			//update second bullet			
			if(getY(&bullet2) < 1)
			{
				setX(&bullet2,getX(&ship2));
				setY(&bullet2,getY(&ship2));

			}
			setY(&bullet2,getY(&bullet2)-1);
		}
	}
}

static void PCconn_task(void *pvParameters)
{
	(void)pvParameters;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 4);

	BaseType_t result = 0;
	uint8_t byte;
	uint8_t length = 0;
	uint8_t crc = 0;
	uint8_t key_input = 0;
	uint8_t recieved_seq_num = 0;
	
	uint8_t recieved_frame[8] = {0};
	uint8_t unstuffed_frame[8] = {0};
	uint8_t to_send_frame[8] = {0};
	
	while(1)
	{
		if(xSemaphoreTake(xTimerSemaphorePCConn,portMAX_DELAY))
		{
			while(xQueueReceive(_x_com_received_chars_queue, &byte, 5))
			{
				recieved_frame[length++] = byte;
			}
			
			if(length > 0)
			{
			byte_unstuff(recieved_frame,length,unstuffed_frame,3);

			recieved_seq_num = unstuffed_frame[0];
			key_input = unstuffed_frame[1];
			crc = unstuffed_frame[2];
			uint8_t temp_table[2] = {recieved_seq_num,key_input};
			
			if(crc == getCRC(temp_table,2))
			{
				temp_table[0] = seq_num;
				temp_table[1] = ACK;
				seq_num++;
				xQueueSend(pc_ship_queue,&key_input,10);
			}
			else
			{
				temp_table[0] = seq_num;
				temp_table[1] = NACK;
			}
			
			uint8_t send_size = byte_stuff(temp_table,2,to_send_frame,8);
			com_send_bytes(to_send_frame,send_size);
			length = 0;
			}
		}
	}
}

static void draw_ships_task(void *pvParameters)
{
	(void)pvParameters;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 5);
	
	while (1)
	{
		if(xSemaphoreTake(xTimerSemaphoreDrawShips, portMAX_DELAY))
		{
			if(xSemaphoreTake(xFramebufMutex,portMAX_DELAY))
			{
				clear_frame();
				//draw first ship
				uint16_t a = power(2,getX(&ship1)-1);
				uint16_t b = power(2,getX(&ship1));
				uint16_t c = power(2,getX(&ship1)+1);
				
				frame_buf[getY(&ship1)] = a + b + c;
				frame_buf[getY(&ship1)+1] = power(2,getX(&ship1));
				
				//draw second ship
				a = power(2,getX(&ship2)-1);
				b = power(2,getX(&ship2));
				c = power(2,getX(&ship2)+1);
				
				frame_buf[getY(&ship2)] = a + b + c;
				frame_buf[getY(&ship2)-1] = power(2,getX(&ship2));
				xSemaphoreGive(xFramebufMutex);
			}
		}
	}
}

static void draw_bullets_task(void *pvParameters)
{
	(void)pvParameters;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 6);

	while (1)
	{
		if(xSemaphoreTake(xTimerSemaphoreDrawBullets, portMAX_DELAY))
		{
			if(xSemaphoreTake(xFramebufMutex,portMAX_DELAY))
			{
				//draw first bullet
				
				if(frame_buf[getY(&bullet1)] != power(2,getX(&bullet1)))
				frame_buf[getY(&bullet1)] += power(2,getX(&bullet1));

				//draw second bullet
				
				if(frame_buf[getY(&bullet2)] != power(2,getX(&bullet2)))
				frame_buf[getY(&bullet2)] += power(2,getX(&bullet2));
				xSemaphoreGive(xFramebufMutex);
			}
		}
	}
}

static void move_pc_ship_task(void *pvParameters)
{
	(void)pvParameters;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 5);
	uint8_t key_input = 0;
	while(1)
	{
		if(xSemaphoreTake(xTimerSemaphoreMoveShip, portMAX_DELAY))
		{
		if(xQueueReceive(pc_ship_queue,&key_input,10))
		{
	 			if(key_input==100)
	 			{
		 			if(getX(&ship2) < 2)
		 			{
			 			setX(&ship2,1);
		 			}
		 			else
		 			setX(&ship2,getX(&ship2) - 1);
	 			}
	 			else if(key_input==97)
	 			{
		 			if(getX(&ship2) > 7)
		 			{
			 			setX(&ship2,8);
		 			}
		 			else
		 			setX(&ship2,getX(&ship2) + 1);
		 			
	 			}
	 			else if(key_input==115)
	 			{
		 			
		 			if(getY(&ship2) > 12)
		 			{
			 			setY(&ship2, 13);
		 			}
		 			else
		 			setY(&ship2,getY(&ship2) + 1);
		 			
		 			
	 			}
	 			else if(key_input==119)
	 			{
		 			if(getY(&ship2) < 9)
		 			{
			 			setY(&ship2, 8);
		 			}
		 			else
		 				setY(&ship2,getY(&ship2) + 1);
				 }
	}
	}
	}
}
///////////TASKS END HERE///////////

int main(void)
{
	init_board();
	
	// Shift register Enable output (G=0)
	PORTD &= ~_BV(PORTD6);

	_x_com_received_chars_queue = xQueueCreate( _COM_RX_QUEUE_LENGTH, ( unsigned portBASE_TYPE ) sizeof( uint8_t ) );
	init_com(_x_com_received_chars_queue);

	pc_ship_queue = xQueueCreate(10,sizeof(uint8_t));
	
	seq_num = 0;
	results[0] = 0;
	results[1] = 0;
	init_variables();
	init_timers();
	init_semaphores();
	start_timers();
	
	BaseType_t t1 = xTaskCreate(check_collisions_task, (const char *)"check_collisions_task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
	BaseType_t t2 = xTaskCreate(joystick_task, (const char *)"joystick_task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	BaseType_t t3 = xTaskCreate(draw_ships_task, (const char *)"draw_ships_task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	BaseType_t t4 = xTaskCreate(update_bullets_cords_task, (const char *)"bullets_update_task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	BaseType_t t5 = xTaskCreate(draw_bullets_task, (const char *)"draw_bullets_task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	//BaseType_t t6 = xTaskCreate(PCconn_task, (const char *)"PCconn_task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	//BaseType_t t7 = xTaskCreate(move_pc_ship_task, (const char *)"move_pc_ship", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);


	// Start the display handler timer
	init_display_timer(handle_display);
	
	sei();
	
	//Start the scheduler
	vTaskStartScheduler();
	
	//Should never reach here
	while (1)
	{
		vTaskDelay(1000);
	}
}

init_variables()
{
	setX(&bullet1,0);
	setY(&bullet1,15);
	
	setX(&bullet2,15);
	setY(&bullet2,0);
	
	setX(&ship1,4);
	setY(&ship1,0);
	
	setX(&ship2,4);
	setY(&ship2,13);
}

void clear_frame()
{
	for(int i = 0; i < 14; i++)
	{
		frame_buf[i] = 0;
	}
}


void handle_display(void)
{
	static uint8_t col = 0;
	
	if (col == 0)
	{
		prepare_shiftregister();
	}
	
	load_col_value(frame_buf[col]);
	
	clock_shift_register_and_prepare_for_next_col();
	
	// count column up - prepare for next
	col++;
	if (col > 13)
	{
		col = 0;
	}
}


void display_points()
{
	clear_frame();
	if(results[1] < 3)
	{
		if(results[0] == 0)
		{
			frame_buf[1] = 56;
			frame_buf[2] = 40;
			frame_buf[3] = 40;
			frame_buf[4] = 40;
			frame_buf[5] = 56;
			
		}
		else if(results[0] == 1)
		{
			frame_buf[1] = 56;
			frame_buf[2] = 16;
			frame_buf[3] = 16;
			frame_buf[4] = 24;
			frame_buf[5] = 16;
		}
		else if(results[0] == 2)
		{
			frame_buf[1] = 56;
			frame_buf[2] = 8;
			frame_buf[3] = 56;
			frame_buf[4] = 32;
			frame_buf[5] = 56;
		}
		else if (results[0] == 3)
		{
			frame_buf[0] = 10;
			frame_buf[1] = 10;
			frame_buf[2] = 910;
			frame_buf[3] = 640;
			frame_buf[4] = 654;
			frame_buf[5] = 10;
			frame_buf[6] = 910;
			frame_buf[7] = 640;
			frame_buf[8] = 896;
			frame_buf[9] = 31;
			frame_buf[10] = 277;
			frame_buf[11] = 277;
			frame_buf[12] = 640;
			frame_buf[13] = 640;
		}
		}
		
		if(results[0] < 3)
		{
			if(results[1] == 0)
			{
				frame_buf[8] = 56;
				frame_buf[9] = 40;
				frame_buf[10] = 40;
				frame_buf[11] = 40;
				frame_buf[12] = 56;
			}
			else if(results[1] == 1)
			{
				frame_buf[8] = 16;
				frame_buf[9] = 24;
				frame_buf[10] = 16;
				frame_buf[11] = 16;
				frame_buf[12] = 56;
			}
			else if(results[1] == 2)
			{
				frame_buf[8] = 56;
				frame_buf[9] = 8;
				frame_buf[10] = 56;
				frame_buf[11] = 32;
				frame_buf[12] = 56;
			}
			else if (results[1] == 3)
			{
				frame_buf[0] = 21;
				frame_buf[1] = 21;
				frame_buf[2] = 671;
				frame_buf[3] = 640;
				frame_buf[4] = 270;
				frame_buf[5] = 266;
				frame_buf[6] = 14;
				frame_buf[7] = 896;
				frame_buf[8] = 654;
				frame_buf[9] = 906;
				frame_buf[10] = 10;
				frame_buf[11] = 640;
				frame_buf[12] = 640;
				frame_buf[13] = 896;
			}
			
		}
}



