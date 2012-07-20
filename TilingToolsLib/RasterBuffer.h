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

	BOOL			createBuffer	(int nBands_set,
									 int nBufferXSize_set,
									 int nBufferYSize_set,
									 void *pData_set = NULL,
									 GDALDataType	dataType = GDT_Byte
									 );


	BOOL			createBuffer				(RasterBuffer *pBuffer);
	BOOL			createBufferFromJpegData	(void *pDataSrc, int size);
	BOOL			createBufferFromPngData		(void *pDataSrc, int size);
	BOOL			createBufferFromTiffData	(void *pDataSrc, int size);

	BOOL			setBackgroundColor(int rgb[3]);

	BOOL			SaveToPngData	(void* &pDataDst, int &size);
	BOOL			SaveToPng24Data	(void* &pDataDst, int &size);
	BOOL			SaveToJpegData	(int quality, void* &pDataDst, int &size);
	BOOL			SaveToTiffData	(void* &pDataDst, int &size);

	BOOL			SaveBufferToFile		(wstring fileName, int quality = 80);
	BOOL			SaveBufferToFileAndData	(wstring fileName, void* &pDataDest, int &size, int quality = 80);

	//BOOL			ResizeAndConvertToRGB	(int nNewWidth, int nNewHeight);
	//BOOL			MergeUsingBlack (RasterBuffer oBackGround, RasterBuffer &oMerged);

	BOOL			createAlphaBand(int *rgb);

	//BOOL			makeZero(LONG nLeft, LONG nTop, LONG nWidth, LONG nHeight, LONG nNoDataValue = 0);
	BOOL			initByNoDataValue(int nNoDataValue = 0);	
	BOOL			initByBackgroundColor();
	
	void*			getBlockFromBuffer	(int left, int top, int w, int h, BOOL stretchTo8Bit = FALSE, double minVal = 0, double maxVal = 0);
	
	BOOL			writeBlockToBuffer	(int left, int top, int w, int h, void *pBlockData, int bands = 0);
	/*
	BOOL			dataIO	(BOOL operationFlag, int left, int top, int w, int h, void *pData, 
							int bands = 0, BOOL stretchTo8Bit = FALSE, double min = 0, double max = 0);
	*/

	BOOL			createSimpleZoomOut	(RasterBuffer &oBufferDst);	

		
	BOOL			convertFromIndexToRGB ();
	BOOL			convertFromPanToRGB();

public:
	BOOL			stretchDataTo8Bit(double minVal, double maxVal);
	void*			getBufferData();
	int				getBandsCount();
	int				getBufferXSize();
	int				getBufferYSize();
	GDALDataType	getBufferDataType();
	GDALColorTable*	getColorMeta ();
	BOOL			setColorMeta (GDALColorTable *pTable);

protected:
	template <typename T>	BOOL			initByNoDataValue	(T type, int noDataValue);
	template <typename T>	void*			getBlockFromBuffer	(T type, int left, int top, int w, int h,  BOOL stretchTo8Bit = FALSE, double minVal = 0, double maxVal = 0);
	template <typename T>	BOOL			writeBlockToBuffer	(T type, int left, int top, int w, int h, void *pBlockData, int bands = 0);
	template <typename T>	BOOL			createSimpleZoomOut	(T type, void* &pDataDst);


	void			*pData;
	GDALDataType	dataType;
	int				dataSize;
	 
	
	int nBands;
	int nBufferXSize;
	int nBufferYSize;

public:
	static const int ZeroColor = 125;

protected:
	GDALColorTable	*pTable;

protected:
	int backgroundColor[3];
	


};
#endif