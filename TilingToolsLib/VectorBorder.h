#pragma once
#ifndef		GMTVECTORBORDER_H
#define		GMTVECTORBORDER_H 
#include "stdafx.h"
#include "TileName.h"

namespace GMT
{


class VectorBorder
{
public:
	static VectorBorder*		createFromVectorFile				(wstring vectorFilePath, MercatorProjType	mercType);
	static wstring				getVectorFileNameByRasterFileName	(wstring strRasterFile);
	static OGRPolygon*			createOGRPolygonByOGREnvelope		(OGREnvelope envelope);
	static BOOL					adjustFor180DegreeIntersection		(OGRGeometry		*poMercGeometry);
	static OGREnvelope			combineOGREnvelopes					(OGREnvelope	&oEnvelope1, OGREnvelope	&oEnvelope2);
	static OGREnvelope			inetersectOGREnvelopes				(OGREnvelope	&oEnvelope1, OGREnvelope	&oEnvelope2);


	VectorBorder();
	VectorBorder(OGREnvelope mercEnvelope, MercatorProjType	mercType);
	~VectorBorder();
	

public:
	//BOOL						initByMercEnvelope (OGREnvelope envelope);
	BOOL						intersects(OGREnvelope &envelope);
	BOOL						intersects(int tile_z, int tile_x, int tile_y);
	OGRGeometry*				getOGRGeometryRef();
	OGRGeometry*				getOGRGeometryTransformed (OGRSpatialReference *poOutputSRS);
	OGRPolygon*					getOGRPolygonTransformedToPixelLine(OGRSpatialReference *poOutputSRS, double *geoTransform);
	OGREnvelope					getEnvelope ();


protected:
	OGRGeometry					*m_poGeometry;
	MercatorProjType			mercType;


protected:
	static OGRMultiPolygon*		readMultiPolygonFromOGRDataSource	(OGRDataSource		*poDS); 
	

};


}
#endif