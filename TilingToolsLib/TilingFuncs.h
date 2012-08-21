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
	int						*backgroundColor;		//фон тайлов

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
		this->backgroundColor = NULL;
		this->baseZoom	= 0;
		this->maxTilesInCache	= 0;
	}


	TilingParameters& operator = (TilingParameters &oParams)
	{
		this->baseZoom			= baseZoom;
		this->nJpegQuality		= oParams.nJpegQuality;
		this->inputFile			= oParams.inputFile;
		this->vectorFile		= oParams.vectorFile;
		this->poTileName		= oParams.poTileName;
		this->minZoom			= oParams.minZoom;
		this->mercType			= oParams.mercType;
		this->useContainer		= oParams.useContainer;
		this->tileType			= oParams.tileType; 
		this->dShiftX			= oParams.dShiftX;
		this->dShiftY			= oParams.dShiftY;
		this->maxTilesInCache	= oParams.maxTilesInCache;

		if (oParams.backgroundColor!=0)
		{
			this->backgroundColor[0] = oParams.backgroundColor[0];
			this->backgroundColor[1] = oParams.backgroundColor[1];
			this->backgroundColor[2] = oParams.backgroundColor[2];
		}

		return (*this);
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



BOOL CreaterPyramidalTiles (VectorBorder	&oVectorBorder, 
						int					nBaseZoom, 
						int					nMinZoom, 
						TilingParameters	&oParams, 
						int					&nExpectedTiles, 
						int					&nGeneratedTiles, 
						BOOL				bOnlyCalculate, 
						TilePyramid			*tilePyramid,
						int					nJpegQuality	= 80
						);


BOOL MakeZoomOutTile (VectorBorder				&oVectorBorder,
					  int						nCurrZoom,
					  int						nX,
					  int						nY,
					  int						nBaseZoom,
					  int						nMinZoom,
					  TilingParameters			&oParams,
					  RasterBuffer				&oBuffer, 
					  BOOL						&bBlackTile,
					  int						&nExpectedTiles,
					  int						&nGeneratedTiles,
					  BOOL						bOnlyCalculate,
					  TilePyramid				*tilePyramid,
					  //vector<pair<wstring,pair<void*,int>>> *tilesCash = NULL,
					  int						nJpegQuality = 80);



int CalcBaseZoomTilesForImage (wstring strImage, wstring vectorFile, TilingParameters	&oParams, BundleOfRasterFiles *poBundle, list<wstring> &tilesList_);

int CalcBaseZoomTilesForBundle (TilingParameters	&oParams, BundleOfRasterFiles *poBundle, int &nAllTiles, list<wstring> &tilesList );



//#endif