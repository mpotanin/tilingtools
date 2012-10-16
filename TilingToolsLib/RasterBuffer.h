#pragma once
#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H
#include "stdafx.h"
#include "VectorFile.h"


class RasterBuffer
{
public:
	RasterBuffer(void);
	~RasterBuffer(void);

	void	clearBuffer();

	BOOL			createBuffer	(int			nBands,
									 int			nXSize,
									 int			nYSize,
									 void			*pDataSrc				= NULL,
									 GDALDataType	dataType				= GDT_Byte,
									 int			*pNoDataValue			= NULL,
									 BOOL			bAlphaBand				= FALSE	
									 );

	BOOL			createBuffer				(RasterBuffer *pSrcBuffer);
	BOOL			createBufferFromJpegData	(void *pDataSrc, int size);
	BOOL			createBufferFromPngData		(void *pDataSrc, int size);
	BOOL			createBufferFromTiffData	(void *pDataSrc, int size);

	BOOL			setNoDataValue(int noDataValue = 0);
	int*			getNoDataValue		();


	BOOL			SaveToPngData	(void* &pDataDst, int &size);
	BOOL			SaveToPng24Data	(void* &pDataDst, int &size);
	BOOL			SaveToJpegData	(int quality, void* &pDataDst, int &size);
	BOOL			SaveToTiffData	(void* &pDataDst, int &size);

	BOOL			isAnyNoDataPixel			();

	BOOL			SaveBufferToFile		(wstring fileName, int quality = 80);
	BOOL			SaveBufferToFileAndData	(wstring fileName, void* &pDataDest, int &size, int quality = 80);

	//BOOL			ResizeAndConvertToRGB	(int nNewWidth, int nNewHeight);
	//BOOL			MergeUsingBlack (RasterBuffer oBackGround, RasterBuffer &oMerged);

	//BOOL			makeZero(LONG nLeft, LONG nTop, LONG nWidth, LONG nHeight, LONG nNoDataValue = 0);
	BOOL			initByRGBColor	 (BYTE rgb[3]);
	BOOL			initByValue(int value = 0);	
	BOOL			initByNoDataValue(int noDataValue = 0);

	void*			copyData	(int left, int top, int w, int h, BOOL stretchTo8Bit = FALSE, double minVal = 0, double maxVal = 0);
	BOOL			setData	(int left, int top, int w, int h, void *pBlockData, int bands = 0);
	void*			getDataZoomedOut	();	
	BOOL			convertFromIndexToRGB ();
	BOOL			convertFromPanToRGB();
	BOOL			createAlphaBandByColor(BYTE	*pRGB);
	BOOL			isAlphaBand();
	//BOOL			createAlphaBandByValue(int	value);

public:
	BOOL			stretchDataTo8Bit(double minVal, double maxVal);
	void*			getDataRef();
	int				getBandsCount();
	int				getXSize();
	int				getYSize();
	GDALDataType	getDataType();
	GDALColorTable*	getColorMeta ();
	BOOL			setColorMeta (GDALColorTable *pTable);

protected:
	//void									initAlphaBand();
	template <typename T>	BOOL			isAnyNoDataPixel	(T type);
	template <typename T>	BOOL			initByValue	(T type, int value);
	template <typename T>	void*			copyData	(T type, int left, int top, int w, int h,  BOOL stretchTo8Bit = FALSE, double minVal = 0, double maxVal = 0);
	template <typename T>	BOOL			setData	(T type, int left, int top, int w, int h, void *pBlockData, int bands = 0);
	template <typename T>	void*			getDataZoomedOut	(T type);

	BOOL			bAlphaBand;
	void			*pData;
	GDALDataType	dataType;
	int				dataSize;
	 
	
	int				nBands;
	int				nXSize;
	int				nYSize;

protected:
	GDALColorTable	*pTable;

protected:
	int				*pNoDataValue;			
	


};
#endif