#pragma once
//#ifndef IMAGETILLING_RasterFile_H
//#define IMAGETILLING_RasterFile_H

#include "stdafx.h"
#include "RasterBuffer.h"
#include "TileName.h"
#include "VectorBorder.h"
using namespace std;


namespace GMT
{


class RasterFile
{

public:
	BOOL			init(wstring strRasterFile, BOOL isGeoReferenced, double dShiftX=0.0, double dShiftY=0.0 );
	BOOL			close();

	RasterFile();
	RasterFile(wstring strRasterFile, BOOL isGeoReferenced = TRUE);
	~RasterFile(void);

	void			getPixelSize (int &nWidth, int &nHeight);
	void			getGeoReference (double &dULx, double &dULy, double &dRes);
	double			getResolution();

	BOOL			getSpatialRef(OGRSpatialReference	&oSRS); 
	BOOL			getDefaultSpatialRef (OGRSpatialReference	&oSRS, MercatorProjType mercType);

	GDALDataset*	getGDALDatasetRef();

	

public: 
	void			setGeoReference	(double dResolution, double dULx, double dULy);
	BOOL			getMinMaxPixelValues (double &minVal, double &maxVal);
	BOOL			getNoDataValue (int *pNoDataValue);


public:


	void			readMetaData ();
	OGREnvelope		GetEnvelope ();
	OGREnvelope		getMercatorEnvelope (MercatorProjType	mercType);

	static			BOOL readSpatialRefFromMapinfoTabFile (wstring tabFilePath, OGRSpatialReference *poSRS);


protected:
	void			delete_all();

protected:
	_TCHAR buf[256];
	wstring m_strRasterFile;
	GDALDataset  *m_poDataset;
	BOOL	m_isGeoReferenced;
	int		m_nWidth;
	int		m_nHeight;
	double	m_dResolution;
	double	m_dULx;
	double	m_dULy;
	int		m_nNoDataValue;
	BOOL	m_bNoDataValueDefined;
	int		m_nBands;
	GDALDataType m_oGDALDataType;

};





int _stdcall GMTPrintNoProgress ( double, const char*,void*);

class BundleOfRasterFiles
{
public:
	BundleOfRasterFiles(void);
	~BundleOfRasterFiles(void);
	void close_all();

public:
	
	int				init	(wstring inputPath, MercatorProjType mercType, wstring vectorFile=L"", double dShiftX = 0.0, double dShiftY = 0.0);
	//wstring			BestImage(double min_x, double min_y, double max_x, double max_y, double &max_intersection);

	OGREnvelope		getMercatorEnvelope();
	int				calculateNumberOfTiles (int zoom);
	int				calculateBestMercZoom();
	BOOL			warpToMercBuffer (int zoom,	OGREnvelope	oMercEnvelope, RasterBuffer &oBuffer, int *pNoDataValue = NULL, BYTE *pDefaultColor = NULL);

	list<wstring>	GetFilesList();
	list<wstring>	getFilesListByEnvelope(OGREnvelope mercatorEnvelope);
	BOOL			intersects(OGREnvelope mercatorEnvelope);



	//BOOL			createBundleBorder (VectorBorder &border);	
	int ImagesCount();

protected:

	BOOL			addItemToBundle (wstring rasterFile, wstring	vectorFile, double dShiftX = 0.0, double dShiftY = 0.0);


protected:
	list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>	dataList;
	MercatorProjType		mercType;
};


}