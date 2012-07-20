#pragma once
#include "stdafx.h"


class PixelEnvelope
{
public:
	PixelEnvelope(void);
	PixelEnvelope(int min_x, int max_y, int tile_size);
	PixelEnvelope(int min_x, int min_y, int max_x, int max_y);	
public:
	~PixelEnvelope(void);
public:
	BOOL Intersects (PixelEnvelope oPixelEnvelope);
	BOOL Contains	(PixelEnvelope oPixelEnvelope);
	BOOL OnEdge		(PixelEnvelope oPixelEnvelope);

public:
	int min_x;
	int min_y;
	int max_x;
	int max_y;

};
