#pragma once

#ifndef		VectorFile_H
#define		VectorFile_H 
#include	"stdafx.h"
#include	"VectorBorder.h"
#include	"FileSystemFuncs.h"
#include	"GeometryFuncs.h"
#include	"StringFuncs.h"
#include	"TileName.h"


class VectorFile
{
public:
	VectorFile(void);
	
	
	BOOL	Open (wstring	strVectorFile);
	
	//static BOOL CalcRectEnvelope(OGRGeometry	*poGeometry, double *min_x, double *min_y,double *max_x, double *max_y);
	
	/*
	static BOOL CreateVectorFileByPoints  (int nNumOfPoints, 
											double *dblArray,
											wstring	strFeatureType,
											wstring	strFeatureData,
											wstring	strVectorFile
											);
	*/

	static	BOOL	CreateMercatorVectorFileByGeometryArray(OGRGeometry **poGeometry, int num, wstring fileName, MercatorProjType mercType);

	static	wstring	GetVectorFileByRasterFileName				(wstring strRasterFile);
	static	BOOL	AdjustFor180DegreeIntersection (OGRGeometry	*poGeometry);
	static	BOOL	deleteVectorFile(wstring fileName);
	static	BOOL	OpenAndCreatePolygonInMercator (wstring strVectorFile, VectorBorder &destPolygon, MercatorProjType mercType);
	static	BOOL	transformOGRPolygonToMerc(OGRGeometry *poSrcPolygon, VectorBorder &destPolygon, MercatorProjType mercType, OGRSpatialReference	*poSrcSpatial);
	static BOOL		CreateTileGridFile (wstring shapeFileName, int zoom, MercatorProjType projType);  
	
	~VectorFile(void);
	BOOL				GetVectorBorder(VectorBorder	&oPolygon);
protected:


	OGRGeometry*	 GetOGRGeometry ();
	void delete_all();
	BOOL readPolygonAndTransformToMerc (VectorBorder &oDestPolygon, MercatorProjType mercType);
	
protected:
	OGRDataSource       *m_poDS;
};

#endif