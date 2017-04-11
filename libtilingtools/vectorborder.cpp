#include "vectorborder.h"
#include "filesystemfuncs.h"
#include "stringfuncs.h"


namespace gmx
{


VectorOperations::VectorOperations ()
{
	p_ogr_geometry_	= NULL;
}

VectorOperations::~VectorOperations()
{
	if (p_ogr_geometry_!=NULL) 
  {
 	  delete(p_ogr_geometry_);
    p_ogr_geometry_	= NULL;
  }
}  




string VectorOperations::GetVectorFileNameByRasterFileName (string raster_file)
{
 	string	vector_file_base = GMXFileSys::RemoveExtension(raster_file);
	string	vector_file = GMXFileSys::FileExists(vector_file_base+".mif") ?	vector_file_base+".mif" :
                        GMXFileSys::FileExists(vector_file_base+".shp") ?	vector_file_base+".shp" :
	                      GMXFileSys::FileExists(vector_file_base+".tab") ?	vector_file_base+".tab" :"";
	
  if (vector_file != "")
  {
    VECTORDS* poVecDS = VectorOperations::OpenVectorFile(vector_file);
    if (!poVecDS) vector_file = "";
    else VectorOperations::CloseVECTORDS(poVecDS);
  }

  return vector_file;
}


OGREnvelope	VectorOperations::CombineOGREnvelopes (OGREnvelope	&envp1, OGREnvelope	&envp2)
{
	OGREnvelope envp;
  envp.MaxX = max(envp1.MaxX,envp2.MaxX);
	envp.MaxY = max(envp1.MaxY,envp2.MaxY);
	envp.MinX = min(envp1.MinX,envp2.MinX);
	envp.MinY = min(envp1.MinY,envp2.MinY);
	return envp;
}

OGREnvelope	VectorOperations::InetersectOGREnvelopes (OGREnvelope	&envp1,OGREnvelope	&envp2)
{
	OGREnvelope envp;
	envp.MaxX = min(envp1.MaxX,envp2.MaxX);
	envp.MaxY = min(envp1.MaxY,envp2.MaxY);
	envp.MinX = max(envp1.MinX,envp2.MinX);
	envp.MinY = max(envp1.MinY,envp2.MinY);
	return envp;
}

/*

bool VectorBorder::ConvertOGRGeometryToArrayOfSegments (OGRGeometry *p_ogr_geom, int &num_segments, OGRLineString **pp_ls)
{
  if (p_ogr_geom->getGeometryType()!=OGRWkb 
  OGRPolygon *p_polygons = NULL;

  return TRUE;
}
*/

bool VectorOperations::IsPointInsidePixelLineGeometry (OGRPoint point, OGRGeometry *po_ogr_geom)
{
  double e = 1e-6;
  int num_points = 0;
  int num_rings = 0;

  if (!po_ogr_geom) return false;
  po_ogr_geom->closeRings();
  
  OGRLinearRing **pp_lr = GetLinearRingsRef(po_ogr_geom,num_rings);
  if (pp_lr == NULL) return false;
    

  double x1,x2,y1,y2;
  double y_line = point.getY();
  double x0 = point.getX();

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
        if ((fabs(y_line-y1)>e)&&(fabs(y_line-y2)>e))
        {
          if ((y_line-y1)*(y_line-y2)<e)
          {
            if ((x2*(y1-y_line)-x1*(y2-y_line))/(y1-y2)>x0+e)
              num_points++;
          }
        }
        else if (fabs(y_line-y1)<e) continue;
        else if (x2>x0+e) 
        {
          for (int k=2;k<pp_lr[i]->getNumPoints();k++)
          {
            int k__ = (j+k) % pp_lr[i]->getNumPoints();
            if (fabs(pp_lr[i]->getY(k__)-y_line)<e) continue;
            else if ((y_line-y1)*(y_line-pp_lr[i]->getY(k__))<e)
            {
              num_points++;
              break;
            }
            else break;
          }
        }
      }
    }
  }
  delete[]pp_lr;
  return num_points%2;

}


bool VectorOperations::CalcIntersectionBetweenLineAndPixelLineGeometry (int y_line, OGRGeometry *po_ogr_geom, int &num_points, int *&x)
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
        if ((fabs(y_line-y1)<e)&&(fabs(y_line-y2)<e))
        {
          for (double x=min(x1,x2);x<=max(x1,x2)+e;x++)
            x_val_list.push_back(x);
        }
        else if ((y_line-y1)*(y_line-y2)<=e)
        {
          x_val_list.push_back((x2*(y1-y_line)-x1*(y2-y_line))/(y1-y2));
        }
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


bool VectorOperations::AddIntermediatePoints(OGRPolygon *p_polygon, int points_on_segment)
{
  if(!p_polygon) return false;
  
  p_polygon->closeRings();

  OGRLinearRing*  p_ring = p_polygon->getExteriorRing();
  int num_points = p_ring->getNumPoints();

  OGRLinearRing o_new_ring;
  
  for (int i=0;i<num_points-1;i++)
  {
    double x = p_ring->getX(i);
    double y = p_ring->getY(i);
    double dx=(p_ring->getX(i+1)-x)/points_on_segment;
    double dy=(p_ring->getY(i+1)-y)/points_on_segment;
    o_new_ring.addPoint(x,y);
    for (int j=0;j<points_on_segment-1;j++)
      o_new_ring.addPoint(x+dx*(j+1),y+dy*(j+1));
  }
  o_new_ring.closeRings();
  p_ring->empty();
  for (int i=0; i<o_new_ring.getNumPoints();i++)
  {
    p_ring->addPoint(o_new_ring.getX(i),o_new_ring.getY(i));
  }

  return true;
}

