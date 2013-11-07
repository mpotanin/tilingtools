#pragma once
#ifndef		gmxVECTORBORDER_H
#define		gmxVECTORBORDER_H 
#include "stdafx.h"
#include "TileName.h"

namespace gmx
{


class VectorBorder
{
public:
	static VectorBorder*		CreateFromVectorFile				(string vector_file, MercatorProjType	merc_type);
	static string				GetVectorFileNameByRasterFileName	(string raster_file);
	static OGRPolygon*			CreateOGRPolygonByOGREnvelope		(OGREnvelope &envelope);
	static BOOL					AdjustFor180DegreeIntersection		(OGRGeometry		*p_ogr_geom_merc);
	static OGREnvelope			CombineOGREnvelopes					(OGREnvelope	&envp1, OGREnvelope	&envp2);
	static OGREnvelope			InetersectOGREnvelopes				(OGREnvelope	&envp1, OGREnvelope	&envp2);
	static OGRLinearRing**		GetLinearRingsRef					(OGRGeometry	*p_ogr_geom, int &num_rings);
	static BOOL					Intersects180Degree (OGRGeometry	*p_ogr_geom, OGRSpatialReference *p_ogr_sr);

	VectorBorder	();
	VectorBorder	(OGREnvelope merc_envp, MercatorProjType	merc_type);
	~VectorBorder	();
	

public:
	//BOOL						initByMercEnvelope (OGREnvelope envelope);
	BOOL						Intersects(OGREnvelope &envelope);
	BOOL						Intersects(int tile_z, int tile_x, int tile_y);
	OGRGeometry*				get_ogr_geometry_ref();
	OGRGeometry*				GetOGRGeometryTransformed (OGRSpatialReference *poOutputSRS);
	OGRPolygon*					GetOGRPolygonTransformedToPixelLine(OGRSpatialReference *poOutputSRS, double *geoTransform);
	OGREnvelope					GetEnvelope ();


protected:
	OGRGeometry					*p_ogr_geometry_;
	MercatorProjType			merc_type_;


protected:
	static OGRMultiPolygon*		ReadMultiPolygonFromOGRDataSource	(OGRDataSource		*p_ogr_ds); 
	

};


}
#endif