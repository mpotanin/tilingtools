#include "tilecache.h"
//#include "stringfuncs.h"


namespace ttx
{

TileCache::TileCache(int64_t cache_max_size)
{
  cache_size_=0;
  cache_max_size_ = (cache_max_size==0) ? DEFAULT_CACHE_MAX_SIZE : cache_max_size;
}


TileCache::~TileCache(void)
{
	for (map<string,unsigned char*>::const_iterator iter = tile_data_map_.begin(); iter!=tile_data_map_.end(); iter++)
		delete[]((*iter).second);
	tile_data_map_.empty();
	tile_size_map_.empty();
}


bool	TileCache::AddTile(int z, int x, int y, unsigned char* p_data, unsigned int size)
{

  addtile_mutex_.lock();
  if (cache_size_ + size> cache_max_size_)
  {
    addtile_mutex_.unlock();
    return false;
  }

	string tile_key = std::to_string(z) + "_" + std::to_string(x) + "_" + std::to_string(y);
	if (tile_data_map_.find(tile_key)!=tile_data_map_.end())
	{
    delete[](*tile_data_map_.find(tile_key)).second;
		tile_data_map_.erase(tile_key);
	  cache_size_-=(*tile_size_map_.find(tile_key)).second;
    tile_size_map_.erase(tile_key);
	}

	unsigned char* p_data_copy = new unsigned char[size];
	memcpy(p_data_copy,p_data,size);
	tile_data_map_[tile_key]=p_data_copy;
	tile_size_map_[tile_key]=size;
  cache_size_+=size;
  addtile_mutex_.unlock();
	return true;
}


bool	TileCache::GetTile(int z, int x, int y, unsigned char* &p_data, unsigned int &size)
{
	string tile_key = std::to_string(z) + "_" + std::to_string(x) + "_" + std::to_string(y);
	map<string,unsigned char*>::const_iterator iter;
	if ((iter=tile_data_map_.find(tile_key)) == tile_data_map_.end())
	{
		p_data	= NULL;
		size	= 0;
		return false;
	}
	else
	{
		size	= (*tile_size_map_.find(tile_key)).second;
		p_data = new unsigned char[size];
		memcpy(p_data,(*iter).second,size);
	}
	return true;
}

bool	TileCache::FindTile(int z, int x, int y)
{
  string tile_key = std::to_string(z) + "_" + std::to_string(x) + "_" + std::to_string(y);
	return (tile_data_map_.find(tile_key) == tile_data_map_.end()) ? false : true;
}


uint64_t TileCache::cache_size()
{
  return cache_size_;
}


}