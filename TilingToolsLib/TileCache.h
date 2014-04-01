#ifndef GMX_TILE_CACHE_H
#define GMX_TILE_CACHE_H
#pragma once
#include	"stdafx.h"


namespace gmx
{

class TileCache
{
public:
	TileCache(void);
	BOOL		AddTile(int z, int x, int y, BYTE *p_data, unsigned int size);
	BOOL		GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size);
  BOOL    FindTile(int z, int x, int y);
  unsigned __int64 cache_size();
	~TileCache(void);

protected:
	map<string,BYTE*>			tile_data_map_;
	map<string,unsigned int>	tile_size_map_;
  __int64 cache_size_;

};


}

#endif