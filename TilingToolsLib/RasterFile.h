#pragma once
//#ifndef IMAGETILLING_RasterFile_H
//#define IMAGETILLING_RasterFile_H

#include "stdafx.h"
#include "RasterBuffer.h"
#include "TileName.h"
#include "VectorBorder.h"
using namespace std;


namespace GMX
{


class RasterFile
{

public:
	BOOL			init(string strRasterFile, BOOL isGeoReferenced, double dShiftX=0.0, double dShiftY=0.0 );
	BOOL			close();

	RasterFile();
	RasterFile(string strRasterFile, BOOL isGeoReferenced = TRUE);
	~RasterFile(void);

	void			getPixelSize (int &nWidth, int &nHeight);
	void			getGeoReference (double &dULx, double &dULy, double &dRes);
	double			getResolution();

	BOOL			getSpatialRef(OGRSpatialReference	&oSRS); 
	BOOL			getDefaultSpatialRef (OGRSpatialReference	&oSRS, MercatorProjType mercType);

	GDALDataset*	getGDALDatasetRef();

	

public: 
	void			setGeoReference		(double dResolution, double dULx, double dULy);
	BOOL			computeStatistics	(int &bands, double *&min, double *&max, double *&mean, double *&stdDev);
	BOOL			getNoDataValue		(int *pNoDataValue);


public:


	void			readMetaData ();
	OGREnvelope		GetEnvelope ();
	OGREnvelope		getMercatorEnvelope (MercatorProjType	mercType);

	static			BOOL readSpatialRefFromMapinfoTabFile (string tabFilePath, OGRSpatialReference *poSRS);


protected:
	void			delete_all();

protected:
	_TCHAR buf[256];
	string m_strRasterFile;
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





int _stdcall GMXPrintNoProgress ( double, const char*,void*);

class BundleOfRasterFiles
{
public:
	BundleOfRasterFiles(void);
	~BundleOfRasterFiles(void);
	void close_all();

public:
	
	int				init	(string inputPath, MercatorProjType mercType, string vectorFile="", 
							double dShiftX = 0.0, double dShiftY = 0.0);
	//string			BestImage(double min_x, double min_y, double max_x, double max_y, double &max_intersection);

	OGREnvelope		getMercatorEnvelope();
	int				calculateNumberOfTiles (int zoom);
	int				calculateBestMercZoom();
	BOOL			warpToMercBuffer (	int zoom,	OGREnvelope	oMercEnvelope, RasterBuffer *poBuffer, 
										int *pNoDataValue = NULL, BYTE *pDefaultColor = NULL);

	list<string>	getFileList();
	list<string>	getFileListByEnvelope(OGREnvelope mercatorEnvelope);
	BOOL			intersects(OGREnvelope mercatorEnvelope);



	//BOOL			createBundleBorder (VectorBorder &border);	
protected:

	BOOL			addItemToBundle (string rasterFile, string	vectorFile, double dShiftX = 0.0, double dShiftY = 0.0);


protected:
	list<pair<string,pair<OGREnvelope,VectorBorder*>>>	dataList;
	MercatorProjType		mercType;
};


}