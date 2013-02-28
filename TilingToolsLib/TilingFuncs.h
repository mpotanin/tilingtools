#pragma once

#include "stdafx.h"
#include "TileName.h"
#include "TilePyramid.h"
#include "RasterFile.h"


class GMXilingParameters
{
public:
//обязательные параметры
	string						inputPath;				//входной файл или шаблон имени файла
	GMX::MercatorProjType		mercType;				//тип Меркатора
	GMX::TileType				tileType;				//тип тайлов
	

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
	GMX::TileName			*poTileName;			//имена тайлов
	int						maxTilesInCache;			//максимальное количество тайлов в оперативной памяти

	static const int		DEFAULT_JPEG_QUALITY = 80;


	GMXilingParameters(string inputPath, GMX::MercatorProjType mercType, GMX::TileType tileType)
	{
		this->inputPath = inputPath;
		this->mercType	= mercType;
		this->tileType	= tileType;
		
		this->useContainer = TRUE;
		this->nJpegQuality = GMXilingParameters::DEFAULT_JPEG_QUALITY;
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

		
	~GMXilingParameters ()
	{
		delete(pBackgroundColor);
		delete(pTransparentColor);
		delete(pNoDataValue);		
	}
};



BOOL GMXPrintTilingProgress (int nExpectedTiles, int nGeneratedTiles);



BOOL GMXMakeTiling (	GMXilingParameters		*poParams);


BOOL GMXTilingFromBuffer	(	GMXilingParameters				*poParams,
								GMX::RasterBuffer				&oBuffer, 
								GMX::BundleOfRasterFiles		*poBundle, 
								int								nULx, 
								int								nULy,
								int								z,
								int								nTilesExpected, 
								int								&nTilesGenerated,
								GMX::ITilePyramid				*tileContainer);
	

BOOL GMXMakeBaseZoomTiling (GMXilingParameters				*poParams, 
							GMX::BundleOfRasterFiles		*poBundle, 
							int								nExpected, 
							GMX::ITilePyramid				*tileContainer);



BOOL GMXCreatePyramidalTiles (	GMX::VectorBorder	&oVectorBorder, 
								int					nBaseZoom, 
								int					nMinZoom, 
								GMXilingParameters	*poParams, 
								int					&nExpectedTiles, 
								int					&nGeneratedTiles, 
								BOOL				bOnlyCalculate, 
								GMX::ITilePyramid	*ITilePyramid,
								int					nJpegQuality	= 80
								);


BOOL GMXCreateZoomOutTile (	GMX::VectorBorder				&oVectorBorder,
								int								nCurrZoom,
								int								nX,
								int								nY,
								int								nBaseZoom,
								int								nMinZoom,
								GMXilingParameters				*poParams,
								GMX::RasterBuffer				&oBuffer, 
								int								&nExpectedTiles,
								int								&nGeneratedTiles,
								BOOL							bOnlyCalculate,
								GMX::ITilePyramid				*ITilePyramid,
								int								nJpegQuality = 80);


BOOL ZoomOutTileBuffer		(	GMX::RasterBuffer				srcQuarterTile[4], 
								BOOL							quarterTileExists[4], 
								GMX::RasterBuffer				&zoomOutTileBuffer); 


int GMXCalcBaseZoomTilesForImage (	string						strImage, 
									string						vectorFile, 
									GMXilingParameters			*poParams, 
									GMX::BundleOfRasterFiles	*poBundle, 
									list<string>				&tilesList);


int GMXCalcBaseZoomTilesForBundle (GMXilingParameters			*poParams, 
									GMX::BundleOfRasterFiles	*poBundle, 
									int							&nAllTiles, 
									list<string>				&tilesList );



//#endif