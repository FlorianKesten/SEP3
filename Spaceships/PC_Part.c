#include <stdio.h>
#include <conio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include <stdbool.h>

#define mainRECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )
#define	mainSEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define	FLAG 0x7E
#define	ESCCHAR	0x1B
#define	POLYNOMIAL	263


static bool ACK = true;        //Message acknowledged
static bool NACK = false;      //Message not acknowledged
static uint16_t seq_num = 0;   //sequence number of the message
static bool check_NACK = true; //value to check if not acknowledged message gets resent

//tasks definitions
static void PC_send_task(void *pvParameters);
static void PC_recieve_task(void *pvParameters);
static void PC_input_task(void *pvParameters);

//methods definitions
static void open_port();
static int closeConnection();

HANDLE hSerial;
QueueHandle_t PC_input_queue;

void main_blinky(void)
{
	PC_input_queue = xQueueCreate(10, sizeof(uint8_t));
	open_port();
	xTaskCreate(PC_recieve_task,"Rx",configMINIMAL_STACK_SIZE,NULL,mainRECEIVE_TASK_PRIORITY,NULL);
	xTaskCreate(PC_send_task, "TX", configMINIMAL_STACK_SIZE, NULL, mainSEND_TASK_PRIORITY, NULL);
	xTaskCreate(PC_input_task, "Input", configMINIMAL_STACK_SIZE, NULL, mainSEND_TASK_PRIORITY, NULL);

	vTaskStartScheduler();
	for (;; );
}

void PC_input_task(void *pvParameters)
{
	uint8_t key;

	while (1) {
		if (ACK)
		{
			key = (uint8_t)getch();

			if (key == 100 || key == 97 || key == 115 || key == 119)
			{
				xQueueSend(PC_input_queue, &key, 100);
			}
		}
		vTaskDelay(300);

	}
}



static void PC_send_task(void *pvParameters)
{
	(void)pvParameters;

	//array for bytes recieved from port
	uint8_t bytes_to_receive[50] = { 0 };

	//array for bytes ready to prepare to send (unstuffed)
	uint8_t bytes_to_prepare[5] = { 0 };

	//array for ready-to-send bytes
	uint8_t bytes_to_send[8] = { 0 };

	//buffer for a byte recieved from pc_input_queue
	uint8_t byte;

	//value for CRC calculations
	uint8_t crc = 0;

	//temporary value for array sizes after stuffing/unstuffing methods
	uint8_t temp_array_size = 0;

	DWORD bytes_written, total_bytes_written = 0;


			while (1)
			{

				//ACK = true, always done first time
				if (ACK) //if the message is acknowledged
				{

					if (xQueueReceive(PC_input_queue, &byte, 100))
					{
						uint8_t to_crc[] = { seq_num, byte };
						crc = getCRC(to_crc, 2);

						//corrupt crc value so not acknoledged message can be resent (testing NACK)
						if (check_NACK)
						{
							crc++;
						}

						bytes_to_prepare[0] = seq_num;
						bytes_to_prepare[1] = byte;
						bytes_to_prepare[2] = crc;

						temp_array_size = byte_stuff(&bytes_to_prepare, 3, &bytes_to_send, 8);

						//Displaying byte_stuffed package that is sent to board
						fprintf(stderr, "Packet sent: [  ");
						for (int i = 0; i < temp_array_size; i++)
						{
							printf("%d ", bytes_to_send[i]);
						}
						fprintf(stderr, "] ");

						//sending
						if (!WriteFile(hSerial, bytes_to_send, temp_array_size, &bytes_written, NULL))
						{
							fprintf(stderr, "Error while sending\n");
							CloseHandle(hSerial);
							return 1;
						}
					}
				}
				else if(NACK) //if the message was not acknowledged
				{

					// make the corrupted data correct (testing NACK)
					if (check_NACK)
					{
						check_NACK = 0; //test only once
						bytes_to_send[3] = 2;
					}
					printf("RESENDING PACKET NUMBER: %d ", seq_num);
					if (!WriteFile(hSerial, bytes_to_send, temp_array_size, &bytes_written, NULL))
					{
						fprintf(stderr, "Error while sending\n");
						CloseHandle(hSerial);
						return 1;
					}
					NACK = false;
					vTaskDelay(100);
				}
				vTaskDelay(100);
			}
}
//Task recieving messages from the board and setting the ACK and NACK values
static void PC_recieve_task(void *pvParameters)
{
	(void)pvParameters;
	vTaskDelay(1000);


	uint8_t bytes_to_receive[50] = { 0 };
	uint8_t recieved_unstuffed[5];
	uint8_t temp_array_size = 0;

	while(1)
	{
		DWORD bytes_read = 0, total_bytes_read = 0;
		
		if (!ReadFile(hSerial, bytes_to_receive, 20, &bytes_read, total_bytes_read))
		{
			fprintf(stderr, "Error recieving \n");
			CloseHandle(hSerial);
			return 1;
		}

		if (bytes_read == 2)
		{
			printf("THE SCORE IS: %d : %d \n", bytes_to_receive[0], bytes_to_receive[1]);
		}

		else if (bytes_read > 0) {
			temp_array_size = byte_unstuff(bytes_to_receive, bytes_read, recieved_unstuffed, 5);

			//Displaying recieved and unstuffed package
			fprintf(stderr, "SEQ NUM: %d \n", seq_num);
			fprintf(stderr, "RECIEVED MESSAGE (UNSTUFFED): ");
			for (int i = 0; i < temp_array_size; i++)
			{
				fprintf(stderr, "%d  ", recieved_unstuffed[i]);
			}
			fprintf(stderr, "\n ");

			//END OF DISPLAYING
			uint8_t recieved_seq_num = recieved_unstuffed[0];
			uint8_t acknowledgement = recieved_unstuffed[1];

			if (recieved_seq_num == seq_num && acknowledgement == 1)
			{
				ACK = true;
				NACK = false;
				seq_num++;
			}
			else
			{
				ACK = false;
				NACK = true;
			}
		}
		vTaskDelay(50);
	}
}
/*-----------------------------------------------------------*/


