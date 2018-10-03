#pragma once
#ifndef HEADER_H
#define HEADER_H

typedef struct point{
	uint8_t x;
	uint8_t y;
}point;


void point_setCoordinates(struct point *self, uint8_t x, uint8_t y);
uint8_t getX(struct point *self);
uint8_t getY(struct point *self);
void setX(struct point *self, uint8_t x);
void setY(struct point *self, uint8_t y);

#endif // HEADER_H