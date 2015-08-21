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
	bool		AddTile(int z, int x, int y, BYTE *p_data, unsigned int size);
	bool		GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size);
  bool    FindTile(int z, int x, int y);
  unsigned __int64 cache_size();
	~TileCache(void);

protected:
	map<string,BYTE*>			tile_data_map_;
	map<string,unsigned int>	tile_size_map_;
  __int64 cache_size_;

};


}

#endif