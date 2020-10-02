#include "vectorborder.h"
//#include "filesystemfuncs.h"
//#include "stringfuncs.h"


namespace ttx
{





  string VectorOperations::GetVectorFileNameByRasterFileName(string raster_file)
  {
    string	vector_file_base = MPLFileSys::RemoveExtension(raster_file);
    string	vector_file = MPLFileSys::FileExists(vector_file_base + ".mif") ? vector_file_base + ".mif" :
      MPLFileSys::FileExists(vector_file_base + ".shp") ? vector_file_base + ".shp" :
      MPLFileSys::FileExists(vector_file_base + ".tab") ? vector_file_base + ".tab" : "";

    if (vector_file != "")
    {
      VECTORDS* poVecDS = VectorOperations::OpenVectorFile(vector_file);
      if (!poVecDS) vector_file = "";
      else VectorOperations::CloseVECTORDS(poVecDS);
    }

    return vector_file;
  }


  OGRMultiPolygon* VectorOperations::ConvertFromSRSToPixelLine(OGRGeometry *p_ogr_geom, double geotransform[6])
  {
    double d = geotransform[1] * geotransform[5] - geotransform[2] * geotransform[4];
    if (fabs(d)< 1e-7) return 0;

    OGRMultiPolygon* p_input_geom = 0;

    bool is_collection = ((p_ogr_geom->getGeometryType() == wkbPolygon) ||
      (p_ogr_geom->getGeometryType() == wkbPolygon25D)) ? false : true;

    int num_geom = is_collection ? ((OGRMultiPolygon*)p_ogr_geom)->getNumGeometries() : 1;

    OGRMultiPolygon* p_output_geom = (OGRMultiPolygon*)OGRGeometryFactory::createGeometry(wkbMultiPolygon);

    for (int j = 0; j<num_geom; j++)
    {
      OGRPolygon* p_poly = is_collection ?
        (OGRPolygon*)((OGRMultiPolygon*)p_ogr_geom)->getGeometryRef(j) :
        (OGRPolygon*)p_ogr_geom;
      int num_rings = 0;
      p_poly->closeRings();
      OGRLinearRing** p_rings = VectorOperations::GetLinearRingsRef(p_poly, num_rings);

      if (num_rings == 0 || p_rings == 0)
      {
        delete (p_output_geom);
        return 0;
      }

      OGRPolygon* p_output_poly = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);

      for (int i = 0; i < num_rings; i++)
      {
        OGRLinearRing* p_input_ring = (OGRLinearRing*)p_rings[i];
        OGRLinearRing* p_output_ring = (OGRLinearRing*)p_input_ring->clone();

        for (int k = 0; k< p_input_ring->getNumPoints(); k++)
        {
          double x = p_input_ring->getX(k) - geotransform[0];
          double y = p_input_ring->getY(k) - geotransform[3];

          double l = (geotransform[1] * y - geotransform[4] * x) / d;
          double p = (x - l*geotransform[2]) / geotransform[1];

          p_output_ring->setPoint(k, (int)(p + 0.5), (int)(l + 0.5));
        }
        p_output_poly->addRingDirectly(p_output_ring);
      }

      p_output_geom->addGeometryDirectly(p_output_poly);
    }

