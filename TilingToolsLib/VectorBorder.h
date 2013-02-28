#pragma once
#ifndef		GMXVECTORBORDER_H
#define		GMXVECTORBORDER_H 
#include "stdafx.h"
#include "TileName.h"

namespace GMX
{


class VectorBorder
{
public:
	static VectorBorder*		createFromVectorFile				(string vectorFilePath, MercatorProjType	mercType);
	static string				getVectorFileNameByRasterFileName	(string strRasterFile);
	static OGRPolygon*			createOGRPolygonByOGREnvelope		(OGREnvelope envelope);
	static BOOL					adjustFor180DegreeIntersection		(OGRGeometry		*poMercGeometry);
	static OGREnvelope			combineOGREnvelopes					(OGREnvelope	&oEnvelope1, OGREnvelope	&oEnvelope2);
	static OGREnvelope			inetersectOGREnvelopes				(OGREnvelope	&oEnvelope1, OGREnvelope	&oEnvelope2);
	static OGRLinearRing**		getLinearRingsRef					(OGRGeometry	*poGeometry, int &numRings);
	static BOOL						intersects180Degree (OGRGeometry	*poGeometry, OGRSpatialReference *poSR);

	VectorBorder	();
	VectorBorder	(OGREnvelope mercEnvelope, MercatorProjType	mercType);
	~VectorBorder	();
	

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