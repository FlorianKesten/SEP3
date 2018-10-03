#pragma once
#ifndef HEADER_H
#define HEADER_H
#include "utils.h"
// Prepare shift register setting SER = 1


void init_timers()
{
	xTimerCollisions = xTimerCreate("Timer collisions",80, pdTRUE, 0, vTimerCallbackCollisions);
	xTimerControllersAndDraw = xTimerCreate("Timer controllers and draw",100, pdTRUE, 0, vTimerCallbackControllersAndDraw);
}

void init_semaphores()
{
	xTimerSemaphoreJoystick = xSemaphoreCreateBinary();
	xTimerSemaphoreDrawShips = xSemaphoreCreateBinary();
	xTimerSemaphoreDrawBullets = xSemaphoreCreateBinary();
	xTimerSemaphorePCConn = xSemaphoreCreateBinary();
	xTimerSemaphoreBullets = xSemaphoreCreateBinary();
	xTimerSemaphoreCollisions = xSemaphoreCreateBinary();
	xTimerSemaphoreMoveShip = xSemaphoreCreateBinary();
	xFramebufMutex = xSemaphoreCreateMutex();
}

void start_timers()
{
	xTimerStart(xTimerCollisions, 0);
	xTimerStart(xTimerControllersAndDraw, 0);

}

void vTimerCallbackCollisions(TimerHandle_t pxTimer){
	xSemaphoreGive(xTimerSemaphoreCollisions);
}

void vTimerCallbackControllersAndDraw(TimerHandle_t pxTimer){
	xSemaphoreGive(xTimerSemaphoreDrawShips);
	xSemaphoreGive(xTimerSemaphoreJoystick);
	xSemaphoreGive(xTimerSemaphorePCConn);
	xSemaphoreGive(xTimerSemaphoreDrawBullets);
	xSemaphoreGive(xTimerSemaphoreBullets);
	xSemaphoreGive(xTimerSemaphoreMoveShip);

}

void wait()
{
	for(uint16_t i = 0; i < 2000; i++)
	{
		for(uint16_t j = 0; j < 2000; j++)
			getCRC(j,i);
	}
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

int bullet_ship_collision(point *bullet, point *ship)
{
	if(getY(bullet) == getY(ship))
	{
		if((getX(bullet) == getX(ship)-1) || (getX(bullet) == getX(ship)+1))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else if (getX(bullet) == getX(ship) && getY(bullet) == getY(ship)+1)
	{
		return 1;
	}
	
	else
	{
		return 0;
	}
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

int power(uint16_t base, uint16_t p)
{
	if(base == 0 || p < 0)
	{
		return 0;
	}
	
	int result = 1;
	for(int i = 0; i < p; i++)
	{
		result *= base;
	}
	return result;
}

uint8_t get_size_after_stuffing(uint8_t* data_frame, uint8_t data_frame_size)
{
	uint8_t size = 2; //for both flags
	for(uint8_t i = 0; i < data_frame_size; i++)
	{
		if(*data_frame == ESCCHAR || *data_frame == FLAG)
		{
			size++;
		}
		size++;
	}
	data_frame++;
	return size;
}
#endif // HEADER_H