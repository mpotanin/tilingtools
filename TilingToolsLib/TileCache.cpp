#include "StdAfx.h"
#include "TileCache.h"
#include "StringFuncs.h"
const int	GMX_TILE_CACHE_MAX_SIZE = 800000000;


namespace gmx
{


TileCache::TileCache(void)
{
  cache_size_=0;
}


TileCache::~TileCache(void)
{
	for (map<string,BYTE*>::const_iterator iter = tile_data_map_.begin(); iter!=tile_data_map_.end(); iter++)
		delete[]((*iter).second);
	tile_data_map_.empty();
	tile_size_map_.empty();
}


BOOL	TileCache::AddTile(int z, int x, int y, BYTE *p_data, unsigned int size)
{

  if (cache_size_ + size> GMX_TILE_CACHE_MAX_SIZE) return FALSE;

	string tile_key = ConvertIntToString(z) + "_" + ConvertIntToString(x) + "_" + ConvertIntToString(y);
	if (tile_data_map_.find(tile_key)!=tile_data_map_.end())
	{
    delete[](*tile_data_map_.find(tile_key)).second;
		tile_data_map_.erase(tile_key);
	  cache_size_-=(*tile_size_map_.find(tile_key)).second;
    tile_size_map_.erase(tile_key);
	}

	BYTE	*p_data_copy = new BYTE[size];
	memcpy(p_data_copy,p_data,size);
	tile_data_map_.insert(pair<string,BYTE*>(tile_key,p_data_copy));
	tile_size_map_.insert(pair<string,unsigned int>(tile_key,size));
  cache_size_+=size;
	return TRUE;
}


BOOL	TileCache::GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size)
{
	string tile_key = ConvertIntToString(z) + "_" + ConvertIntToString(x) + "_" + ConvertIntToString(y);
	map<string,BYTE*>::const_iterator iter;
	if ((iter=tile_data_map_.find(tile_key)) == tile_data_map_.end())
	{
		p_data	= NULL;
		size	= 0;
		return FALSE;
	}
	else
	{
		size	= (*tile_size_map_.find(tile_key)).second;
		p_data = new BYTE[size];
		memcpy(p_data,(*iter).second,size);
	}
	return TRUE;
}

BOOL	TileCache::FindTile(int z, int x, int y)
{
  string tile_key = ConvertIntToString(z) + "_" + ConvertIntToString(x) + "_" + ConvertIntToString(y);
	return (tile_data_map_.find(tile_key) == tile_data_map_.end()) ? FALSE : TRUE;
}


unsigned __int64 TileCache::cache_size()
{
  return cache_size_;
}


}