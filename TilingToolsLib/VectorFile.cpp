#include "StdAfx.h"
#include "VectorFile.h"
#include "str.h"

BOOL	VectorFile::CreateTileGridFile (wstring shapeFileName, int zoom, MercatorProjType projType)
{

	OGRGeometry **poGeometry = new OGRGeometry*[1<<(2*zoom)];
	for (int i=0;i<(1<<(zoom));i++)
	{
		for (int j=0;j<(1<<(zoom));j++)
		{
			int n = i*(1<<zoom) + j;
			poGeometry[n] = new OGRPolygon();
			OGRLinearRing oLR;
			OGREnvelope tileEnvelope = MercatorTileGrid::calcEnvelopeByTile(zoom,j,i);
			oLR.addPoint(tileEnvelope.MinX,tileEnvelope.MinY);
			oLR.addPoint(tileEnvelope.MinX,tileEnvelope.MaxY);
			oLR.addPoint(tileEnvelope.MaxX,tileEnvelope.MaxY);
			oLR.addPoint(tileEnvelope.MaxX,tileEnvelope.MinY);
			oLR.closeRings();
			((OGRPolygon*)poGeometry[n])->addRing(&oLR);
		}
	}
	
	BOOL result = CreateMercatorVectorFileByGeometryArray(poGeometry,(1<<(2*zoom)),shapeFileName,projType);
	for (int i=0;i<(1<<(2*zoom));i++)
		poGeometry[i]->empty();
	delete[]poGeometry;

	return result;
	//CreateMercatorVectorFileByGeometryArray
	//return CreateMercatorVectorFileByGeometryArray(poGeometry,
}


VectorFile::VectorFile (void)
{
	m_poDS		= NULL;
	OGRRegisterAll();
}

void VectorFile::delete_all()
{
	if (m_poDS		!= NULL) OGRDataSource::DestroyDataSource( m_poDS );
	m_poDS		= NULL;
}

VectorFile::~VectorFile(void)
{
	delete_all();	
}


BOOL VectorFile::Open (wstring	strVectorFile)
{
	delete_all();
	string strVectorFileUTF8;
	wstrToUtf8(strVectorFileUTF8,strVectorFile);

	m_poDS = OGRSFDriverRegistrar::Open( strVectorFileUTF8.c_str(), FALSE );
	if( m_poDS == NULL ) return FALSE;
	
	return TRUE;
};


BOOL	VectorFile::OpenAndCreatePolygonInMercator (wstring strVectorFile,VectorBorder &destPolygon, MercatorProjType mercType)
{
	VectorFile oVectorFile;
	if (!oVectorFile.Open(strVectorFile)) return FALSE;
	if (!oVectorFile.readPolygonAndTransformToMerc(destPolygon, mercType)) return FALSE;
	return TRUE;
}



BOOL VectorFile::GetVectorBorder (VectorBorder &oPolygon)
{
	OGRGeometry *poGeometry = GetOGRGeometry();
	if (poGeometry==NULL) return FALSE;
	if (!oPolygon.InitByOGRGeometry(poGeometry)) return FALSE;
	poGeometry->empty();
	return TRUE;
};

OGRGeometry* VectorFile::GetOGRGeometry ()
{
	if (this->m_poDS == NULL) return NULL;
	OGRLayer *poLayer = m_poDS->GetLayer(0);
	if( poLayer == NULL ) return NULL;
	poLayer->ResetReading();

	OGRMultiPolygon *poMultiPolygon = NULL;
	while (OGRFeature *poFeature = poLayer->GetNextFeature())
	{
		if (poFeature->GetGeometryRef()->getGeometryType() == wkbMultiPolygon)
		{
			if (poMultiPolygon!=NULL) break;
			poMultiPolygon = (OGRMultiPolygon*)(poFeature->GetGeometryRef())->clone();
		}
		else if (poFeature->GetGeometryRef()->getGeometryType() == wkbPolygon)
		{
			if (poMultiPolygon==NULL) poMultiPolygon = new OGRMultiPolygon();
			poMultiPolygon->addGeometry(poFeature->GetGeometryRef());
		}
		OGRFeature::DestroyFeature( poFeature);
	}
	//if( poFeature == NULL ) return NULL;
	return poMultiPolygon;
}



