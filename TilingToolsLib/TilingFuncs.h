#pragma once

#include "stdafx.h"
#include "TileName.h"
#include "RasterFile.h"
#include "VectorFile.h"
#include "PixelEnvelope.h"
#include "GeometryFuncs.h"
#include "TilePyramid.h"

static int TILE_SIZE = 256;



class TilingParameters
{
//обязательные параметры
public:
	wstring					inputFile;				//входной файл
	MercatorProjType		mercType;				//тип Меркатора
	TileType				tileType;				//тип тайлов
	

//дополнительные параметры
public:
	int						baseZoom;				//максимальный (базовый зум)
	int						minZoom;				//минимальный зум
	wstring					vectorFile;				//векторная граница
	//wstring				imagesFolder;			//входная директория
	//wstring				imageType;				//разрешение входных файлов
	wstring					containerFile;			//название файла-контейнера тайлов
	bool					useContainer;			//писать тайлы в контейнер
	BYTE					*pBackgroundColor;		//RGB-цвет для заливки фона в тайлах
	BYTE					*pTransparentColor;		//RGB-цвет для маски прозрачности
	int						*pNoDataValue;			//значение для маски прозрачности

	int						nJpegQuality;
	double					dShiftX;				//сдвиг по x
	double					dShiftY;				//сдвиг по y
	TileName				*poTileName;			//имена тайлов
	int						maxTilesInCache;			//максимальное количество тайлов в оперативной памяти

public:
	static const int		DEFAULT_JPEG_QUALITY = 80;

public:
	TilingParameters(wstring inputFile, MercatorProjType mercType, TileType tileType)
	{
		this->inputFile = inputFile;
		this->mercType	= mercType;
		this->tileType	= tileType;
		
		this->useContainer = TRUE;
		this->nJpegQuality = TilingParameters::DEFAULT_JPEG_QUALITY;
		this->dShiftX = 0;
		this->dShiftY = 0;
		this->poTileName = NULL;
		this->pBackgroundColor	= NULL;
		this->pTransparentColor = NULL;
		this->pNoDataValue		= NULL;
		this->baseZoom	= 0;
		this->minZoom	= 0;
		this->maxTilesInCache	= 0;
	}


	TilingParameters& operator = (TilingParameters &oSrcParams)
	{
		this->baseZoom			= oSrcParams.baseZoom;
		this->minZoom			= oSrcParams.minZoom;
		this->nJpegQuality		= oSrcParams.nJpegQuality;
		this->inputFile			= oSrcParams.inputFile;
		this->vectorFile		= oSrcParams.vectorFile;
		this->poTileName		= oSrcParams.poTileName;
		this->minZoom			= oSrcParams.minZoom;
		this->mercType			= oSrcParams.mercType;
		this->useContainer		= oSrcParams.useContainer;
		this->tileType			= oSrcParams.tileType; 
		this->dShiftX			= oSrcParams.dShiftX;
		this->dShiftY			= oSrcParams.dShiftY;
		this->maxTilesInCache	= oSrcParams.maxTilesInCache;

		if (oSrcParams.pBackgroundColor!=NULL)
		{
			this->pBackgroundColor = new BYTE[3];
			memcpy(this->pBackgroundColor,oSrcParams.pBackgroundColor,3);
		}

		if (oSrcParams.pTransparentColor!=NULL)
		{
			this->pBackgroundColor = new BYTE[3];
			memcpy(this->pTransparentColor,oSrcParams.pTransparentColor,3);
		}

		if (oSrcParams.pNoDataValue !=NULL)
		{
			this->pNoDataValue = new int[1];
			this->pNoDataValue[0] = oSrcParams.pNoDataValue[0];
		}
		return (*this);
	}
	~TilingParameters ()
	{
		delete(pBackgroundColor);
		delete(pTransparentColor);
		delete(pNoDataValue);		
	}
};



BOOL PrintTilingProgress (int nExpectedTiles, int nGeneratedTiles);



BOOL MakeTiling (	TilingParameters		&oParams);


BOOL TilingFromBuffer (TilingParameters			&oParams,
					   RasterBuffer				&oBuffer, 
					   BundleOfRasterFiles		*poBundle, 
					   int						nULx, 
					   int						nULy,
					   int						z,
					   int						nTilesExpected, 
					   int						&nTilesGenerated,
					   TilePyramid			*tileContainer);
	

BOOL BaseZoomTiling2 (TilingParameters		&oParams, 
				   BundleOfRasterFiles		*poBundle, 
				   int nExpected, 
				   TilePyramid			*tileContainer);



BOOL CreatePyramidalTiles (VectorBorder	&oVectorBorder, 
						int					nBaseZoom, 
						int					nMinZoom, 
						TilingParameters	&oParams, 
						int					&nExpectedTiles, 
						int					&nGeneratedTiles, 
						BOOL				bOnlyCalculate, 
						TilePyramid			*tilePyramid,
						int					nJpegQuality	= 80
						);


BOOL CreateZoomOutTile (VectorBorder				&oVectorBorder,
					  int						nCurrZoom,
					  int						nX,
					  int						nY,
					  int						nBaseZoom,
					  int						nMinZoom,
					  TilingParameters			&oParams,
					  RasterBuffer				&oBuffer, 
					  int						&nExpectedTiles,
					  int						&nGeneratedTiles,
					  BOOL						bOnlyCalculate,
					  TilePyramid				*tilePyramid,
					  int						nJpegQuality = 80);

BOOL ZoomOutTileBuffer (RasterBuffer srcQuarterTile[4], 
						BOOL quarterTileExists[4], 
						RasterBuffer &zoomOutTileBuffer); 




int CalcBaseZoomTilesForImage (wstring strImage, wstring vectorFile, TilingParameters	&oParams, BundleOfRasterFiles *poBundle, list<wstring> &tilesList_);

int CalcBaseZoomTilesForBundle (TilingParameters	&oParams, BundleOfRasterFiles *poBundle, int &nAllTiles, list<wstring> &tilesList );



//#endif