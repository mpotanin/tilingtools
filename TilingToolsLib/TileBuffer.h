#pragma once
#include	"stdafx.h"

namespace GMX
{


class TileBuffer
{
public:
	TileBuffer(void);
	BOOL		addTile(int z, int x, int y, BYTE *pData, unsigned int size);
	BOOL		getTile(int z, int x, int y, BYTE *&pData, unsigned int &size);
	~TileBuffer(void);
protected:
	map<string,BYTE*>			tileData;
	map<string,unsigned int>	tileSize;

};


}