#include "StdAfx.h"
#include "VectorBorder.h"
namespace GMT
{


VectorBorder::VectorBorder ()
{
	m_poGeometry	= NULL;
}

VectorBorder::~VectorBorder()
{
	if (m_poGeometry!=NULL) m_poGeometry->empty();
	m_poGeometry	= NULL;
}


VectorBorder::VectorBorder (OGREnvelope mercEnvelope, MercatorProjType	mercType)
{
	this->mercType = mercType;
	this->m_poGeometry = VectorBorder::createOGRPolygonByOGREnvelope(mercEnvelope);
}



wstring VectorBorder::getVectorFileNameByRasterFileName (wstring strRasterFile)
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


OGREnvelope	VectorBorder::combineOGREnvelopes (OGREnvelope	&oEnvelope1, OGREnvelope	&oEnvelope2)
{
	OGREnvelope oEnvelope;
	oEnvelope.MaxX = max(oEnvelope1.MaxX,oEnvelope2.MaxX);
	oEnvelope.MaxY = max(oEnvelope1.MaxY,oEnvelope2.MaxY);
	oEnvelope.MinX = min(oEnvelope1.MinX,oEnvelope2.MinX);
	oEnvelope.MinY = min(oEnvelope1.MinY,oEnvelope2.MinY);
	return oEnvelope;
}

OGREnvelope	VectorBorder::inetersectOGREnvelopes (OGREnvelope	&oEnvelope1,OGREnvelope	&oEnvelope2)
{
	OGREnvelope oEnvelope;
	oEnvelope.MaxX = min(oEnvelope1.MaxX,oEnvelope2.MaxX);
	oEnvelope.MaxY = min(oEnvelope1.MaxY,oEnvelope2.MaxY);
	oEnvelope.MinX = max(oEnvelope1.MinX,oEnvelope2.MinX);
	oEnvelope.MinY = max(oEnvelope1.MinY,oEnvelope2.MinY);
	return oEnvelope;
}

VectorBorder*	VectorBorder::createFromVectorFile(wstring vectorFilePath, MercatorProjType	mercType)
{
	string vectorFilePathUTF8;
	wstrToUtf8(vectorFilePathUTF8,vectorFilePath);

	OGRDataSource *poDS= OGRSFDriverRegistrar::Open( vectorFilePathUTF8.c_str(), FALSE );
	if( poDS == NULL ) return NULL;
	
	OGRMultiPolygon *poMultiPolygon = readMultiPolygonFromOGRDataSource(poDS);
	if (poMultiPolygon == NULL) return NULL;

	OGRLayer *poLayer = poDS->GetLayer(0);
	OGRSpatialReference *poInputSpatial = poLayer->GetSpatialRef();
	char	*proj_ref = NULL;
	if (poInputSpatial!=NULL)
	{
		if (OGRERR_NONE!=poInputSpatial->exportToProj4(&proj_ref))
		{
			if (OGRERR_NONE!=poInputSpatial->morphFromESRI()) return NULL; 
			if (OGRERR_NONE!=poInputSpatial->exportToProj4(&proj_ref)) return NULL;
		}
		CPLFree(proj_ref);
	}
	else
	{
		poInputSpatial = new OGRSpatialReference();
		poInputSpatial->SetWellKnownGeogCS("WGS84");
	}
	

	OGRSpatialReference		oSpatialMerc;
	MercatorTileGrid::setMercatorSpatialReference(mercType,&oSpatialMerc);
	
	poMultiPolygon->assignSpatialReference(poInputSpatial);
	if (OGRERR_NONE != poMultiPolygon->transformTo(&oSpatialMerc))
	{
		poMultiPolygon->empty();
		return NULL;
	}

	adjustFor180DegreeIntersection(poMultiPolygon);
		
	VectorBorder	*poVB	= new VectorBorder();
	poVB->mercType			= mercType;
	poVB->m_poGeometry		= poMultiPolygon;
	return poVB;
};


OGRMultiPolygon*		VectorBorder::readMultiPolygonFromOGRDataSource(OGRDataSource *poDS)
{
	OGRLayer *poLayer = poDS->GetLayer(0);
	if( poLayer == NULL ) return NULL;
	poLayer->ResetReading();

	OGRMultiPolygon *poMultiPolygon = NULL;
	BOOL	validGeometry = FALSE;
	while (OGRFeature *poFeature = poLayer->GetNextFeature())
	{
		if (poMultiPolygon == NULL) poMultiPolygon = new OGRMultiPolygon();
		if ((poFeature->GetGeometryRef()->getGeometryType() == wkbMultiPolygon) ||
			(poFeature->GetGeometryRef()->getGeometryType() == wkbPolygon))
		{
			if (OGRERR_NONE == poMultiPolygon->addGeometry(poFeature->GetGeometryRef()))
				validGeometry = TRUE;
		}
		OGRFeature::DestroyFeature( poFeature);
	}

	if (!validGeometry) return NULL;

	return poMultiPolygon;
};


OGRGeometry*	VectorBorder::getOGRGeometryTransformed (OGRSpatialReference *poOutputSRS)
{
	if (this->m_poGeometry == NULL) return NULL;

	OGRSpatialReference		oSpatialMerc;
	MercatorTileGrid::setMercatorSpatialReference(mercType,&oSpatialMerc);

	OGRMultiPolygon *poTransformedGeometry = (OGRMultiPolygon*)this->m_poGeometry->clone();	
	poTransformedGeometry->assignSpatialReference(&oSpatialMerc);
	
	if (OGRERR_NONE != poTransformedGeometry->transformTo(poOutputSRS))
	{
		poTransformedGeometry->empty();
		return NULL;
	}

	return poTransformedGeometry;
};


OGRPolygon*	VectorBorder::getOGRPolygonTransformedToPixelLine(OGRSpatialReference *poOutputSRS, double *geoTransform)
{
	OGRGeometry *poGeometry = getOGRGeometryTransformed(poOutputSRS);

	OGRPolygon *poResultPolygon = (OGRPolygon*)((OGRMultiPolygon*)poGeometry)->getGeometryRef(0)->clone();
	poGeometry->empty();

	OGRLinearRing	*poRing = poResultPolygon->getExteriorRing();
	poRing->closeRings();
	
	for (int i=0;i<poRing->getNumPoints();i++)
	{
		double D = geoTransform[1]*geoTransform[5]-geoTransform[2]*geoTransform[4];
		if ( fabs(D) < 1e-7)
		{
			poResultPolygon->empty();
			return NULL;
		}

		double X = poRing->getX(i)	-	geoTransform[0];
		double Y = poRing->getY(i)	-	geoTransform[3];
		
		double L = (geoTransform[1]*Y-geoTransform[4]*X)/D;
		double P = (X - L*geoTransform[2])/geoTransform[1];

		poRing->setPoint(i,(int)(P+0.5),(int)(L+0.5));
	}
	return poResultPolygon;
}


BOOL			VectorBorder::adjustFor180DegreeIntersection (OGRGeometry	*poMercGeometry)
{
	OGRLinearRing	**poRings;
	double D = 10018754.17139462153829420444035;
	
	double max_x = -1e+308, min_x = 1e+308;

	//poMercGeometry->getGeometryType()
	OGRwkbGeometryType type = poMercGeometry->getGeometryType();

	if ((poMercGeometry->getGeometryType()!=wkbMultiPolygon) && 
		(poMercGeometry->getGeometryType()!=wkbMultiPolygon) &&
		(poMercGeometry->getGeometryType()!=wkbLinearRing) &&
		(poMercGeometry->getGeometryType()!=wkbLineString)
		) return FALSE;
	OGRPolygon	**poPolygons = NULL;

	int	numPolygons;
	if (poMercGeometry->getGeometryType()==wkbPolygon)
	{
		numPolygons		= 1;
		poPolygons		= new OGRPolygon*[numPolygons];
		poPolygons[0]	= (OGRPolygon*)poMercGeometry;
	}
	else if (poMercGeometry->getGeometryType()==wkbMultiPolygon)
	{
		numPolygons		= ((OGRMultiPolygon*)poMercGeometry)->getNumGeometries();
		poPolygons		= new OGRPolygon*[numPolygons];
		for (int i=0;i<numPolygons;i++)
			poPolygons[i] = (OGRPolygon*)((OGRMultiPolygon*)poMercGeometry)->getGeometryRef(i);
	}
	else 
	{
		numPolygons		= 1;
		poPolygons		= new OGRPolygon*[numPolygons];
		poPolygons[0]	= new OGRPolygon();
		poPolygons[0]->addRingDirectly((OGRLinearRing*)poMercGeometry);
	}


	for (int j=0;j<numPolygons;j++)
	{
		for (int i=0; i <poPolygons[j]->getNumInteriorRings() + 1; i++)
		{
			OGRLinearRing *poLR = (i==0) ? poPolygons[j]->getExteriorRing() : poPolygons[j]->getInteriorRing(i-1);
			for (int i = 0; i<poLR->getNumPoints();i++)
			{
				if (poLR->getX(i)>max_x) max_x = poLR->getX(i);
				if (poLR->getX(i)<min_x) min_x = poLR->getX(i);
			}
		}
	}

	if ((max_x>D)&&(min_x<-D))
	{
		for (int j=0;j<numPolygons;j++)
		{
			for (int i=0; i <poPolygons[j]->getNumInteriorRings() + 1; i++)
			{
				OGRLinearRing *poLR = (i==0) ? poPolygons[j]->getExteriorRing() : poPolygons[j]->getInteriorRing(i-1);
				for (int i = 0; i<poLR->getNumPoints();i++)
				{
					if (poLR->getX(i)<0)
						poLR->setPoint(i,4*D + poLR->getX(i),poLR->getY(i));
				}
			}
		}
	}
	delete[]poPolygons;

	return TRUE;
};


BOOL	VectorBorder::intersects(int tile_z, int tile_x, int tile_y)
{
	if (tile_x < (1<<tile_z)) return intersects(MercatorTileGrid::calcEnvelopeByTile(tile_z, tile_x, tile_y));
	else
	{
		if (!intersects(MercatorTileGrid::calcEnvelopeByTile(tile_z, tile_x, tile_y)))
			return intersects(MercatorTileGrid::calcEnvelopeByTile(tile_z, tile_x - (1<<tile_z), tile_y));
		else return TRUE;
	}
};


BOOL	VectorBorder::intersects(OGREnvelope &envelope)
{
	if (this->m_poGeometry == NULL) return FALSE;
	OGRPolygon *poPolygon = createOGRPolygonByOGREnvelope (envelope);
	BOOL bResult = this->m_poGeometry->Intersects(poPolygon);
	poPolygon->empty();
	return bResult;
}
	


OGREnvelope VectorBorder::getEnvelope ()
{
	OGREnvelope oEnvelope;
	if (this->m_poGeometry!=NULL) this->m_poGeometry->getEnvelope(&oEnvelope);
	return oEnvelope;
};



OGRPolygon*		VectorBorder::createOGRPolygonByOGREnvelope (OGREnvelope envelope)
{
	OGRPolygon *poPolygon = new OGRPolygon();

	OGRLinearRing	oLR;
	oLR.addPoint(envelope.MinX,envelope.MinY);
	oLR.addPoint(envelope.MinX,envelope.MaxY);
	oLR.addPoint(envelope.MaxX,envelope.MaxY);
	oLR.addPoint(envelope.MaxX,envelope.MinY);
	oLR.closeRings();
	poPolygon->addRing(&oLR);
	
	return poPolygon;
};


}