/*
BOOL VectorFile::CalcRectEnvelope(OGRGeometry	*poGeometry, double *min_x, double *min_y,double *max_x, double *max_y)
{
	if (poGeometry == NULL) return FALSE;
	if (poGeometry->getGeometryType()!=wkbPolygon) return FALSE;
	
	OGRPolygon	*poPolygon = (OGRPolygon*)poGeometry;

	OGREnvelope oEnvelope;
	poPolygon->getEnvelope(&oEnvelope);
	*min_x  = oEnvelope.MinX;
	*max_x	= oEnvelope.MaxX;
	*min_y  = oEnvelope.MinY;
	*max_y	= oEnvelope.MaxY;
	return TRUE;
};			
*/

///*
wstring VectorFile::GetVectorFileByRasterFileName (wstring strRasterFile)
{
	wstring	strVectorFileBase = RemoveExtension(strRasterFile);
	wstring	strVectorFile;
	
	if (FileExists(strVectorFileBase+L".mif"))	strVectorFile = strVectorFileBase+L".mif";
	if (FileExists(strVectorFileBase+L".shp"))	strVectorFile = strVectorFileBase+L".shp";
	if (FileExists(strVectorFileBase+L".tab"))	strVectorFile = strVectorFileBase+L".tab";
	
	if (strVectorFile!=L"")
	{
		string strVectorFileUTF8;
		wstrToUtf8(strVectorFileUTF8,strVectorFile);

		OGRDataSource * poDS = OGRSFDriverRegistrar::Open( strVectorFileUTF8.c_str(), FALSE );
		if (poDS ==NULL) return L"";
		OGRDataSource::DestroyDataSource( poDS );
	}
	return strVectorFile;
}
//*/


BOOL	VectorFile::transformOGRPolygonToMerc(OGRGeometry *poSrcPolygon, VectorBorder &destPolygon, MercatorProjType mercType, OGRSpatialReference	*poSrcSpatial)
{
	OGRSpatialReference oSpatialMerc;
	if (mercType == WORLD_MERCATOR)
	{
		oSpatialMerc.SetWellKnownGeogCS("WGS84");
		oSpatialMerc.SetMercator(0,0,1,0,0);
	}
	else
	{
		oSpatialMerc.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
		//		PROJCS["Mercator_2SP",GEOGCS["GCS_unnamed ellipse",DATUM["D_unknown",SPHEROID["Unknown",6378137,0]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]],PROJECTION["Mercator_2SP"],PARAMETER["standard_parallel_1",0],PARAMETER["central_meridian",0],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["Meter",1]]
	}

	
	OGRCoordinateTransformation *poTransformation = OGRCreateCoordinateTransformation(poSrcSpatial,&oSpatialMerc);	
	if (poTransformation == NULL) return FALSE;

	///*
	OGRPolygon **poPolygons = NULL;
	OGRPolygon **poPolygonsMerc = NULL;
	int n = 0;
	
	//if (!(VectorBorder::Geometry2ArrayOfPolygons(poPolygon,n,poPolygons))) return FALSE;
	if (!((new VectorBorder(poSrcPolygon))->GetOGRPolygons(n,poPolygons))) return FALSE;
	poPolygonsMerc = new OGRPolygon*[n];

	for (int k=0; k<n;k++)
	{
		poPolygonsMerc[k] = new OGRPolygon();
		for (int i=-1;i<poPolygons[k]->getNumInteriorRings();i++)
		{ 
			OGRLinearRing *poRing;
			
			(i==-1) ? poRing = poPolygons[k]->getExteriorRing() : poRing = poPolygons[k]->getInteriorRing(i);
			double *x = new double[poRing->getNumPoints()];
			double *y = new double[poRing->getNumPoints()];
			for (int i=0;i<poRing->getNumPoints();i++)
			{
				x[i] = poRing->getX(i);
				y[i] = poRing->getY(i);
			}
				//oRingMerc.addPoint(x[i],y[i]);

			poTransformation->TransformEx(poRing->getNumPoints(),x,y,NULL,NULL);
			OGRLinearRing	oRingMerc;

			for (int i=0;i<poRing->getNumPoints();i++)
				oRingMerc.addPoint(x[i],y[i]);
			delete[]x;delete[]y;
			poPolygonsMerc[k]->addRing(&oRingMerc);
			//delete(poRing);
		}
	}
	OCTDestroyCoordinateTransformation(poTransformation);
	delete[]poPolygons;

	if (n==1) destPolygon.InitByOGRGeometry(poPolygonsMerc[0]);
	else
	{
		OGRMultiPolygon oMultiPolygon;
		for (int k=0;k<n;k++)
		{
			oMultiPolygon.addGeometry(poPolygonsMerc[k]);
			poPolygonsMerc[k]->empty();
		}
		destPolygon.InitByOGRGeometry(&oMultiPolygon);
	}
	delete[]poPolygonsMerc;
	AdjustFor180DegreeIntersection(destPolygon.GetOGRGeometry());

	return TRUE;
}




