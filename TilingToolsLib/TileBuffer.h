#pragma once
#include	"stdafx.h"

namespace GMT
{


class TileBuffer
{
public:
	TileBuffer(void);
	BOOL		addTile(int z, int x, int y, BYTE *pData, unsigned int size);
	BOOL		getTile(int z, int x, int y, BYTE *&pData, unsigned int &size);
	~TileBuffer(void);
protected:
	map<wstring,BYTE*>			tileData;
	map<wstring,unsigned int>	tileSize;

};


}