    return p_output_geom;
  }

  /*
  GDALDataset* VectorOperations::CreateVirtualVectorLayer(string strLayerName,
	  OGRSpatialReference* poSRS, OGRwkbGeometryType eType)
  {
	  GDALDriver* poSHPDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
	  string	strInMemName = ("/vsimem/shpinmem_" + MPLString::ConvertIntToString(rand()));
	  GDALDataset* poInMemSHP = poSHPDriver->Create(strInMemName.c_str(), 0, 0, 0, GDT_Unknown, NULL);
	  
	  OGRLayer* poLayer = poInMemSHP->CreateLayer(strLayerName.c_str(), poSRS, wkbPolygon, NULL);

	  return poInMemSHP;
  }
  */


  OGREnvelope	VectorOperations::MergeEnvelopes(const OGREnvelope	&envp1, const OGREnvelope	&envp2)
  {
    OGREnvelope envp(envp1);
    envp.Merge(envp2);
    return envp;
  }

  OGREnvelope	VectorOperations::InetersectEnvelopes(const OGREnvelope	&envp1, const OGREnvelope	&envp2)
  {
    OGREnvelope envp(envp1);
    envp.Intersect(envp2);
    return envp;
  }



  bool VectorOperations::IsPointInsidePixelLineGeometry(OGRPoint point, OGRGeometry *po_ogr_geom)
  {
    double e = 1e-6;
    int num_points = 0;
    int num_rings = 0;

    if (!po_ogr_geom) return false;
    po_ogr_geom->closeRings();

    OGRLinearRing **pp_lr = GetLinearRingsRef(po_ogr_geom, num_rings);
    if (pp_lr == NULL) return false;


    double x1, x2, y1, y2;
    double y_line = point.getY();
    double x0 = point.getX();

    for (int i = 0; i<num_rings; i++)
    {
      if (pp_lr[i]->getNumPoints() >= 4)
      {
        for (int j = 0; j<pp_lr[i]->getNumPoints() - 1; j++)
        {
          x1 = pp_lr[i]->getX(j);
          x2 = pp_lr[i]->getX(j + 1);
          y1 = pp_lr[i]->getY(j);
          y2 = pp_lr[i]->getY(j + 1);
          if ((fabs(y_line - y1)>e) && (fabs(y_line - y2)>e))
          {
            if ((y_line - y1)*(y_line - y2)<e)
            {
              if ((x2*(y1 - y_line) - x1*(y2 - y_line)) / (y1 - y2)>x0 + e)
                num_points++;
            }
          }
          else if (fabs(y_line - y1)<e) continue;
          else if (x2>x0 + e)
          {
            for (int k = 2; k<pp_lr[i]->getNumPoints(); k++)
            {
              int k__ = (j + k) % pp_lr[i]->getNumPoints();
              if (fabs(pp_lr[i]->getY(k__) - y_line)<e) continue;
              else if ((y_line - y1)*(y_line - pp_lr[i]->getY(k__))<e)
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
    return num_points % 2;

  }

  /*
  bool VectorOperations::IntersectYLineWithPixelLineGeometry (int y_line, OGRGeometry *po_ogr_geom, int &num_points, int *&x)
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
  */

  bool VectorOperations::AddIntermediatePoints(OGRPolygon *p_polygon, int points_on_segment)
  {
    if (!p_polygon) return false;

    p_polygon->closeRings();

    OGRLinearRing*  p_ring = p_polygon->getExteriorRing();
    int num_points = p_ring->getNumPoints();

    OGRLinearRing o_new_ring;

    for (int i = 0; i<num_points - 1; i++)
    {
      double x = p_ring->getX(i);
      double y = p_ring->getY(i);
      double dx = (p_ring->getX(i + 1) - x) / points_on_segment;
      double dy = (p_ring->getY(i + 1) - y) / points_on_segment;
      o_new_ring.addPoint(x, y);
      for (int j = 0; j<points_on_segment - 1; j++)
        o_new_ring.addPoint(x + dx*(j + 1), y + dy*(j + 1));
    }
    o_new_ring.closeRings();
    p_ring->empty();
    for (int i = 0; i<o_new_ring.getNumPoints(); i++)
    {
      p_ring->addPoint(o_new_ring.getX(i), o_new_ring.getY(i));
    }

    return true;
  }

  VECTORDS* VectorOperations::OpenVectorFile(string strVectorFile, bool bReadOnly)
  {
#ifdef GDAL_OF_VECTOR
    return (VECTORDS*)GDALOpenEx(strVectorFile.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
#else
    return (VECTORDS*)OGROpen(strVectorFile.c_str(), 0, 0);
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

  int VectorOperations::ReadAllFeatures(string strVectorFile, OGRFeature** &paoFeatures, OGRSpatialReference  *poSRS)
  {
    int nCount = 0;
    if (strVectorFile == "") return 0;

    VECTORDS* poVecDS = VectorOperations::OpenVectorFile(strVectorFile.c_str());
    if (!poVecDS) return 0;
   
    OGRLayer* poLayer = poVecDS->GetLayer(0);
    if (poLayer == 0) return 0;
    
    if ((nCount = poLayer->GetFeatureCount()) == 0) return 0;

    OGRSpatialReference* poInputSRS = 0;
    if (poSRS)
    {
      poInputSRS = poLayer->GetSpatialRef();
      bool is_ogr_sr_valid = true;
      if (poInputSRS)
      {
        poInputSRS = poInputSRS->Clone();
        char	*proj_ref = NULL;
        bool is_ogr_sr_valid = (OGRERR_NONE == poInputSRS->exportToProj4(&proj_ref))
          ? TRUE
          : (OGRERR_NONE != poInputSRS->morphFromESRI())
          ? FALSE
          : (OGRERR_NONE == poInputSRS->exportToProj4(&proj_ref))
          ? TRUE : FALSE;
        if (proj_ref != NULL) CPLFree(proj_ref);
      }
      else
      {
        poInputSRS = new OGRSpatialReference();
        poInputSRS->SetWellKnownGeogCS("WGS84");
      }
      
      if (!is_ogr_sr_valid)
      {
        OGRSpatialReference::DestroySpatialReference(poInputSRS);
        CloseVECTORDS(poVecDS);
        return 0;
      }
    }
      
    paoFeatures = new OGRFeature*[nCount];
    poLayer->ResetReading();
    for (int i=0;i<nCount;i++)
    {
      paoFeatures[i] = poLayer->GetNextFeature();
      if (poSRS)
      {
        OGRGeometry *poGeom = paoFeatures[i]->GetGeometryRef();
        poGeom->assignSpatialReference(poInputSRS);
        if (OGRERR_NONE != poGeom->transformTo(poSRS))
        {
          OGRSpatialReference::DestroySpatialReference(poInputSRS);
          CloseVECTORDS(poVecDS);
          for (int j=0;j<=i;j++)
            OGRFeature::DestroyFeature(paoFeatures[j]);
          delete[]paoFeatures;
          return 0;
        }
        poGeom->assignSpatialReference(0);
      }
    }
   
    if (poInputSRS)
      OGRSpatialReference::DestroySpatialReference(poInputSRS);
       
    CloseVECTORDS(poVecDS);
    return nCount;
  }


  OGRGeometry*	VectorOperations::ReadIntoSingleMultiPolygon(string vector_file, 
                                                                OGRSpatialReference *p_tiling_srs,
                                                                int* panFIDs,
                                                                int nFIDCount)
  {
    OGRFeature** paoFeatures = 0;
    int nFeatures = ReadAllFeatures(vector_file, paoFeatures, p_tiling_srs);

    if (panFIDs)
    {
      OGRFeature** paoFeaturesAll = paoFeatures;
      int nFeaturesAll = nFeatures;
      paoFeatures = new OGRFeature*[nFIDCount];
      nFeatures = 0;
      for (int i = 0; i < nFeaturesAll; i++)
      {
        int j;
        for ( j= 0; j < nFIDCount; j++)
        {
          if (panFIDs[j] == i + 1)
          {
            paoFeatures[nFeatures] = paoFeaturesAll[i];
            nFeatures++;
            break;
          }
        }
        if (j == nFIDCount) OGRFeature::DestroyFeature(paoFeaturesAll[i]);
      }
      delete[]paoFeaturesAll;
    }

    if (nFeatures == 0) return 0;

    OGRMultiPolygon *p_ogr_multipoly;

    if (!(p_ogr_multipoly = CombineAllGeometryIntoSingleMultiPolygon(paoFeatures,nFeatures)))
    {
      cout << "ERROR: VectorOperations::ReadIntoSingleMultiPolygon(OGRFeatures**,int) fail" << endl;
    }

    for (int i=0;i<nFeatures;i++)
      OGRFeature::DestroyFeature(paoFeatures[i]);
    delete[]paoFeatures;

    return p_ogr_multipoly;
  };


  OGRMultiPolygon* VectorOperations::CombineAllGeometryIntoSingleMultiPolygon(OGRFeature** paoFeautures, int nFeatures)
  {
    OGRGeometry *poUnion = 0; 
    for (int i=0;i<nFeatures; i++)
    {
      if (!poUnion) poUnion = paoFeautures[i]->GetGeometryRef()->clone();
      else
      {
        OGRGeometry* poNewGeom = poUnion->Union(paoFeautures[i]->GetGeometryRef());
        delete(poUnion);
        poUnion = poNewGeom;
      }
      if (poUnion == 0)
      {
        cout << "ERROR: OGRGeometry::Union fail"<<endl;
        return 0;
      }
    }

  
    OGRMultiPolygon *poMultiPolygon = 0;
    switch (poUnion->getGeometryType())
    {
      case wkbMultiPolygon:
        poMultiPolygon = (OGRMultiPolygon*)poUnion;
        break;
      case wkbPolygon:
        poMultiPolygon = (OGRMultiPolygon*)OGRGeometryFactory::createGeometry(wkbMultiPolygon);
        poMultiPolygon->addGeometryDirectly(poUnion);
        //delete(poUnion);
        break;
      case wkbPolygon25D:
        poMultiPolygon = (OGRMultiPolygon*)OGRGeometryFactory::createGeometry(wkbMultiPolygon);
        poMultiPolygon->addGeometryDirectly((OGRPolygon*)poUnion);
        break;
      case wkbMultiPolygon25D:
        poMultiPolygon = (OGRMultiPolygon*)poUnion;
        break;
      default:
        delete(poUnion);
        break;
    }
    
       
    return poMultiPolygon;
  }

  bool VectorOperations::RemovePolygonFromMultiPolygon(OGRMultiPolygon* poMultiPoly, OGRPolygon* poPoly)
  {
    
    for (int i = 0; i < poMultiPoly->getNumGeometries(); i++)
    {
      if (poMultiPoly->getGeometryRef(i) == poPoly)
      {
        poMultiPoly->removeGeometry(i);
        return true;
      }
    }
    
    return false;
  }

  int VectorOperations::ExtractAllPoints(OGRGeometry* poGeometry, double* &padblX, double* &padblY)
  {
    int nPointsCount = 0;
    int nRingsCount;
    OGRLinearRing** papoRings = GetLinearRingsRef(poGeometry,nRingsCount);

    for (int i = 0; i < nRingsCount; i++)
      nPointsCount += papoRings[i]->getNumPoints();
    
    padblX = new double[nPointsCount];
    padblY = new double[nPointsCount];

    int n = 0;    
    for (int i = 0; i < nRingsCount; i++)
    {
      for (int j = 0; j < papoRings[i]->getNumPoints(); j++)
      {
        padblX[n] = papoRings[i]->getX(j);
        padblY[n] = papoRings[i]->getY(j);
        n++;
      }
    }
    
    
    return nPointsCount;  
  }


  OGRLinearRing**		VectorOperations::GetLinearRingsRef(OGRGeometry* p_ogr_geom, int &num_rings)
  {
    num_rings = 0;
    OGRLinearRing	**pp_ogr_rings = NULL;
    OGRPolygon		**pp_ogr_poly = NULL;

    int	numPolygons = 0;
    bool	inputIsRing = FALSE;
    if ((wkbPolygon == p_ogr_geom->getGeometryType()) ||
      (wkbPolygon25D == p_ogr_geom->getGeometryType()))
    {
      numPolygons = 1;
      pp_ogr_poly = new OGRPolygon*[numPolygons];
      pp_ogr_poly[0] = (OGRPolygon*)p_ogr_geom;
    }
    else
    {
      numPolygons = ((OGRMultiPolygon*)p_ogr_geom)->getNumGeometries();
      pp_ogr_poly = new OGRPolygon*[numPolygons];
      for (int i = 0; i<numPolygons; i++)
        pp_ogr_poly[i] = (OGRPolygon*)((OGRMultiPolygon*)p_ogr_geom)->getGeometryRef(i);
    }

    if (numPolygons>0)
    {
      for (int i = 0; i<numPolygons; i++)
      {
        num_rings++;
        num_rings += pp_ogr_poly[i]->getNumInteriorRings();
      }
      pp_ogr_rings = new OGRLinearRing*[num_rings];
      int j = 0;
      for (int i = 0; i<numPolygons; i++)
      {
        pp_ogr_rings[j] = pp_ogr_poly[i]->getExteriorRing();
        j++;
        for (int k = 0; k<pp_ogr_poly[i]->getNumInteriorRings(); k++)
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

  /*
  bool	VectorOperations::Intersects(OGREnvelope &envelope)
  {
    if (p_ogr_geometry_ == NULL) return FALSE;
    OGRPolygon *p_poly_from_envp = CreateOGRPolygonByOGREnvelope(envelope);

    bool result = this->p_ogr_geometry_->Intersects(p_poly_from_envp);
    delete(p_poly_from_envp);

    return result;
  }
  

  OGREnvelope VectorOperations::GetEnvelope()
  {
    OGREnvelope envp;
    if (p_ogr_geometry_ != NULL) p_ogr_geometry_->getEnvelope(&envp);
    return envp;
  };
  */


  OGRPolygon*		VectorOperations::CreateOGRPolygonByOGREnvelope(const OGREnvelope &envelope)
  {
    OGRPolygon *p_ogr_poly = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);

    OGRLinearRing	lr;
    lr.addPoint(envelope.MinX, envelope.MinY);
    lr.addPoint(envelope.MinX, envelope.MaxY);
    lr.addPoint(envelope.MaxX, envelope.MaxY);
    lr.addPoint(envelope.MaxX, envelope.MinY);
    lr.closeRings();
    p_ogr_poly->addRing(&lr);

    return p_ogr_poly;
  };

  /*
  OGRGeometry*	VectorOperations::get_ogr_geometry_ref()
  {
    return p_ogr_geometry_;
  }
  */

}