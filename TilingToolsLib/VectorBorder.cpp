#include "StdAfx.h"
#include "VectorBorder.h"
namespace gmx
{


VectorBorder::VectorBorder ()
{
	p_ogr_geometry_	= NULL;
}

VectorBorder::~VectorBorder()
{
	if (p_ogr_geometry_!=NULL) p_ogr_geometry_->empty();
  OGRGeometryFactory::destroyGeometry(p_ogr_geometry_);
	p_ogr_geometry_	= NULL;
}  


VectorBorder::VectorBorder (OGREnvelope merc_envp, MercatorProjType	merc_type)
{
	merc_type_ = merc_type;
	p_ogr_geometry_ = VectorBorder::CreateOGRPolygonByOGREnvelope(merc_envp);
}



string VectorBorder::GetVectorFileNameByRasterFileName (string raster_file)
{
	string	vector_file_base = RemoveExtension(raster_file);
	string	vector_file;
	
	if (FileExists(vector_file_base+".mif"))	vector_file = vector_file_base+".mif";
	if (FileExists(vector_file_base+".shp"))	vector_file = vector_file_base+".shp";
	if (FileExists(vector_file_base+".tab"))	vector_file = vector_file_base+".tab";
	
	if (vector_file!="")
	{
		OGRDataSource * p_ogr_ds = OGRSFDriverRegistrar::Open( vector_file.c_str(), FALSE );
		if (p_ogr_ds ==NULL) return "";
		OGRDataSource::DestroyDataSource( p_ogr_ds );
	}
	return vector_file;
}


OGREnvelope	VectorBorder::CombineOGREnvelopes (OGREnvelope	&envp1, OGREnvelope	&envp2)
{
	OGREnvelope envp;
	envp.MaxX = max(envp1.MaxX,envp2.MaxX);
	envp.MaxY = max(envp1.MaxY,envp2.MaxY);
	envp.MinX = min(envp1.MinX,envp2.MinX);
	envp.MinY = min(envp1.MinY,envp2.MinY);
	return envp;
}

OGREnvelope	VectorBorder::InetersectOGREnvelopes (OGREnvelope	&envp1,OGREnvelope	&envp2)
{
	OGREnvelope envp;
	envp.MaxX = min(envp1.MaxX,envp2.MaxX);
	envp.MaxY = min(envp1.MaxY,envp2.MaxY);
	envp.MinX = max(envp1.MinX,envp2.MinX);
	envp.MinY = max(envp1.MinY,envp2.MinY);
	return envp;
}

/*

BOOL VectorBorder::ConvertOGRGeometryToArrayOfSegments (OGRGeometry *p_ogr_geom, int &num_segments, OGRLineString **pp_ls)
{
  if (p_ogr_geom->getGeometryType()!=OGRWkb 
  OGRPolygon *p_polygons = NULL;

  return TRUE;
}
*/


BOOL VectorBorder::CalcIntersectionBetweenLineAndPixelLineGeometry (int y_line, OGRGeometry *po_ogr_geom, int &num_points, int *&x)
{
  double e = 1e-6;

  num_points = 0;
  x = NULL;
  int num_rings = 0;
  
  list<double> x_val_list;
  
  OGRLinearRing **pp_lr = GetLinearRingsRef(po_ogr_geom,num_rings);
  if (pp_lr == NULL) return NULL;
    
  double x1,x2,y1,y2;
  for (int i=0;i<num_rings;i++)
  {
    if (pp_lr[i]->getNumPoints()>=4)
    {
      for (int j=0;j<pp_lr[i]->getNumPoints()-1;j++)
      {
        x1 = pp_lr[i]->getX(j);
        x2 = pp_lr[i]->getX(j+1);
        y1 = pp_lr[i]->getY(j);
        y2 = pp_lr[i]->getY(j+1);
        if (((y_line-y1)*(y_line-y2)<=0)&&(fabs(y2-y1)>e))
          x_val_list.push_back((x2*(y1-y_line)-x1*(y2-y_line))/(y1-y2));
      }
    }
  }

  if (x_val_list.size()>0)
  {
    x_val_list.sort();
    x = new int[x_val_list.size()];
    for (list<double>::iterator iter = x_val_list.begin();iter!=x_val_list.end();iter++)
    {
      x[num_points] = (int)floor((*iter)+0.5);
      num_points++;
    }
  }

  delete[]pp_lr;
  return TRUE;
}


BOOL	VectorBorder::Intersects180Degree (OGRGeometry	*p_ogr_geom, OGRSpatialReference *p_ogr_sr)
{
	int num_rings;
	OGRLinearRing **p_rings = VectorBorder::GetLinearRingsRef(p_ogr_geom,num_rings);

	OGRSpatialReference	ogr_wgs84;
	ogr_wgs84.SetWellKnownGeogCS("WGS84");
	

	for (int i=0;i<num_rings;i++)
	{
    OGRLinearRing *p_temp_ring = (OGRLinearRing*) p_rings[i]->clone();

    p_temp_ring->assignSpatialReference(p_ogr_sr);
    OGRErr error_transform =	p_temp_ring->transformTo(&ogr_wgs84);
    for (int l=0; l<p_temp_ring->getNumPoints()-1; l++)
		{
			if(	((p_temp_ring->getX(l)>90)&&(p_temp_ring->getX(l+1)<-90)) || ((p_temp_ring->getX(l)<-90)&&(p_temp_ring->getX(l+1)>90)) ||
				((p_temp_ring->getX(l)>180.00001)&&(p_temp_ring->getX(l+1)<179.9999)) || ((p_temp_ring->getX(l)<179.9999)&&(p_temp_ring->getX(l+1)>180.00001)) || 
				((p_temp_ring->getX(l)>-179.9999)&&(p_temp_ring->getX(l+1)<-180.00001)) || ((p_temp_ring->getX(l)<-180.00001)&&(p_temp_ring->getX(l+1)>-179.9999)))
			{
        p_temp_ring->empty();
				delete[]p_rings;
				return TRUE;
			}
		}
    p_temp_ring->empty();


    /*
		for (int k=0;k<p_rings[i]->getNumPoints()-1;k++)
		{
			OGRLineString ls;


			
      for (double j=0;j<=1.0001;j+=0.1)
      {
				ls.addPoint(	p_rings[i]->getX(k)*(1-j) + p_rings[i]->getX(k+1)*j,
								p_rings[i]->getY(k)*(1-j) + p_rings[i]->getY(k+1)*j);
      }
			

			ls.assignSpatialReference(p_ogr_sr);
			//ToDo
			OGRErr error_transform =	ls.transformTo(&ogr_wgs84);
			
			
			for (int l=0;l<ls.getNumPoints()-1;l++)
			{
				double x1 = ls.getX(l);
				double y1 = ls.getY(l);
				double x2 = ls.getX(l+1);
				double y2 = ls.getY(l+1);


				if(	((ls.getX(l)>90)&&(ls.getX(l+1)<-90)) || ((ls.getX(l)<-90)&&(ls.getX(l+1)>90)) ||
					((ls.getX(l)>180.00001)&&(ls.getX(l+1)<179.9999)) || ((ls.getX(l)<179.9999)&&(ls.getX(l+1)>180.00001)) || 
					((ls.getX(l)>-179.9999)&&(ls.getX(l+1)<-180.00001)) || ((ls.getX(l)<-180.00001)&&(ls.getX(l+1)>-179.9999)))
				{
					delete[]p_rings;
					return TRUE;
				}
			}
		}
    */
	}
	
	delete[]p_rings;
	return FALSE;
}


VectorBorder*	VectorBorder::CreateFromVectorFile(string vector_file, MercatorProjType	merc_type)
{
	OGRDataSource *p_ogr_ds= OGRSFDriverRegistrar::Open( vector_file.c_str(), FALSE );
	if( p_ogr_ds == NULL ) return NULL;
	
	OGRMultiPolygon *p_ogr_multipoly = ReadMultiPolygonFromOGRDataSource(p_ogr_ds);
	if (p_ogr_multipoly == NULL) return NULL;

	OGRLayer *p_ogr_layer = p_ogr_ds->GetLayer(0);
	OGRSpatialReference *p_input_ogr_sr = p_ogr_layer->GetSpatialRef();
	char	*proj_ref = NULL;
	if (p_input_ogr_sr!=NULL)
	{
		if (OGRERR_NONE!=p_input_ogr_sr->exportToProj4(&proj_ref))
		{
			if (OGRERR_NONE!=p_input_ogr_sr->morphFromESRI()) return NULL; 
			if (OGRERR_NONE!=p_input_ogr_sr->exportToProj4(&proj_ref)) return NULL;
		}
		CPLFree(proj_ref);
	}
	else
	{
		p_input_ogr_sr = new OGRSpatialReference();
		p_input_ogr_sr->SetWellKnownGeogCS("WGS84");
	}
	

	OGRSpatialReference		ogr_sr_merc;
	MercatorTileGrid::SetMercatorSpatialReference(merc_type,&ogr_sr_merc);
	
	p_ogr_multipoly->assignSpatialReference(p_input_ogr_sr);
	BOOL	intersects180 = VectorBorder::Intersects180Degree(p_ogr_multipoly,p_input_ogr_sr);
	if (OGRERR_NONE != p_ogr_multipoly->transformTo(&ogr_sr_merc))
	{
		p_ogr_multipoly->empty();
		return NULL;
	}
	if (intersects180) AdjustFor180DegreeIntersection(p_ogr_multipoly);
		
	VectorBorder	*p_vb_res	= new VectorBorder();
	p_vb_res->merc_type_				= merc_type;
	p_vb_res->p_ogr_geometry_			= p_ogr_multipoly;
	return p_vb_res;
};


OGRMultiPolygon*		VectorBorder::ReadMultiPolygonFromOGRDataSource(OGRDataSource *p_ogr_ds)
{
	OGRLayer *p_ogr_layer = p_ogr_ds->GetLayer(0);
	if( p_ogr_layer == NULL ) return NULL;
	p_ogr_layer->ResetReading();

	OGRMultiPolygon *p_ogr_multipoly = NULL;
	BOOL	is_valid = FALSE;
	while (OGRFeature *poFeature = p_ogr_layer->GetNextFeature())
	{
    if (p_ogr_multipoly == NULL) p_ogr_multipoly = (OGRMultiPolygon*)OGRGeometryFactory::createGeometry(wkbMultiPolygon);
		if ((poFeature->GetGeometryRef()->getGeometryType() == wkbMultiPolygon) ||
			(poFeature->GetGeometryRef()->getGeometryType() == wkbPolygon))
		{
			if (OGRERR_NONE == p_ogr_multipoly->addGeometry(poFeature->GetGeometryRef()))
				is_valid = TRUE;
		}
		OGRFeature::DestroyFeature( poFeature);
	}

	if (!is_valid) return NULL;

	return p_ogr_multipoly;
};


OGRGeometry*	VectorBorder::GetOGRGeometryTransformed (OGRSpatialReference *poOutputSRS)
{
	if (this->p_ogr_geometry_ == NULL) return NULL;

	OGRSpatialReference		ogr_sr_merc;
	MercatorTileGrid::SetMercatorSpatialReference(merc_type_,&ogr_sr_merc);

	OGRMultiPolygon *poTransformedGeometry = (OGRMultiPolygon*)this->p_ogr_geometry_->clone();	
	poTransformedGeometry->assignSpatialReference(&ogr_sr_merc);
	
	if (OGRERR_NONE != poTransformedGeometry->transformTo(poOutputSRS))
	{
		poTransformedGeometry->empty();
		return NULL;
	}

	return poTransformedGeometry;
};


OGRPolygon*	VectorBorder::GetOGRPolygonTransformedToPixelLine(OGRSpatialReference *poRasterSRS, double *rasterGeoTransform)
{
	OGRGeometry *p_ogr_geom = GetOGRGeometryTransformed(poRasterSRS);

	OGRPolygon *p_ogr_poly_result = (OGRPolygon*)((OGRMultiPolygon*)p_ogr_geom)->getGeometryRef(0)->clone();
	p_ogr_geom->empty();

	OGRLinearRing	*p_ogr_ring = p_ogr_poly_result->getExteriorRing();
	p_ogr_ring->closeRings();
	
	for (int i=0;i<p_ogr_ring->getNumPoints();i++)
	{
		double d = rasterGeoTransform[1]*rasterGeoTransform[5]-rasterGeoTransform[2]*rasterGeoTransform[4];
		if ( fabs(d) < 1e-7)
		{
			p_ogr_poly_result->empty();
			return NULL;
		}

		double x = p_ogr_ring->getX(i)	-	rasterGeoTransform[0];
		double y = p_ogr_ring->getY(i)	-	rasterGeoTransform[3];
		
		double l = (rasterGeoTransform[1]*y-rasterGeoTransform[4]*x)/d;
		double p = (x - l*rasterGeoTransform[2])/rasterGeoTransform[1];

		p_ogr_ring->setPoint(i,(int)(p+0.5),(int)(l+0.5));
	}
	return p_ogr_poly_result;
}

OGRLinearRing**		VectorBorder::GetLinearRingsRef	(OGRGeometry	*p_ogr_geom, int &num_rings)
{
	num_rings = 0;
	OGRLinearRing	**pp_ogr_rings = NULL;
	OGRPolygon		**pp_ogr_poly = NULL;
	OGRwkbGeometryType type = p_ogr_geom->getGeometryType();

	if ((type!=wkbMultiPolygon) && (type!=wkbMultiPolygon) && 
		(type!=wkbLinearRing) && (type!=wkbLineString)
		) return NULL;

	int	numPolygons = 0;
	BOOL	inputIsRing = FALSE;
	if (type==wkbPolygon)
	{
		numPolygons		= 1;
		pp_ogr_poly		= new OGRPolygon*[numPolygons];
		pp_ogr_poly[0]	= (OGRPolygon*)p_ogr_geom;
	}
	else if (type==wkbMultiPolygon)
	{
		numPolygons		= ((OGRMultiPolygon*)p_ogr_geom)->getNumGeometries();
		pp_ogr_poly		= new OGRPolygon*[numPolygons];
		for (int i=0;i<numPolygons;i++)
			pp_ogr_poly[i] = (OGRPolygon*)((OGRMultiPolygon*)p_ogr_geom)->getGeometryRef(i);
	}

	if (numPolygons>0)
	{
		for (int i=0;i<numPolygons;i++)
		{
			num_rings++;
			num_rings+=pp_ogr_poly[i]->getNumInteriorRings();
		}
		pp_ogr_rings = new OGRLinearRing*[num_rings];
		int j=0;
		for (int i=0;i<numPolygons;i++)
		{
			pp_ogr_rings[j] = pp_ogr_poly[i]->getExteriorRing();
			j++;
			for (int k=0;k<pp_ogr_poly[i]->getNumInteriorRings();k++)
			{
				pp_ogr_rings[j] = pp_ogr_poly[i]->getInteriorRing(k);
				j++;
			}
		}
	}
	else
	{
		num_rings = 1;
		pp_ogr_rings = new OGRLinearRing*[num_rings];
		pp_ogr_rings[0] = (OGRLinearRing*)p_ogr_geom;
	}

	delete[]pp_ogr_poly;
	return pp_ogr_rings;
}


BOOL			VectorBorder::AdjustFor180DegreeIntersection (OGRGeometry	*p_ogr_geom_merc)
{
	OGRLinearRing	**pp_ogr_rings;
	int num_rings = 0;
	
	if (!(pp_ogr_rings = VectorBorder::GetLinearRingsRef(p_ogr_geom_merc,num_rings))) return FALSE;
	
	for (int i=0;i<num_rings;i++)
	{
		for (int k=0;k<pp_ogr_rings[i]->getNumPoints();k++)
		{
				if (pp_ogr_rings[i]->getX(k)<0)
					pp_ogr_rings[i]->setPoint(k,-2*MercatorTileGrid::ULX() + pp_ogr_rings[i]->getX(k),pp_ogr_rings[i]->getY(k));
		}
	}
	
	delete[]pp_ogr_rings;
	return TRUE;
};


BOOL	VectorBorder::Intersects(int tile_z, int tile_x, int tile_y)
{
	if (tile_x < (1<<tile_z)) return Intersects(MercatorTileGrid::CalcEnvelopeByTile(tile_z, tile_x, tile_y));
	else
	{
		if (!Intersects(MercatorTileGrid::CalcEnvelopeByTile(tile_z, tile_x, tile_y)))
			return Intersects(MercatorTileGrid::CalcEnvelopeByTile(tile_z, tile_x - (1<<tile_z), tile_y));
		else return TRUE;
	}
};


BOOL	VectorBorder::Intersects(OGREnvelope &envelope)
{
  if (p_ogr_geometry_ == NULL) return FALSE;
	OGRPolygon *p_poly_from_envp = CreateOGRPolygonByOGREnvelope (envelope);
	BOOL result = this->p_ogr_geometry_->Intersects(p_poly_from_envp);
  p_poly_from_envp->empty();
  OGRGeometryFactory::destroyGeometry(p_poly_from_envp);
 	
  return result;
}


OGREnvelope VectorBorder::GetEnvelope ()
{
	OGREnvelope envp;
	if (p_ogr_geometry_!=NULL) p_ogr_geometry_->getEnvelope(&envp);
	return envp;
};



OGRPolygon*		VectorBorder::CreateOGRPolygonByOGREnvelope (OGREnvelope &envelope)
{
	OGRPolygon *p_ogr_poly = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);

	OGRLinearRing	lr;
	lr.addPoint(envelope.MinX,envelope.MinY);
	lr.addPoint(envelope.MinX,envelope.MaxY);
	lr.addPoint(envelope.MaxX,envelope.MaxY);
	lr.addPoint(envelope.MaxX,envelope.MinY);
	lr.closeRings();
	p_ogr_poly->addRing(&lr);
	
	return p_ogr_poly;
};


OGRGeometry*	VectorBorder::get_ogr_geometry_ref()
{
  return p_ogr_geometry_;
}


}