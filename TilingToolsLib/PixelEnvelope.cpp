#include "PixelEnvelope.h"

PixelEnvelope::PixelEnvelope(void)
{
}

PixelEnvelope::~PixelEnvelope(void)
{
}

PixelEnvelope::PixelEnvelope(int min_x, int min_y, int max_x, int max_y)
{
	this->max_x = max_x;
	this->max_y = max_y;
	this->min_x = min_x;
	this->min_y = min_y;
}

PixelEnvelope::PixelEnvelope(int min_x, int max_y, int tile_size)
{
	this->min_x = min_x;
	this->min_y = max_y-tile_size;
	this->max_x = min_x + tile_size;
	this->max_y = max_y;
}


BOOL PixelEnvelope::Intersects (PixelEnvelope oPixelEnvelope)
{
	if ((oPixelEnvelope.min_x>max_x)||
	 (oPixelEnvelope.max_x<min_x)||
	 (oPixelEnvelope.max_y<min_y)||
	 (oPixelEnvelope.min_y>max_y))
		return FALSE;
	else 
		return TRUE;
}

BOOL PixelEnvelope::Contains (PixelEnvelope oPixelEnvelope)
{
	if ((oPixelEnvelope.min_x>=min_x)&&
	 (oPixelEnvelope.max_x<=max_x)&&
	 (oPixelEnvelope.max_y<=max_y)&&
	 (oPixelEnvelope.min_y>=min_y))
		return TRUE;
	else 
		return FALSE;
}

BOOL PixelEnvelope::OnEdge (PixelEnvelope oPixelEnvelope)
{
	if (!Intersects(oPixelEnvelope)) return FALSE;
	if (Contains(oPixelEnvelope)) return FALSE;
	return TRUE;
}
