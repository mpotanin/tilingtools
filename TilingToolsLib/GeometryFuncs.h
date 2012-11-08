#ifndef IMAGE_TILLING_GEOMETRY_FUNCS_H
#define IMAGE_TILLING_GEOMETRY_FUNCS_H
#include "stdafx.h"

//BOOL		CalcRectEnvelope	(OGRGeometry	*poGeometry, double *min_x, double *min_y,double *max_x, double *max_y);

typedef enum { 
	WORLD_MERCATOR=0,                         
	WEB_MERCATOR=1
} MercatorProjType;


OGREnvelope	CombineEnvelopes			(OGREnvelope	&oEnvelope1,OGREnvelope	&oEnvelope2);
OGREnvelope	InetersectEnvelopes			(OGREnvelope	&oEnvelope1,OGREnvelope	&oEnvelope2);
double		CalcAreaOfEnvelope			(OGREnvelope	oEnvelope);
OGREnvelope	CreateEnvelope				(double minx,double miny,double maxx,double maxy);


#endif
