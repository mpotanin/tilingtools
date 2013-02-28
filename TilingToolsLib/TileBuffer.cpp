#include "StdAfx.h"
#include "TileBuffer.h"
#include "StringFuncs.h"

namespace GMX
{


TileBuffer::TileBuffer(void)
{
}


TileBuffer::~TileBuffer(void)
{
	for (map<string,BYTE*>::const_iterator iter = tileData.begin(); iter!=tileData.end(); iter++)
		delete[]((*iter).second);
	tileData.empty();
	tileSize.empty();
}


BOOL	TileBuffer::addTile(int z, int x, int y, BYTE *pData, unsigned int size)
{
	string tileKey = ConvertIntToString(z) + "_" + ConvertIntToString(x) + "_" + ConvertIntToString(y);
	if (tileData.find(tileKey)!=tileData.end())
	{
		delete[](*tileData.find(tileKey)).second;
		tileData.erase(tileKey);
		tileSize.erase(tileKey);
	}

	BYTE	*pData_copy = new BYTE[size];
	memcpy(pData_copy,pData,size);
	tileData.insert(pair<string,BYTE*>(tileKey,pData_copy));
	tileSize.insert(pair<string,unsigned int>(tileKey,size));

	return TRUE;
}


BOOL	TileBuffer::getTile(int z, int x, int y, BYTE *&pData, unsigned int &size)
{
	string tileKey = ConvertIntToString(z) + "_" + ConvertIntToString(x) + "_" + ConvertIntToString(y);
	map<string,BYTE*>::const_iterator iter;
	if ((iter=tileData.find(tileKey)) == tileData.end())
	{
		pData	= NULL;
		size	= 0;
		return FALSE;
	}
	else
	{
		size	= (*tileSize.find(tileKey)).second;
		pData = new BYTE[size];
		memcpy(pData,(*iter).second,size);
	}
	return TRUE;
}


}