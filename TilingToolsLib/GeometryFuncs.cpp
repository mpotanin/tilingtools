#include "GeometryFuncs.h"

/*
BOOL CalcRectEnvelope(OGRGeometry	*poGeometry, double *min_x, double *min_y,double *max_x, double *max_y)
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


OGREnvelope CreateEnvelope (double minx,double miny,double maxx,double maxy)
{
	OGREnvelope	oEnvelope;
	oEnvelope.MaxX = maxx;
	oEnvelope.MaxY = maxy;
	oEnvelope.MinY = miny;
	oEnvelope.MinX = minx;
	return oEnvelope;
};


OGREnvelope	CombineEnvelopes (OGREnvelope	&oEnvelope1, OGREnvelope	&oEnvelope2)
{
	OGREnvelope oEnvelope;
	oEnvelope.MaxX = max(oEnvelope1.MaxX,oEnvelope2.MaxX);
	oEnvelope.MaxY = max(oEnvelope1.MaxY,oEnvelope2.MaxY);
	oEnvelope.MinX = min(oEnvelope1.MinX,oEnvelope2.MinX);
	oEnvelope.MinY = min(oEnvelope1.MinY,oEnvelope2.MinY);
	return oEnvelope;
}

OGREnvelope	InetersectEnvelopes (OGREnvelope	&oEnvelope1,OGREnvelope	&oEnvelope2)
{
	OGREnvelope oEnvelope;
	oEnvelope.MaxX = min(oEnvelope1.MaxX,oEnvelope2.MaxX);
	oEnvelope.MaxY = min(oEnvelope1.MaxY,oEnvelope2.MaxY);
	oEnvelope.MinX = max(oEnvelope1.MinX,oEnvelope2.MinX);
	oEnvelope.MinY = max(oEnvelope1.MinY,oEnvelope2.MinY);
	return oEnvelope;
}

double		CalcAreaOfEnvelope (OGREnvelope	oEnvelope)
{
	return ((oEnvelope.MaxX-oEnvelope.MinX)*(oEnvelope.MaxY-oEnvelope.MinY));
}


BOOL GeometryToArrayOfRings (OGRGeometry *poGeometry, int &n, OGRLinearRing** &poRings)
{
	n = 0;
	if (poGeometry == NULL) return FALSE;
	int nNumPolygons = 0;
	OGRPolygon **poPolygons = NULL;
	if (!GeometryToArrayOfPolygons(poGeometry,nNumPolygons,poPolygons)) return FALSE;

	for (int i=0;i<nNumPolygons;i++)
		n+=1+poPolygons[i]->getNumInteriorRings();
	
	poRings = new OGRLinearRing*[n];
	
	int j=0;
	for (int i=0;i<nNumPolygons;i++)
	{
		poRings[j] = poPolygons[i]->getExteriorRing();
		j++;
		for (int k=0;k<poPolygons[i]->getNumInteriorRings();k++)
		{
			poRings[j] = poPolygons[i]->getInteriorRing(k);
			j++;
		}
	}
	delete[]poPolygons;
	

	return TRUE;
}


BOOL GeometryToArrayOfPolygons (OGRGeometry *poGeometry, int &n, OGRPolygon** &poPolygons)
{
	n = 0;
	if (poGeometry == NULL) return FALSE;
	if (poGeometry->getGeometryType()==wkbPolygon)
	{
		n = 1;
		poPolygons = new OGRPolygon*[1];
		poPolygons[0] = (OGRPolygon*)poGeometry;
	}
	else if (poGeometry->getGeometryType()==wkbMultiPolygon)
	{
		n = ((OGRGeometryCollection*)poGeometry)->getNumGeometries();
		if (n == 0) return FALSE;
		poPolygons = new OGRPolygon*[n];
		for (int i=0;i<n;i++)
			poPolygons[i] = (OGRPolygon*)((OGRGeometryCollection*)poGeometry)->getGeometryRef(i);
	}
	return TRUE;
}