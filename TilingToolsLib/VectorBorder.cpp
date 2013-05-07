#include "StdAfx.h"
#include "VectorBorder.h"
namespace GMX
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



string VectorBorder::getVectorFileNameByRasterFileName (string strRasterFile)
{
	string	strVectorFileBase = RemoveExtension(strRasterFile);
	string	strVectorFile;
	
	if (FileExists(strVectorFileBase+".mif"))	strVectorFile = strVectorFileBase+".mif";
	if (FileExists(strVectorFileBase+".shp"))	strVectorFile = strVectorFileBase+".shp";
	if (FileExists(strVectorFileBase+".tab"))	strVectorFile = strVectorFileBase+".tab";
	
	if (strVectorFile!="")
	{
		OGRDataSource * poDS = OGRSFDriverRegistrar::Open( strVectorFile.c_str(), FALSE );
		if (poDS ==NULL) return "";
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

BOOL	VectorBorder::intersects180Degree (OGRGeometry	*poGeometry, OGRSpatialReference *poSR)
{
	int numRings;
	OGRLinearRing **poRings = VectorBorder::getLinearRingsRef(poGeometry,numRings);

	OGRSpatialReference	oWGS84;
	oWGS84.SetWellKnownGeogCS("WGS84");
	

	for (int i=0;i<numRings;i++)
	{
		for (int k=0;k<poRings[i]->getNumPoints()-1;k++)
		{
			OGRLineString oLS;
			for (double j=0;j<=1.0001;j+=0.1)
				oLS.addPoint(	poRings[i]->getX(k)*(1-j) + poRings[i]->getX(k+1)*j,
								poRings[i]->getY(k)*(1-j) + poRings[i]->getY(k+1)*j);
			if (k==9)
			{
				double x1 = poRings[0]->getX(9);
				double y1 = poRings[0]->getY(9);
				double x2 = poRings[0]->getX(10);
				double y2 = poRings[0]->getY(10);
				y2=y2;
			}
			oLS.assignSpatialReference(poSR);
			//ToDo
			OGRErr errorTransform =	oLS.transformTo(&oWGS84);
			
			
			for (int l=0;l<oLS.getNumPoints()-1;l++)
			{
				double x1 = oLS.getX(l);
				double y1 = oLS.getY(l);
				double x2 = oLS.getX(l+1);
				double y2 = oLS.getY(l+1);


				if(	((oLS.getX(l)>90)&&(oLS.getX(l+1)<-90)) || ((oLS.getX(l)<-90)&&(oLS.getX(l+1)>90)) ||
					((oLS.getX(l)>180.00001)&&(oLS.getX(l+1)<179.9999)) || ((oLS.getX(l)<179.9999)&&(oLS.getX(l+1)>180.00001)) || 
					((oLS.getX(l)>-179.9999)&&(oLS.getX(l+1)<-180.00001)) || ((oLS.getX(l)<-180.00001)&&(oLS.getX(l+1)>-179.9999)))
				{
					delete[]poRings;
					return TRUE;
				}
			}
		}
	}
	
	delete[]poRings;
	return FALSE;
}


VectorBorder*	VectorBorder::createFromVectorFile(string vectorFilePath, MercatorProjType	mercType)
{
	OGRDataSource *poDS= OGRSFDriverRegistrar::Open( vectorFilePath.c_str(), FALSE );
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
	BOOL	intersects180 = VectorBorder::intersects180Degree(poMultiPolygon,poInputSpatial);
	if (OGRERR_NONE != poMultiPolygon->transformTo(&oSpatialMerc))
	{
		poMultiPolygon->empty();
		return NULL;
	}
	if (intersects180) adjustFor180DegreeIntersection(poMultiPolygon);
		
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

OGRLinearRing**		VectorBorder::getLinearRingsRef	(OGRGeometry	*poGeometry, int &numRings)
{
	numRings = 0;
	OGRLinearRing	**poRings = NULL;
	OGRPolygon		**poPolygons = NULL;
	OGRwkbGeometryType type = poGeometry->getGeometryType();

	if ((type!=wkbMultiPolygon) && (type!=wkbMultiPolygon) && 
		(type!=wkbLinearRing) && (type!=wkbLineString)
		) return NULL;

	int	numPolygons = 0;
	BOOL	inputIsRing = FALSE;
	if (type==wkbPolygon)
	{
		numPolygons		= 1;
		poPolygons		= new OGRPolygon*[numPolygons];
		poPolygons[0]	= (OGRPolygon*)poGeometry;
	}
	else if (type==wkbMultiPolygon)
	{
		numPolygons		= ((OGRMultiPolygon*)poGeometry)->getNumGeometries();
		poPolygons		= new OGRPolygon*[numPolygons];
		for (int i=0;i<numPolygons;i++)
			poPolygons[i] = (OGRPolygon*)((OGRMultiPolygon*)poGeometry)->getGeometryRef(i);
	}

	if (numPolygons>0)
	{
		for (int i=0;i<numPolygons;i++)
		{
			numRings++;
			numRings+=poPolygons[i]->getNumInteriorRings();
		}
		poRings = new OGRLinearRing*[numRings];
		int j=0;
		for (int i=0;i<numPolygons;i++)
		{
			poRings[j] = poPolygons[i]->getExteriorRing();
			j++;
			for (int k=0;k<poPolygons[i]->getNumInteriorRings();k++)
			{
				poRings[j] = poPolygons[i]->getInteriorRing(k);
				j++;
			}
		}
	}
	else
	{
		numRings = 1;
		poRings = new OGRLinearRing*[numRings];
		poRings[0] = (OGRLinearRing*)poGeometry;
	}

	delete[]poPolygons;
	return poRings;
}


BOOL			VectorBorder::adjustFor180DegreeIntersection (OGRGeometry	*poMercGeometry)
{
	OGRLinearRing	**poRings;
	int numRings = 0;
	
	if (!(poRings = VectorBorder::getLinearRingsRef(poMercGeometry,numRings))) return FALSE;
	
	for (int i=0;i<numRings;i++)
	{
		for (int k=0;k<poRings[i]->getNumPoints();k++)
		{
				if (poRings[i]->getX(k)<0)
					poRings[i]->setPoint(k,-2*MercatorTileGrid::getULX() + poRings[i]->getX(k),poRings[i]->getY(k));
		}
	}
	
	delete[]poRings;
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