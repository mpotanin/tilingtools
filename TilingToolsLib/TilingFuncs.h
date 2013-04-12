#pragma once

#include "stdafx.h"
#include "TileName.h"
#include "TilePyramid.h"
#include "RasterFile.h"


class GMXTilingParameters
{
public:
//обязательные параметры
	string						inputPath;				//входной файл или шаблон имени файла
	GMX::MercatorProjType		mercType;				//тип Меркатора
	GMX::TileType				tileType;				//тип тайлов
	

//дополнительные параметры
	int							baseZoom;				//максимальный (базовый зум)
	int							minZoom;				//минимальный зум
	string						vectorFile;				//векторная граница
	string						containerFile;			//название файла-контейнера тайлов
	bool						useContainer;			//писать тайлы в контейнер
	BYTE						*pBackgroundColor;		//RGB-цвет для заливки фона в тайлах
	BYTE						*pTransparentColor;		//RGB-цвет для маски прозрачности
	int							*pNoDataValue;			//значение для маски прозрачности
	bool						bAutoStretchTo8Bit;		//автоматически пересчитывать значения к 8 бит						


	int							nJpegQuality;
	double						dShiftX;				//сдвиг по x
	double						dShiftY;				//сдвиг по y
	GMX::TileName				*poTileName;			//имена тайлов
	int							maxTilesInCache;			//максимальное количество тайлов в оперативной памяти

	static const int			DEFAULT_JPEG_QUALITY = 80;


	GMXTilingParameters(string inputPath, GMX::MercatorProjType mercType, GMX::TileType tileType)
	{
		this->inputPath = inputPath;
		this->mercType	= mercType;
		this->tileType	= tileType;
		
		this->useContainer = TRUE;
		this->nJpegQuality = GMXTilingParameters::DEFAULT_JPEG_QUALITY;
		this->dShiftX = 0;
		this->dShiftY = 0;
		this->poTileName = NULL;
		this->pBackgroundColor	= NULL;
		this->pTransparentColor = NULL;
		this->pNoDataValue		= NULL;
		this->baseZoom	= 0;
		this->minZoom	= 0;
		this->maxTilesInCache	= 0;
		this->bAutoStretchTo8Bit = FALSE;
	}

		
	~GMXTilingParameters ()
	{
		delete(pBackgroundColor);
		delete(pTransparentColor);
		delete(pNoDataValue);		
	}
};

BOOL GMXMakeTiling (	GMXTilingParameters		*poParams);


BOOL GMXMakeZoomFromBundle (GMXTilingParameters				*poParams, 
							GMX::BundleOfRasterFiles		*poBundle, 
							int								nExpected, 
							GMX::ITilePyramid				*tileContainer);



BOOL GMXMakePyramidFromBaseZoom (	GMX::VectorBorder	&oVectorBorder, 
								int					nBaseZoom, 
								int					nMinZoom, 
								GMXTilingParameters	*poParams, 
								int					&nExpectedTiles, 
								int					&nGeneratedTiles, 
								BOOL				bOnlyCalculate, 
								GMX::ITilePyramid	*ITilePyramid,
								int					nJpegQuality	= 80
								);


struct GMXTilingFromBufferParams
{
	GMXTilingParameters				*poTilingParams;
	GMX::RasterBuffer				*poBuffer;
	GMX::BundleOfRasterFiles		*poBundle; 
	int								nULx; 
	int								nULy;
	int								z;
	int								nTilesExpected; 
	int								*pnTilesGenerated;
	GMX::ITilePyramid				*poTilePyramid;
	BOOL (*pfCleanAfterTiling)(GMX::RasterBuffer*poBuffer);
};

BOOL GMXPrintTilingProgress (int nExpectedTiles, int nGeneratedTiles);



BOOL GMXTilingFromBuffer	(	GMXTilingParameters				*poTilingParams,
								GMX::RasterBuffer				*poBuffer, 
								GMX::BundleOfRasterFiles		*poBundle, 
								int								nULx, 
								int								nULy,
								int								z,
								int								nTilesExpected, 
								int								*pnTilesGenerated,
								GMX::ITilePyramid				*potilePyramid
								);

int GMXCalcTilesAtZoomForBundle (GMXTilingParameters						*poParams, 
									GMX::BundleOfRasterFiles	*poBundle, 
									int							&nAllTiles, 
									list<string>				&tilesList );


DWORD WINAPI GMXCallTilingFromBuffer( LPVOID lpParam);

BOOL GMXCleanAfterTilingFromBufer (GMX::RasterBuffer				*poBuffer);


HANDLE GMXAsyncTilingFromBuffer	(GMXTilingParameters			*poTilingParams,
								GMX::RasterBuffer				*poBuffer, 
								GMX::BundleOfRasterFiles		*poBundle, 
								int								nULx, 
								int								nULy,
								int								z,
								int								nTilesExpected, 
								int								*pnTilesGenerated,
								GMX::ITilePyramid				*potilePyramid,
								unsigned long					&threadId);
	

BOOL GMXMakeZoomOutTile (	GMX::VectorBorder				&oVectorBorder,
								int								nCurrZoom,
								int								nX,
								int								nY,
								int								nBaseZoom,
								int								nMinZoom,
								GMXTilingParameters				*poParams,
								GMX::RasterBuffer				&oBuffer, 
								int								&nExpectedTiles,
								int								&nGeneratedTiles,
								BOOL							bOnlyCalculate,
								GMX::ITilePyramid				*ITilePyramid,
								int								nJpegQuality = 80);


BOOL GMXZoomOutTileBuffer		(	GMX::RasterBuffer				srcQuarterTile[4], 
								BOOL							quarterTileExists[4], 
								GMX::RasterBuffer				&zoomOutTileBuffer); 


int GMXCalcTilesAtZoomForImage (	string						strImage, 
									string						vectorFile, 
									GMXTilingParameters			*poParams, 
									GMX::BundleOfRasterFiles	*poBundle, 
									list<string>				&tilesList);





//#endif