BOOL VectorFile::readPolygonAndTransformToMerc (VectorBorder &destPolygon, MercatorProjType mercType)
{
	OGRGeometry *poPolygon = GetOGRGeometry();
	if (poPolygon == NULL) return FALSE;

	OGRLayer *poLayer = m_poDS->GetLayer(0);
	

	OGRSpatialReference *poSpatial = poLayer->GetSpatialRef();
	char	*proj_ref;
	if (poSpatial!=NULL)
	{
		if (OGRERR_NONE!=poSpatial->exportToProj4(&proj_ref))
		{
			if (OGRERR_NONE!=poSpatial->morphFromESRI()) return FALSE; 
			if (OGRERR_NONE!=poSpatial->exportToProj4(&proj_ref)) return FALSE;
		}
		CPLFree(proj_ref);
	}
	else
	{
		poSpatial = new OGRSpatialReference();
		poSpatial->SetWellKnownGeogCS("WGS84");
	}
	
	return VectorFile::transformOGRPolygonToMerc(poPolygon,destPolygon,mercType,poSpatial);

};


BOOL	VectorFile::AdjustFor180DegreeIntersection (OGRGeometry	*poGeometry)
{
	OGRLinearRing	**poRings;
	int n;
	double D = 10018754.17139462153829420444035;
	
	if (!GeometryToArrayOfRings (poGeometry,n,poRings)) return FALSE;
	double max_x = 0, min_x = 0;
	for (int i=0;i<n;i++)
	{
		for (int j=0;j<poRings[i]->getNumPoints();j++)
		{
			if (poRings[i]->getX(j)>max_x) max_x = poRings[i]->getX(j);
			if (poRings[i]->getX(j)<min_x) min_x = poRings[i]->getX(j);
		}
	}
	if ((max_x>D)&&(min_x<-D))
	{
		for (int i=0;i<n;i++)
		{
			for (int j=0;j<poRings[i]->getNumPoints();j++)
			{
				if (poRings[i]->getX(j)<0) poRings[i]->setPoint(j,4*D + poRings[i]->getX(j),poRings[i]->getY(j));
			}
		}
	}

	return TRUE;
}

BOOL	VectorFile::deleteVectorFile (wstring fileName)
{
	if (!FileExists(fileName)) return TRUE;
	
	int s = fileName.find(L".shp");
	OGRSFDriver *poDriver = (s>=0) ? OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName("ESRI Shapefile") :
									OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName("MapInfo File");

	string fileNameUTF8;
	wstrToUtf8(fileNameUTF8,fileName);
	if (poDriver->DeleteDataSource(fileNameUTF8.c_str())==0) return TRUE;

	return FALSE;
}

BOOL	VectorFile::CreateMercatorVectorFileByGeometryArray(OGRGeometry **poGeometry, int num, wstring fileName, MercatorProjType mercType)
{
	VectorFile::deleteVectorFile(fileName);

	int k = MakeLower(fileName).find(L".shp");
	const char *pszDriverName = (k>=0) ? "ESRI Shapefile" : "Mapinfo File";
	OGRSFDriver *poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(pszDriverName );
	if( poDriver == NULL ) return FALSE;

	string fileNameUTF8;
	wstrToUtf8(fileNameUTF8,fileName);
	OGRDataSource *poDS_W = poDriver->CreateDataSource(fileNameUTF8.c_str(),NULL);
	if( poDS_W == NULL )
	{
		wcout<<"Error: Can't create border-file: "<<fileName<<endl;
		return FALSE;
	}
	OGRSpatialReference oSpatialRef;

	if (mercType = WORLD_MERCATOR)
	{
		oSpatialRef.SetWellKnownGeogCS("WGS84");
		oSpatialRef.SetMercator(0,0,1,0,0);;
	}
	else
	{
		oSpatialRef.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
	}
	OGRLayer *poLayer_W = poDS_W->CreateLayer( "BORDER", &oSpatialRef, wkbPolygon, NULL );
	poLayer_W->CreateField(new OGRFieldDefn("ID",OFTInteger));
	//delete(poLayer_W);
	OGRDataSource::DestroyDataSource( poDS_W );
	
	
	poDS_W = OGRSFDriverRegistrar::Open(fileNameUTF8.c_str(),TRUE);
	poLayer_W = poDS_W->GetLayer(0);	
	for (int i=0;i<num;i++)
	{
		OGRFeature *poFeature_W = new OGRFeature(poLayer_W->GetLayerDefn());
		poFeature_W->SetField(0,i);
		poFeature_W->SetGeometry(poGeometry[i]);
		OGRErr err = poLayer_W->CreateFeature(poFeature_W);
		//OGRFeature::DestroyFeature(poFeature_W);
	}
	


	//OGRFeature::DestroyFeature(poFeature_W);
	OGRDataSource::DestroyDataSource( poDS_W );

	return TRUE;
}





