#pragma once
#include "stdafx.h"
#include "TileName.h"
#include "TileBuffer.h"
#include "VectorBorder.h"

namespace GMX
{


class ITilePyramid
{
public:
	~ITilePyramid(void)
	{
		//empty();
	};

	virtual BOOL		addTile(int z, int x, int y, BYTE *pData, unsigned int size) = 0;
	virtual	BOOL		getTile(int z, int x, int y, BYTE *&pData, unsigned int &size) = 0;
	virtual	BOOL		tileExists(int z, int x, int y) = 0; 
	virtual BOOL		close() = 0;
	virtual int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, string vectorFile = "", 
									MercatorProjType mercType = WORLD_MERCATOR) = 0;
	virtual OGREnvelope getMercatorEnvelope() = 0;
	virtual int			getMaxZoom() = 0;
	virtual BOOL		getTileBounds (int tileBounds[92]) = 0;


	virtual __int64		tileID( int z, int x, int y)
	{
		__int64 n = ((((__int64)1)<<(2*z))-1)/3;
		n = n<<1;
		n += y*(((__int64)1)<<(z+1)) + x;
		return n;
	};

	virtual BOOL		tileXYZ(__int64 id, int &z, int &x, int &y)
	{
		int i = 0;
		for (i=0;i<23;i++)
			if (( (((((__int64)1)<<(2*i+2))-1)/3)<<1) > id) break;
		if (i==23) return FALSE;
		else z = i;
		
		__int64 n = (((((__int64)1)<<(2*z))-1)/3)<<1;

		y = ((id-n)>>(z+1));
		x = (id-n) % (1<<(z+1));

		return TRUE;
	};
};



class ITileContainer : public ITilePyramid
{

public:
	
	virtual BOOL					openForReading (string fileName) = 0;
	virtual TileType				getTileType() = 0;
	virtual MercatorProjType		getProjType() = 0;
};




class GMXTileContainer : public ITileContainer
{
public:
	GMXTileContainer			(	string				containerFileName, 
									TileType			tileType,
									MercatorProjType	mercType,
									OGREnvelope			envelope, 
									int					maxZoom, 
									BOOL				useBuffer
								);

	GMXTileContainer			(	string				containerFileName, 
									TileType			tileType,
									MercatorProjType	mercType,
									int					tileBounds[92],
									BOOL				useBuffer
								);


	GMXTileContainer				();
	~GMXTileContainer				();

	BOOL				openForReading			(string containerFileName);
	BOOL				addTile			(int z, int x, int y, BYTE *pData, unsigned int size);
	BOOL				getTile			(int z, int x, int y, BYTE *&pData, unsigned int &size);
	BOOL				tileExists		(int z, int x, int y);
	BOOL				close			();
	int 				getTileList		(	list<__int64> &tileList, 
											int minZoom, int maxZoom, 
											string vectorFile = "", 
											MercatorProjType mercType = WORLD_MERCATOR
										);
	BOOL		getTileBounds (int tileBounds[92]);


	__int64				tileID			( int z, int x, int y);
	BOOL				tileXYZ			(__int64 n, int &z, int &x, int &y);
	TileType			getTileType	();
	MercatorProjType	getProjType();
	OGREnvelope			getMercatorEnvelope();
	int					getMaxZoom		();
	

protected:
	BOOL 		init			(	int					tileBounds[92], 
									BOOL				useBuffer, 
									string				containerFileName, 
									TileType			tileType,
									MercatorProjType	mercType
								);

	BOOL	writeContainerFromBuffer();
	BOOL	addTileToContainerFile(int z, int x, int y, BYTE *pData, unsigned int size);
	BOOL	getTileFromContainerFile (int z, int x, int y, BYTE *&pData, unsigned int &size);
	BOOL	writeTilesToContainerFileFromBuffer();
	BOOL	writeContainerFileHeader();
	BOOL	writeHeaderToByteArray(BYTE*	&pData);
	unsigned int headerSize();
	void empty ();

protected:
	BOOL					bReadOnly;
	BOOL					USE_BUFFER;
	int						minx[23];
	int						miny[23];
	int						maxx[23];
	int						maxy[23];
	//int						maxZoom;
	unsigned int			maxTiles;
	unsigned int			*sizes;
	unsigned __int64		*offsets;
	TileType				tileType;
	MercatorProjType		mercType;

	TileBuffer				*poTileBuffer;
	unsigned int			MAX_TILES_IN_CONTAINER;
	string					containerFileName;
	FILE*					containerFileData;
	unsigned __int64		containerLength;
};



class MBTileContainer  : public ITileContainer
{	

public:
	MBTileContainer ();
	~MBTileContainer ();
		
	int			getMaxZoom();	
	BOOL openForReading  (string fileName);
	MBTileContainer (string fileName, TileType tileType,MercatorProjType mercType, OGREnvelope mercEnvelope);
	
	BOOL		addTile(int z, int x, int y, BYTE *pData, unsigned int size);
	BOOL		tileExists(int z, int x, int y);

	BOOL		getTile(int z, int x, int y, BYTE *&pData, unsigned int &size);
	int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, string vectorFile = "", 
							MercatorProjType mercType = WORLD_MERCATOR);
	
	OGREnvelope getMercatorEnvelope();
	BOOL		getTileBounds (int tileBounds[92]);
	TileType	getTileType();
	MercatorProjType	getProjType();
	BOOL close ();

protected:
	sqlite3	*pDB;
	TileType				tileType;
	MercatorProjType		mercType;
};


class TileFolder : public ITilePyramid
{
public:
	TileFolder (TileName *poTileName, BOOL useBuffer);
	~TileFolder ();
	BOOL close();
	BOOL	addTile(int z, int x, int y, BYTE *pData, unsigned int size);
	BOOL		getTile(int z, int x, int y, BYTE *&pData, unsigned int &size);
	BOOL		tileExists(int z, int x, int y);
	int			getMaxZoom();
	int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, string vectorFile = "",  
							MercatorProjType mercType = WORLD_MERCATOR);
	OGREnvelope getMercatorEnvelope();
	BOOL		getTileBounds (int tileBounds[92]);
	
	
protected:
	BOOL	writeTileToFile (int z, int x, int y, BYTE *pData, unsigned int size);
	BOOL	readTileFromFile (int z,int x, int y, BYTE *&pData, unsigned int &size);


protected:
	TileName	*poTileName;
	BOOL		USE_BUFFER;
	TileBuffer	*poTileBuffer;
};

ITileContainer* OpenITileContainerForReading (string fileName);

}
