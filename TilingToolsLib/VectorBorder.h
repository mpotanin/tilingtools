#pragma once

#ifndef		VectorBorder_H
#define		VectorBorder_H 
#include "stdafx.h"
#include "GeometryFuncs.h"
#include "str.h"
#include "TileName.h"

class VectorBorder
{
public:
	static VectorBorder*		createFromVectorFile				(wstring vectorFilePath, MercatorProjType	mercType);
	static wstring				getVectorFileNameByRasterFileName	(wstring strRasterFile);
	static OGRPolygon*			createOGRPolygonByOGREnvelope		(OGREnvelope envelope);
	static BOOL					adjustFor180DegreeIntersection		(OGRGeometry		*poMercGeometry);


	VectorBorder();
	VectorBorder(OGREnvelope mercEnvelope, MercatorProjType	mercType);
	~VectorBorder();
	

public:
	//BOOL						initByMercEnvelope (OGREnvelope envelope);
	BOOL						intersects(OGREnvelope &envelope);
	BOOL						intersects(int tile_z, int tile_x, int tile_y);
	OGRGeometry*				getOGRGeometryRef();
	OGRGeometry*				getOGRGeometryTransformed (OGRSpatialReference *poOutputSRS);
	OGREnvelope					getEnvelope ();


protected:
	OGRGeometry					*m_poGeometry;
	MercatorProjType			mercType;


protected:
	static OGRMultiPolygon*		readMultiPolygonFromOGRDataSource	(OGRDataSource		*poDS); 
	

};

#endif



/*
public:
	int						Intersects(double min_x, double min_y, double max_x, double max_y);
	VectorBorder(OGREnvelope	&oEnvelope);
	VectorBorder(VectorBorder	*poBorder);
	VectorBorder(OGRGeometry	*poGeometry);
	VectorBorder(void);
	
	VectorBorder& operator=(VectorBorder &border)
	{
		InitByOGRGeometry(border.getOGRGeometryRef());
		return *this;
	}
	
	OGREnvelope GetEnvelope ();
	
	
	BOOL		InitByEnvelope (OGREnvelope oEnvelope);
	BOOL		InitByOGRGeometry (OGRGeometry *poGeometry_set);


	BOOL		GetAllPoints (int &num, int* &nums, double** &arr);

	
		
	BOOL AdjustBounds (	double *min_x, double *min_y, double *max_x, double *max_y);
	
	BOOL AdjustBounds (	OGREnvelope &oEnvelope);
	
	int Contains(double min_x, double min_y, double max_x, double max_y);

	int OnEdge(double min_x, double min_y, double max_x, double max_y);

	int Contains(OGREnvelope &oEnvelope);
	int OnEdge(OGREnvelope &oEnvelope);

	double	GetArea();
	BOOL	Intersection (OGREnvelope &oEnvelope, VectorBorder &oPolygon);

	double* CalcHorizontalLineIntersection (double y, int &nNumOfPoints);


	BOOL		GetOGRPolygons	(int &num,	OGRPolygon**	&poPolygons);
	BOOL		GetOGRRings		(int &num,	OGRLinearRing** &poRings);


	~VectorBorder(void);


	BOOL Buffer (double dfDist);

protected:
	BOOL BufferOfPolygon  (double dfDist, OGRPolygon *poPolygon);
	double distance (OGRPoint oP1, OGRPoint oP2);

	void delete_all();

	void makePolygon (int n, double *arr,OGRPolygon	*poPolygon);

	void makePolygon (double min_x, double min_y, double max_x, double max_y, OGRPolygon	*poPolygon);
*/