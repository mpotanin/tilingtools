#pragma once
#include "stdafx.h"
#include "TileName.h"


//unsigned int GetTiffTileType (GDALDataType type, int bands);



class TilesContainer
{
public:
	
public:
	TilesContainer	(	OGREnvelope envelope, 
						int			maxZoom, 
						BOOL		useBuffer, 
						BOOL		useContainer,
						TileName	*poTileName	= NULL,
						wstring		containerFileName = L"", 
						TileType	tileType = JPEG_TILE,
						MercatorProjType	mercType = WORLD_MERCATOR);
	TilesContainer (wstring containerFileName, TileName *poTileName);


	

	BOOL			addTile(int x, int y, int z, BYTE *pData, unsigned int size);
	BOOL			getTile(int x, int y, int z, BYTE *&pData, unsigned int &size);

	BOOL			tileExists	(int x, int y, int z); 
	unsigned int	tileNumber	(int x, int y, int z);
	void			tileXYZ		(unsigned int n, int &x, int &y, int &z);
	BOOL			closeContainerFile();
	//BOOL			unpackAllTiles();
	int 			getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile = L"");

	//BOOL			
	~TilesContainer(void);

public:


protected:
	
	BOOL			writeContainerFromBuffer();

	BOOL			addTileToBuffer(int x, int y, int z, BYTE *pData, unsigned int size);
	//BOOL	addTileToContainer(int x, int y, int z, BYTE *pData, unsigned int size);
	BOOL			writeTileToFile (int x, int y, int z, BYTE *pData, unsigned int size);
	//ToDo
	BOOL			addTileToContainerFile(int x, int y, int z, BYTE *pData, unsigned int size);


	BOOL			tileExistsInBuffer (int x, int y, int z);
	BOOL			tileExistsOnDisk	(int x, int y, int z);

	BOOL			getTileFromBuffer(int x, int y, int z, BYTE *&pData, unsigned int &size);
	BOOL			readTileFromFile (int x, int y, int z, BYTE *&pData, unsigned int &size);
	
	//ToDo
	BOOL			getTileFromContainerFile (int x, int y, int z, BYTE *&pData, unsigned int &size);

	BOOL			writeTilesToContainerFileFromBuffer();
	BOOL			writeContainerFileHeader();
	BOOL			writeHeaderToByteArray(BYTE*	&pData);
	int 			getTileListFromContainerFile(list<__int64> &tilesList, int minZoom, int maxZoom, wstring vectorFile = L"");
	int 			getTileListFromDisk(list<__int64> &tilesList, int minZoom, int maxZoom, wstring vectorFile = L"");

	//BOOL	getTileFromFile(int x, int y, int z, BYTE *&pData, unsigned int &size);
	//BOOL	getTileFromContainer(int x, int y, int z, BYTE *&pData, unsigned int &size);
	unsigned int	headerSize();
	BOOL			empty();


protected:
	BOOL					USE_CONTAINER;
	BOOL					USE_BUFFER;
	TileName				*poTileName;
	int						minx[32];
	int						miny[32];
	int						maxx[32];
	int						maxy[32];
	unsigned int			MAX_TILES_IN_CONTAINER;
	int						maxZoom;
	unsigned int			maxTiles;
	unsigned int			*sizes;
	BYTE					**tilesData;
	unsigned __int64		*offsets;

	wstring					containerFileName;
	FILE*					containerFileData;
	unsigned __int64		containerLength;
	TileType				tileType;
	MercatorProjType		mercType;
};
