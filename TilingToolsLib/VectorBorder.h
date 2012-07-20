#pragma once

#ifndef		VectorBorder_H
#define		VectorBorder_H 
#include "stdafx.h"

class VectorBorder
{
public:

	VectorBorder(OGREnvelope	&oEnvelope);
	VectorBorder(VectorBorder	*poBorder);
	VectorBorder(OGRGeometry	*poGeometry);
	VectorBorder(void);
	
	VectorBorder& operator=(VectorBorder &border)
	{
		InitByOGRGeometry(border.GetOGRGeometry());
		return *this;
	}
	
	OGREnvelope GetEnvelope ();
	
	BOOL		IsInitialized(); 

	BOOL		InitByPoints (int nNumOfPoints, double *dblPoints);
	BOOL		InitByEnvelope (OGREnvelope oEnvelope);
	BOOL		InitByOGRGeometry (OGRGeometry *poGeometry_set);


	BOOL		GetAllPoints (int &num, int* &nums, double** &arr);

	
		
	BOOL AdjustBounds (	double *min_x, double *min_y, double *max_x, double *max_y);
	
	BOOL AdjustBounds (	OGREnvelope &oEnvelope);
	
	int Contains(double min_x, double min_y, double max_x, double max_y);

	int Intersects(double min_x, double min_y, double max_x, double max_y);

	int OnEdge(double min_x, double min_y, double max_x, double max_y);

	int Contains(OGREnvelope &oEnvelope);
	int Intersects(OGREnvelope &oEnvelope);
	int OnEdge(OGREnvelope &oEnvelope);

	double	GetArea();
	BOOL	Intersection (OGREnvelope &oEnvelope, VectorBorder &oPolygon);

	double* CalcHorizontalLineIntersection (double y, int &nNumOfPoints);


	BOOL		GetOGRPolygons	(int &num,	OGRPolygon**	&poPolygons);
	BOOL		GetOGRRings		(int &num,	OGRLinearRing** &poRings);


	~VectorBorder(void);

	OGRGeometry*	GetOGRGeometry();

	BOOL Buffer (double dfDist);

protected:
	BOOL BufferOfPolygon  (double dfDist, OGRPolygon *poPolygon);
	double distance (OGRPoint oP1, OGRPoint oP2);

	void delete_all();

	void makePolygon (int n, double *arr,OGRPolygon	*poPolygon);

	void makePolygon (double min_x, double min_y, double max_x, double max_y, OGRPolygon	*poPolygon);

protected:
	OGRGeometry			*m_poGeometry;
};

#endif