VECTORDS* VectorOperations::OpenVectorFile(string strVectorFile, bool bReadOnly)
{
#ifdef GDAL_OF_VECTOR
  return (VECTORDS*)GDALOpenEx(strVectorFile.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
#else
  return (VECTORDS*)OGROpen(vector_file.c_str(), 0, 0);
#endif
}

void VectorOperations::CloseVECTORDS(VECTORDS* poVecDS)
{
#ifdef GDAL_OF_VECTOR
  GDALClose(poVecDS);
#else
  OGRDataSource::DestroyDataSource(poVecDS);
#endif
}




OGRGeometry*	VectorOperations::ReadAndTransformGeometry(string vector_file, OGRSpatialReference *p_tiling_srs)
{
  if (vector_file=="") return 0;
  
  VECTORDS* poVecDS = VectorOperations::OpenVectorFile(vector_file.c_str());
  if (!poVecDS) return 0;
	
	OGRMultiPolygon *p_ogr_multipoly;

  if (!(p_ogr_multipoly=ReadMultiPolygonFromOGRDataSource(poVecDS)))
  {
    CloseVECTORDS(poVecDS);
    cout<<"ERROR: ReadMultiPolygonFromOGRDataSource: can't read polygon from input vector"<<endl;
    return NULL;
  }
  
  p_ogr_multipoly->closeRings();
  OGRLayer *p_ogr_layer = poVecDS->GetLayer(0);
  OGRSpatialReference *p_input_ogr_sr = p_ogr_layer->GetSpatialRef();
  bool is_ogr_sr_valid = TRUE;
  if (p_input_ogr_sr)
  {
    p_input_ogr_sr = p_input_ogr_sr->Clone();
    char	*proj_ref = NULL;
	  bool is_ogr_sr_valid = (OGRERR_NONE == p_input_ogr_sr->exportToProj4(&proj_ref))  
                            ? TRUE
                            : (OGRERR_NONE != p_input_ogr_sr->morphFromESRI()) 
                            ? FALSE 
                            : (OGRERR_NONE == p_input_ogr_sr->exportToProj4(&proj_ref)) 
                            ? TRUE : FALSE;
    if (proj_ref !=NULL) CPLFree(proj_ref);
  }
  else
  {
	  p_input_ogr_sr = new OGRSpatialReference();
	  p_input_ogr_sr->SetWellKnownGeogCS("WGS84");
  }
    
  CloseVECTORDS(poVecDS);
  if (!is_ogr_sr_valid)
  {
    OGRSpatialReference::DestroySpatialReference(p_input_ogr_sr);
    delete(p_ogr_multipoly);
    return NULL;
  }
        
  p_ogr_multipoly->assignSpatialReference(p_input_ogr_sr);
      
  if (OGRERR_NONE != p_ogr_multipoly->transformTo(p_tiling_srs))
  {
    OGRSpatialReference::DestroySpatialReference(p_input_ogr_sr);
    delete(p_ogr_multipoly);
	  return NULL;
  }

  p_ogr_multipoly->assignSpatialReference(NULL);
  OGRSpatialReference::DestroySpatialReference(p_input_ogr_sr);
  return p_ogr_multipoly;


};

OGRMultiPolygon* VectorOperations::ReadMultiPolygonFromOGRDataSource(VECTORDS* poVecDS)
{
  OGRLayer *p_ogr_layer = poVecDS->GetLayer(0);
	if( p_ogr_layer == 0 ) return 0;
  if (p_ogr_layer->GetFeatureCount()==0) return 0;
	
  p_ogr_layer->ResetReading();

	OGRMultiPolygon *p_ogr_multipoly = (OGRMultiPolygon*)OGRGeometryFactory::createGeometry(wkbMultiPolygon);
	while (OGRFeature *poFeature = p_ogr_layer->GetNextFeature())
	{
    if (OGRERR_NONE == p_ogr_multipoly->addGeometry(poFeature->GetGeometryRef()))
      OGRFeature::DestroyFeature( poFeature);
    else
    {
      delete(p_ogr_multipoly);
      OGRFeature::DestroyFeature( poFeature);
      return 0;
    }
	}

	return p_ogr_multipoly;
};



OGRLinearRing**		VectorOperations::GetLinearRingsRef	(OGRGeometry	*p_ogr_geom, int &num_rings)
{
	num_rings = 0;
	OGRLinearRing	**pp_ogr_rings = NULL;
	OGRPolygon		**pp_ogr_poly = NULL;
	OGRwkbGeometryType type = p_ogr_geom->getGeometryType();

	if (!(type==wkbMultiPolygon || type==wkbPolygon || 
		    type==wkbLinearRing || type==wkbLineString)) return NULL;

	int	numPolygons = 0;
	bool	inputIsRing = FALSE;
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

bool	VectorOperations::Intersects(OGREnvelope &envelope)
{
  if (p_ogr_geometry_ == NULL) return FALSE;
	OGRPolygon *p_poly_from_envp = CreateOGRPolygonByOGREnvelope (envelope);

  bool result = this->p_ogr_geometry_->Intersects(p_poly_from_envp);
  delete(p_poly_from_envp);

  return result;
}


OGREnvelope VectorOperations::GetEnvelope ()
{
	OGREnvelope envp;
	if (p_ogr_geometry_!=NULL) p_ogr_geometry_->getEnvelope(&envp);
	return envp;
};



OGRPolygon*		VectorOperations::CreateOGRPolygonByOGREnvelope (OGREnvelope &envelope)
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


OGRGeometry*	VectorOperations::get_ogr_geometry_ref()
{
  return p_ogr_geometry_;
}


}