void open_port()
{

	DCB dcbSerialParams = { 0 };
	COMMTIMEOUTS timeouts = { 0 };

	fprintf(stderr, "Opening serial port...");

	// In "Device Manager" find "Ports (COM & LPT)" and see which COM port the game controller is using (here COM4)
	hSerial = CreateFile(
		"COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hSerial == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Here Error\n");
		return;
	}
	else fprintf(stderr, "OK\n");

	// Set device parameters (115200 baud, 1 start bit,
	// 1 stop bit, no parity)
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (GetCommState(hSerial, &dcbSerialParams) == 0)
	{
		fprintf(stderr, "Error getting device state\n");
		CloseHandle(hSerial);
		return;
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (SetCommState(hSerial, &dcbSerialParams) == 0)
	{
		fprintf(stderr, "Error setting device parameters\n");
		CloseHandle(hSerial);
		return;
	}

	// Set COM port timeout settings
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if (SetCommTimeouts(hSerial, &timeouts) == 0)
	{
		fprintf(stderr, "Error setting timeouts\n");
		CloseHandle(hSerial);
		return;
	}
}



static int closeConnection() {

	// Close serial port
	fprintf(stderr, "Closing serial port...");
	if (CloseHandle(hSerial) == 0)
	{
		fprintf(stderr, "Error\n");
		return 1;
	}
	return 0;
}

uint8_t getCRC(uint8_t message[], uint8_t length)
{
	uint8_t crc = 0;

	for (int i = 0; i < length; i++)
	{
		crc ^= message[i];
		for (int j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc ^= POLYNOMIAL;
			crc >>= 1;
		}
	}
	return crc;
}
uint8_t byte_stuff(uint8_t* data_frame, uint8_t data_frame_size, uint8_t* stuffed_frame, uint8_t stuffed_frame_size)
{
	*stuffed_frame = FLAG;
	stuffed_frame++;
	uint8_t size = 1;
	for (uint8_t i = 0; i < data_frame_size; i++)
	{
		if (*data_frame == ESCCHAR || *data_frame == FLAG)
		{
			*stuffed_frame = ESCCHAR;
			stuffed_frame++;
			size++;
		}

		*stuffed_frame = *data_frame;
		data_frame++;
		stuffed_frame++;
		size++;
	}
	*stuffed_frame = FLAG;
	size++;
	return size;
}

uint8_t byte_unstuff(uint8_t* stuffed_frame, uint8_t stuffed_frame_size, uint8_t* data_frame, uint8_t data_frame_size)
{
	stuffed_frame++;
	uint8_t size = 0;

	for (uint8_t i = 0; i < stuffed_frame_size; i++)
	{
		if (*stuffed_frame == ESCCHAR)
		{
			stuffed_frame++;
			*data_frame = *stuffed_frame;
			stuffed_frame++;
			data_frame++;
			size++;
		}
		else {
			*data_frame = *stuffed_frame;
			data_frame++;
			stuffed_frame++;
			size++;
		}
		if (*stuffed_frame == FLAG)
		{
			return size;
		}
	}
	return size;
}

