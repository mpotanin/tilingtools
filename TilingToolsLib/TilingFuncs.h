#pragma once

#include "stdafx.h"
#include "TileName.h"
#include "TilePyramid.h"
#include "RasterFile.h"


class GMTilingParameters
{
public:
//обязательные параметры
	string						inputPath;				//входной файл или шаблон имени файла
	GMT::MercatorProjType		mercType;				//тип Меркатора
	GMT::TileType				tileType;				//тип тайлов
	

//дополнительные параметры
	int						baseZoom;				//максимальный (базовый зум)
	int						minZoom;				//минимальный зум
	string					vectorFile;				//векторная граница
	string					containerFile;			//название файла-контейнера тайлов
	bool					useContainer;			//писать тайлы в контейнер
	BYTE					*pBackgroundColor;		//RGB-цвет для заливки фона в тайлах
	BYTE					*pTransparentColor;		//RGB-цвет для маски прозрачности
	int						*pNoDataValue;			//значение для маски прозрачности

	int						nJpegQuality;
	double					dShiftX;				//сдвиг по x
	double					dShiftY;				//сдвиг по y
	GMT::TileName			*poTileName;			//имена тайлов
	int						maxTilesInCache;			//максимальное количество тайлов в оперативной памяти

	static const int		DEFAULT_JPEG_QUALITY = 80;


	GMTilingParameters(string inputPath, GMT::MercatorProjType mercType, GMT::TileType tileType)
	{
		this->inputPath = inputPath;
		this->mercType	= mercType;
		this->tileType	= tileType;
		
		this->useContainer = TRUE;
		this->nJpegQuality = GMTilingParameters::DEFAULT_JPEG_QUALITY;
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

		
	~GMTilingParameters ()
	{
		delete(pBackgroundColor);
		delete(pTransparentColor);
		delete(pNoDataValue);		
	}
};



BOOL GMTPrintTilingProgress (int nExpectedTiles, int nGeneratedTiles);



BOOL GMTMakeTiling (	GMTilingParameters		*poParams);


BOOL GMTTilingFromBuffer	(	GMTilingParameters				*poParams,
								GMT::RasterBuffer				&oBuffer, 
								GMT::BundleOfRasterFiles		*poBundle, 
								int								nULx, 
								int								nULy,
								int								z,
								int								nTilesExpected, 
								int								&nTilesGenerated,
								GMT::TilePyramid				*tileContainer);
	

BOOL GMTMakeBaseZoomTiling (GMTilingParameters				*poParams, 
							GMT::BundleOfRasterFiles		*poBundle, 
							int								nExpected, 
							GMT::TilePyramid				*tileContainer);



BOOL GMTCreatePyramidalTiles (	GMT::VectorBorder	&oVectorBorder, 
								int					nBaseZoom, 
								int					nMinZoom, 
								GMTilingParameters	*poParams, 
								int					&nExpectedTiles, 
								int					&nGeneratedTiles, 
								BOOL				bOnlyCalculate, 
								GMT::TilePyramid	*tilePyramid,
								int					nJpegQuality	= 80
								);


BOOL GMTCreateZoomOutTile (	GMT::VectorBorder				&oVectorBorder,
								int								nCurrZoom,
								int								nX,
								int								nY,
								int								nBaseZoom,
								int								nMinZoom,
								GMTilingParameters				*poParams,
								GMT::RasterBuffer				&oBuffer, 
								int								&nExpectedTiles,
								int								&nGeneratedTiles,
								BOOL							bOnlyCalculate,
								GMT::TilePyramid				*tilePyramid,
								int								nJpegQuality = 80);


BOOL ZoomOutTileBuffer		(	GMT::RasterBuffer				srcQuarterTile[4], 
								BOOL							quarterTileExists[4], 
								GMT::RasterBuffer				&zoomOutTileBuffer); 


int GMTCalcBaseZoomTilesForImage (	string						strImage, 
									string						vectorFile, 
									GMTilingParameters			*poParams, 
									GMT::BundleOfRasterFiles	*poBundle, 
									list<string>				&tilesList);


int GMTCalcBaseZoomTilesForBundle (GMTilingParameters			*poParams, 
									GMT::BundleOfRasterFiles	*poBundle, 
									int							&nAllTiles, 
									list<string>				&tilesList );



//#endif