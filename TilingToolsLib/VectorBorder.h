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
	static bool					AdjustFor180DegreeIntersection		(OGRGeometry		*p_ogr_geom_merc);
	static OGREnvelope			CombineOGREnvelopes					(OGREnvelope	&envp1, OGREnvelope	&envp2);
	static OGREnvelope			InetersectOGREnvelopes				(OGREnvelope	&envp1, OGREnvelope	&envp2);
	static OGRLinearRing**		GetLinearRingsRef					(OGRGeometry	*p_ogr_geom, int &num_rings);
	static bool					Intersects180Degree (OGRGeometry	*p_ogr_geom, OGRSpatialReference *p_ogr_sr);
  static bool         CalcIntersectionBetweenLineAndPixelLineGeometry (int y_line, OGRGeometry *po_ogr_geom, int &num_points, int *&x);
  //static bool         ConvertOGRGeometryToArrayOfSegments (OGRGeometry *p_ogr_geom, int &num_segments, OGRLineString **pp_ls);

	VectorBorder	();
	VectorBorder	(OGREnvelope merc_envp, MercatorProjType	merc_type);
	~VectorBorder	();
	

public:
	//bool						initByMercEnvelope (OGREnvelope envelope);
	bool						Intersects(OGREnvelope &envelope);
	bool						Intersects(int tile_z, int tile_x, int tile_y);
	OGRGeometry*				get_ogr_geometry_ref();
	OGRGeometry*				GetOGRGeometryTransformed (OGRSpatialReference *poOutputSRS);
	OGRGeometry*				GetOGRGeometryTransformedToPixelLine(OGRSpatialReference *poRasterSRS, double *rasterGeoTransform);
	OGREnvelope					GetEnvelope ();


protected:
	OGRGeometry					*p_ogr_geometry_;
	MercatorProjType			merc_type_;


protected:
	static OGRMultiPolygon*		ReadMultiPolygonFromOGRDataSource	(OGRDataSource		*p_ogr_ds); 
	

};


}
#endif