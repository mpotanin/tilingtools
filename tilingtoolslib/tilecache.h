#pragma once
#include	"stdafx.h"


namespace gmx
{

class TileCache
{
public:
  static const int64_t DEFAULT_CACHE_MAX_SIZE = 800000000;
	TileCache(int64_t max_size=0);
	bool		AddTile(int z, int x, int y, char *p_data, unsigned int size);
	bool		GetTile(int z, int x, int y, char *&p_data, unsigned int &size);
  bool    FindTile(int z, int x, int y);
  uint64_t cache_size();
	~TileCache(void);

protected:
	map<string,char*>			tile_data_map_;
	map<string,unsigned int>	tile_size_map_;
  int64_t cache_size_;
  int64_t cache_max_size_;
  std::mutex addtile_mutex_;


};


}