/*
BOOL VectorFile::CreateVectorFileByPoints  (int nNumOfPoints, 
											double *dblArray,
											wstring	strFeatureType,
											wstring	strFeatureData,
											wstring	strVectorFile
											)
{

	int data_len = 256;
	const char *pszDriverName = "ESRI Shapefile";
	OGRSFDriver *poDriver;

	if( !(poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(pszDriverName )) )
	{
		wcout<<L"Error: OGR: can't register ESRI Shapefile-driver:"<<endl;  
		return FALSE; 
	}

	OGRDataSource *poDS;
	string strVectorFileUTF8;
	wstrToUtf8(strVectorFileUTF8,strVectorFile);

	poDS = poDriver->CreateDataSource( strVectorFileUTF8.c_str(), NULL );

	if( !(poDS = poDriver->CreateDataSource( strVectorFileUTF8.c_str(), NULL )) )
	{
		wcout<<L"Error: OGR: can't open for writting shapefile: "<<strVectorFile<<endl; 
		throw ERROR;
	}

	OGRLayer *poLayer;

	OGRwkbGeometryType	eType;
	//if ((strFeatureType == "point")||(strFeatureType == "Point")||(strFeatureType=="Points")) eType = wkbPoint;
	//else 
	if ((strFeatureType == L"polygon")||(strFeatureType == L"Polygon")||(strFeatureType==L"Polygons")) eType = wkbPolygon;
	else if ((strFeatureType == L"polyline")||(strFeatureType == L"PolyLine")||(strFeatureType == L"Polyline")||(strFeatureType == L"line")||(strFeatureType == L"MultiLine")||(strFeatureType == L"Multiline")||(strFeatureType == L"Line")||(strFeatureType==L"Points")) eType = wkbLineString;
		
	if( !(poLayer = poDS->CreateLayer( "point_out", NULL, eType, NULL )) )
	{
		wcout<<L"Error: OGR: can't create layer: "<<strVectorFile<<endl;  
		return FALSE;
	}

	OGRFieldDefn oField( "Data", OFTString );

	oField.SetWidth(data_len);

	if( poLayer->CreateField( &oField ) != OGRERR_NONE )
	{
		wcout<<L"Error: OGR: can't create field: "<<strVectorFile<<endl;  
		return FALSE;
	}

	OGRFeature *poFeature;
	poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
	poFeature->SetField( "Data",strFeatureData.c_str());

	double x, y;
	OGRLineString	oLS;

	for (int i=0;i<nNumOfPoints;i++)
	{
		oLS.addPoint(dblArray[2*i],dblArray[2*i+1]);
	}

	//OGRLineRing		oLR;

	if (eType==wkbLineString) poFeature->SetGeometry(&oLS);
	else if (eType==wkbPolygon)
	{
		OGRLinearRing		oLR;
		oLR.addSubLineString(&oLS);
		oLR.closeRings();
		OGRPolygon oPolygon;
		oPolygon.addRing(&oLR);
		poFeature->SetGeometry(&oPolygon);
	}

	if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
	{
	   wcout<<L"Error: OGR: can't add feature to the file:: "<<strShpFile<<endl;  
	   return FALSE;
	}
	
	OGRFeature::DestroyFeature( poFeature );
	OGRDataSource::DestroyDataSource( poDS );

	return TRUE;
};
*/



///*
