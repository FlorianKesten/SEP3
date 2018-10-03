#include <stdint.h>
#include "point.h"


void point_setCoordinates(struct point *self, uint8_t x, uint8_t y)
{
	self->x = x;
	self->y = y;
}

uint8_t getX(struct point *self)
{
		return self->x;
}

uint8_t getY(struct point *self)
{
		return self->y;
}

void setX(struct point *self, uint8_t x)
{
		self->x = x;
}

void setY(struct point *self, uint8_t y)
{
		self->y = y;
}
