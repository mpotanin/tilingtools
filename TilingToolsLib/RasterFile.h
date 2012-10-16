#pragma once
//#ifndef IMAGETILLING_RasterFile_H
//#define IMAGETILLING_RasterFile_H

#include "stdafx.h"
#include "RasterBuffer.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"
#include "VectorFile.h"
#include "TileName.h"
//#include "VectorBorder.h"
using namespace std;



class RasterFile
{

public:
	BOOL init(wstring strRasterFile, BOOL isGeoReferenced, double dShiftX=0.0, double dShiftY=0.0 );
	BOOL close();

	RasterFile();
	RasterFile(wstring strRasterFile, BOOL isGeoReferenced = TRUE);
	~RasterFile(void);

	void	GetPixelSize (int &nWidth, int &nHeight);
	void	GetGeoReference (double &dULx, double &dULy, double &dRes);
	double	GetResolution();

	BOOL	getSpatialRef(OGRSpatialReference	&oSRS); 
	BOOL	getDefaultSpatialRef (OGRSpatialReference	&oSRS, MercatorProjType mercType);

	GDALDataset*	getDataset();

	//static	BOOL createTifFileInMercatorProjection (wstring strFileName, int width, int height, int bands, double ulx, double uly, double res);


public: 
	void	setGeoReference	(double dResolution, double dULx, double dULy);
	//void adjustGeoReference (double dResolution);


	BOOL	getMinMaxPixelValues (double &minVal, double &maxVal);

	BOOL	calcFromProjectionToPixels (double dblX, double dblY, int *nX, int *nY);

	BOOL	calcFromPixelsToProjection (int nX, int nY, double *dblX, double *dblY);

	
	BOOL	getToBuffer			(RasterBuffer &oImageBuffer,
								int nLeft,
								int nTop,
								int nWidth,
								int nHeight
								);

	BOOL	getToBuffer			(RasterBuffer &oImageBuffer,
								double min_x,
								double max_y,
								double max_x,
								double min_y
								);
	BOOL	getNoDataValue (int *pNoDataValue);


protected:
	_TCHAR buf[256];
	void delete_all();

/*
	

	BOOL GenerateImage		(RasterBuffer &oImageBuffer,
							wstring	strRasterFile,
							wstring	strImageFormat);
*/
public:
	/*
	BOOL cutAreaUsingVectorFile	(wstring strVectorFile,
								wstring  strRasterFile,
								wstring  strImageFormat,
								BOOL	  bGenerateWldFile);

	BOOL cutAreaUsingMeters (int	nPoints, 
							double	*poPointsArray,
							wstring  strRasterFile,
							wstring  strImageFormat,
							BOOL	  bGenerateWldFile);			
	
	BOOL cutAreaUsingPixels (int	nPoints, 
							int		*poPointsArray,
							wstring  strImageFormat,
							wstring  strAreaImage,
							BOOL	  bGenerateWldFile);

	BOOL cutToBufferByVectorBorder  (VectorBorder		&oVectorBorder,
									RasterBuffer			&oBuffer,
									double _min_x			= 0,
									double _min_y			= 0,
									double _max_x			= 0,
									double _max_y			= 0,
									int ZeroColor			= 0
									);

	BOOL MergeTilesByVectorBorder	(RasterFile				&oBackGroundImage,
									VectorBorder		&oVectorBorder,
									double					min_x,
									double					min_y,
									double					max_x,
									double					max_y,
									RasterBuffer			&oBuffer
									);
	
	*/

	void readMetaData ();
	OGREnvelope GetEnvelope ();
	OGREnvelope	getEnvelopeInMercator (MercatorProjType	mercType);


	BOOL		setBackgroundColor (int rgb[3]);

protected:
	
	BOOL readGeoReferenceFromWLD (double dShiftX=0.0, double dShiftY=0.0);

	BOOL writeWLDFile (wstring strFileName, double dULx, double dULy, double dRes);




public:
	
	const _TCHAR*  FORMAT_JPEG;
	const _TCHAR* FORMAT_GIF;
	const _TCHAR* FORMAT_PNG;
	const _TCHAR* FORMAT_GTIFF;
	const _TCHAR* FORMAT_BMP;
	const _TCHAR* FORMAT_HFA;
	

protected:
	wstring m_strRasterFile;
	GDALDataset  *m_poDataset;
	int m_nBands;
	//wstring m_strImageFormat;
	
	BOOL	m_isGeoReferenced;
	int		m_nWidth;
	int		m_nHeight;
	double	m_dResolution;
	double	m_dULx;
	double	m_dULy;
	int		m_nNoDataValue;
	BOOL	m_bNoDataValueDefined;
};

int _stdcall PrintNoProgress ( double, const char*,void*);

class BundleOfRasterFiles
{
public:
	BundleOfRasterFiles(void);
	~BundleOfRasterFiles(void);
	void close_all();

public:
	
	int				InitFromFilesList(list<wstring> filesList, MercatorProjType mercType, double dShiftX = 0.0, double dShiftY = 0.0);
	int				InitFromFolder(wstring folderPath, wstring rasterType, MercatorProjType mercType, double dShiftX = 0.0, double dShiftY = 0.0);
	int				InitFromRasterFile (wstring strRasterFile, MercatorProjType mercType, wstring vectorFile=L"", double dShiftX = 0.0, double dShiftY = 0.0);
	//wstring			BestImage(double min_x, double min_y, double max_x, double max_y, double &max_intersection);

	OGREnvelope		getMercatorEnvelope();
	int				calculateNumberOfTiles (int zoom);
	int				calculateBestMercZoom();
	BOOL			warpMercToBuffer (int zoom,	OGREnvelope	oMercEnvelope, RasterBuffer &oBuffer, int *pNoDataValue = NULL, BYTE *pDefaultColor = NULL);

	list<wstring>	GetFilesList();
	list<wstring>	getFilesListByEnvelope(OGREnvelope mercatorEnvelope);
	BOOL			intersects(OGREnvelope mercatorEnvelope);



	BOOL			createBundleBorder (VectorBorder &border);	
	int ImagesCount();

protected:

	BOOL			addItemToBundle (wstring rasterFile, wstring	vectorFile, double dShiftX = 0.0, double dShiftY = 0.0);


protected:
	list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>	dataList;
	MercatorProjType		mercType;
};

//#endif