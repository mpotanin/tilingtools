#include "StdAfx.h"
#include "VectorBorder.h"

VectorBorder::VectorBorder (void)
{
	m_poGeometry	= NULL;
	OGRRegisterAll();
}

VectorBorder::VectorBorder(OGREnvelope &oEnvelope)
{
	m_poGeometry	= NULL;
	OGRRegisterAll();
	InitByEnvelope(oEnvelope);
}


VectorBorder::VectorBorder(VectorBorder *poPolygon)
{
	m_poGeometry	= NULL;
	if (poPolygon!=NULL) InitByOGRGeometry(poPolygon->GetOGRGeometry());
}

VectorBorder::VectorBorder(OGRGeometry	*poGeometry)
{
	m_poGeometry	= NULL;
	if (poGeometry!=NULL) InitByOGRGeometry(poGeometry);
}


void VectorBorder::delete_all()
{
	if (m_poGeometry!=NULL) m_poGeometry->empty();
	m_poGeometry	= NULL;
}

VectorBorder::~VectorBorder(void)
{
	delete_all();	
}




BOOL VectorBorder::InitByOGRGeometry (OGRGeometry *poGeometry_set)
{
	delete_all();
	if (poGeometry_set == NULL) return FALSE;
	m_poGeometry = poGeometry_set->clone();
	if (m_poGeometry==NULL) return FALSE;
	return TRUE;
};


OGRGeometry* VectorBorder::GetOGRGeometry ()
{
	return this->m_poGeometry;
};


OGREnvelope VectorBorder::GetEnvelope ()
{
	OGREnvelope oEnvelope;
	if (this->m_poGeometry!=NULL) this->m_poGeometry->getEnvelope(&oEnvelope);

	//this->
	return oEnvelope;
};

BOOL VectorBorder::InitByEnvelope (OGREnvelope oEnvelope)
{
	double points[8];
	points[0] = oEnvelope.MinX;
	points[1] = oEnvelope.MaxY;
	points[2] = oEnvelope.MaxX;
	points[3] = oEnvelope.MaxY;
	points[4] = oEnvelope.MaxX;
	points[5] = oEnvelope.MinY;
	points[6] = oEnvelope.MinX;
	points[7] = oEnvelope.MinY;
	return InitByPoints(4,points);
	//points[0] = oEnvelope.MinX;

};

BOOL VectorBorder::InitByPoints (int nNumOfPoints, double *dblPoints)
{
	delete_all();

	if (nNumOfPoints==0) return FALSE;
	OGRLinearRing	oLR;
	for (int i=0;i<nNumOfPoints;i++)
	{
		oLR.addPoint(dblPoints[2*i],dblPoints[2*i+1]);
	}
	oLR.closeRings();
	
	m_poGeometry = new OGRPolygon();
	((OGRPolygon*)m_poGeometry)->addRing(&oLR);// = new OGRPolygon;
	return TRUE;
};


BOOL VectorBorder::GetAllPoints (int &num, int* &nums, double** &arr)
{
	int n = 0;
	if (m_poGeometry==NULL) return FALSE;
	if (m_poGeometry->IsEmpty()) return FALSE;
	OGRLinearRing	**poRings;
	if (!GetOGRRings(n,poRings)) return FALSE;
	num = n;
	nums = new int[n];
	arr = new double*[n];
	for (int i=0;i<n;i++)
	{
		nums[i] = poRings[i]->getNumPoints();
		arr[i] = new double[2*nums[i]];
		for (int k=0;k<nums[i];k++)
		{
			arr[i][2*k]=poRings[i]->getX(k);
			arr[i][2*k+1]=poRings[i]->getY(k);
		}
	}


	delete[]poRings;

	return TRUE;
}

			


BOOL VectorBorder::AdjustBounds (double *min_x, double *min_y, double *max_x, double *max_y)
{
	if (this->m_poGeometry==NULL) return FALSE;
	if (Intersects(*min_x,*min_y,*max_x,*max_y))
	{
		OGREnvelope	oEnvelope = GetEnvelope ();
		(*max_x) = min((*max_x),oEnvelope.MaxX);
		(*min_x) = max((*min_x),oEnvelope.MinX);
		(*max_y) = min((*max_y),oEnvelope.MaxY);
		(*min_y) = max((*min_y),oEnvelope.MinY);
		return TRUE;
	} 
	
	return FALSE;
};


BOOL VectorBorder::AdjustBounds (	OGREnvelope &oEnvelope)
{
	return AdjustBounds(&oEnvelope.MinX,&oEnvelope.MinY,&oEnvelope.MaxX,&oEnvelope.MaxY);
};


int VectorBorder::Contains(double min_x, double min_y, double max_x, double max_y)
{
	if (this->m_poGeometry==NULL) return -1;
	if (this->m_poGeometry->IsEmpty()) return -1;

	//if (this->poPolygon == NULL) return -1;
	
	
	OGRPolygon  oTile;
	this->makePolygon(min_x,min_y,max_x,max_y,&oTile);

	if ( this->m_poGeometry->Contains(&oTile)) 
	{
		return 1;
	}
	else
	{
		return 0;
	}

};

int VectorBorder::Intersects(double min_x, double min_y, double max_x, double max_y)
{
	if (this->m_poGeometry==NULL) return -1;
	if (this->m_poGeometry->IsEmpty()) return -1;
	OGRPolygon  oTile;
	this->makePolygon(min_x,min_y,max_x,max_y,&oTile);

	if ( this->m_poGeometry->Intersects(&oTile)) 
	{
		return 1;
	}
	else
	{
		return 0;
	}
};


int VectorBorder::OnEdge(double min_x, double min_y, double max_x, double max_y)
{
	if (this->m_poGeometry==NULL) return -1;
	if (this->m_poGeometry->IsEmpty()) return -1;
	OGRPolygon  oTile;
	this->makePolygon(min_x,min_y,max_x,max_y,&oTile);

	if ( this->m_poGeometry->Intersects(&oTile)) 
	{
		if ( !this->m_poGeometry->Contains(&oTile)) 
		{
			return 1;
		}
	}

	return 0;
};


int VectorBorder::Contains(OGREnvelope &oEnvelope)
{
	if (this->m_poGeometry==NULL) return -1;
	VectorBorder oPolygon_;
	if (!oPolygon_.InitByEnvelope(oEnvelope)) return FALSE;
	if ( this->m_poGeometry->Contains(oPolygon_.GetOGRGeometry())) return 1;
	else return 0;
}

int VectorBorder::Intersects(OGREnvelope &oEnvelope)
{
	if (this->m_poGeometry==NULL) return -1;

	//OGRLinearRing oRing((OGRPolygon*)m_poGeometry->get

	/*
	OGRPolygon	*poPolygon = (OGRPolygon*)m_poGeometry;
	OGRLinearRing *poRing	= poPolygon->getExteriorRing();
	
	for (int i=0;i<poRing->getNumPoints();i++)
	{
		double x =	poRing->getX(i);
		double y =	poRing->getY(i);
		x = x;
	}
	*/

	VectorBorder oPolygon_;
	if (!oPolygon_.InitByEnvelope(oEnvelope)) return FALSE;
	if ( this->m_poGeometry->Intersects(oPolygon_.GetOGRGeometry())) return 1;
	else return 0;
}

int VectorBorder::OnEdge(OGREnvelope &oEnvelope)
{
	if (this->m_poGeometry==NULL) return -1;
	VectorBorder oPolygon_;
	if (!oPolygon_.InitByEnvelope(oEnvelope)) return FALSE;
	if ( this->m_poGeometry->Intersects(oPolygon_.GetOGRGeometry()) &&
		!this->m_poGeometry->Contains(oPolygon_.GetOGRGeometry())) return 1;
	else return 0;
}


double	VectorBorder::GetArea()
{
	if (this->m_poGeometry==NULL) return 0;
	switch (this->m_poGeometry->getGeometryType())
	{
		case wkbPolygon:
			return ((OGRPolygon*)this->m_poGeometry)->get_Area();
		case wkbMultiPolygon:
			return ((OGRMultiPolygon*)this->m_poGeometry)->get_Area();
		default: return 0;
	}
	return 0;
}



BOOL	VectorBorder::Intersection (OGREnvelope &oEnvelope, VectorBorder &oPolygon)
{
	VectorBorder oPolygon_;
	if (!oPolygon_.InitByEnvelope(oEnvelope)) return FALSE;
	OGRGeometry *poGeometry = this->m_poGeometry->Intersection(oPolygon_.GetOGRGeometry());
	if (poGeometry==NULL) return FALSE;
	if (!oPolygon.InitByOGRGeometry(poGeometry)) return FALSE;
	poGeometry->empty();
	return TRUE;
}


BOOL VectorBorder::Buffer (double dfDist)
{
	if (this->m_poGeometry==NULL) return FALSE;
	OGRPolygon **poPolygons = NULL;
	int n = 0;
	if (!GetOGRPolygons(n,poPolygons)) return FALSE;
	for (int i=0;i<n;i++)
		if (!BufferOfPolygon(dfDist,poPolygons[i])) {delete[]poPolygons; return FALSE;}
	delete[]poPolygons;
	return TRUE;
}


BOOL VectorBorder::BufferOfPolygon (double dfDist, OGRPolygon *poPolygon)
{
	OGRLinearRing *poRing = poPolygon->getExteriorRing();
	OGRLineString oBufPoints;
	double E = 1e-5;
	OGRLineString oLSbuf;

	for (int i=0;i<poRing->getNumPoints()-1;i++)
	{
		OGRPoint oP2,oP,oP1; 
		poRing->getPoint(i,&oP);
		poRing->getPoint(i+1,&oP2);
		if (i==0)	poRing->getPoint(poRing->getNumPoints()-2,&oP1);
		else		poRing->getPoint(i-1,&oP1);
		if ((distance(oP,oP2)<E)&&(distance(oP,oP1)<E)) continue;
		double k = distance(oP,oP1)/(distance(oP,oP2)+distance(oP,oP1));
		
		OGRPoint oPbuf1(oP1.getX()*(1-k)+oP2.getX()*k,oP1.getY()*(1-k)+oP2.getY()*k);
		double k2 = (dfDist/distance(oPbuf1,oP));
		oPbuf1.setX(oP.getX()+(oPbuf1.getX()-oP.getX())*k2);
		oPbuf1.setY(oP.getY()+(oPbuf1.getY()-oP.getY())*k2);

		
		OGRPoint oPbuf2(2*oP.getX()-oPbuf1.getX(),2*oP.getY()-oPbuf1.getY());
		if (poPolygon->Contains(&oPbuf1)) oLSbuf.addPoint(&oPbuf1);
		else if (poPolygon->Contains(&oPbuf2)) oLSbuf.addPoint(&oPbuf2);
		else continue;
	};
	if (oLSbuf.getNumPoints()<4) return FALSE;
	//poPolygon->empty();

	//OGRLinearRing oLR;
	//oLR.addSubLineString(&oLSbuf);
	//oLR.closeRings();
	//poPolygon->addRing(&oLR);
	poRing->empty();
	poRing->addSubLineString(&oLSbuf);
	poRing->closeRings();
	return TRUE;
};



double VectorBorder::distance (OGRPoint oP1, OGRPoint oP2)
{
	return sqrt((oP1.getX()-oP2.getX())*(oP1.getX()-oP2.getX())+(oP1.getY()-oP2.getY())*(oP1.getY()-oP2.getY()));
};



void VectorBorder::makePolygon (int n, double *arr,OGRPolygon	*poPolygon)
{
	//OGRLinearRing *poLR = NULL;
	OGRLinearRing oLR;
	

	//poLR = new OGRLinearRing();

	for (int i=0; i<n;i++)
		oLR.addPoint(arr[2*i],arr[2*i+1]);
	oLR.closeRings();

	poPolygon->addRing(&oLR);
};

	
void VectorBorder::makePolygon (double min_x, double min_y, double max_x, double max_y, OGRPolygon	*poPolygon)
{
	double arr[8];
	arr[0] = min_x;arr[1]=min_y;arr[2] = min_x;arr[3]=max_y;
	arr[4] = max_x;arr[5]=max_y;arr[6] = max_x;arr[7]=min_y;
	return makePolygon(4,arr,poPolygon);
};


/*
BOOL VectorBorder::Geometry2ArrayOfRings (OGRGeometry *poGeometry, int &n, OGRLinearRing** &poRings)
{
	n = 0;
	if (poGeometry == NULL) return FALSE;
	int nNumPolygons = 0;
	OGRPolygon **poPolygons = NULL;
	if (!VectorBorder::Geometry2ArrayOfPolygons(poGeometry,nNumPolygons,poPolygons)) return FALSE;

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
*/


BOOL	VectorBorder::GetOGRRings		(int &num,	OGRLinearRing** &poRings)
{
	num = 0;
	if (m_poGeometry == NULL) return FALSE;
	int nNumPolygons = 0;
	OGRPolygon **poPolygons = NULL;
	if (!VectorBorder::GetOGRPolygons(nNumPolygons,poPolygons)) return FALSE;

	for (int i=0;i<nNumPolygons;i++)
		num+=1+poPolygons[i]->getNumInteriorRings();
	
	poRings = new OGRLinearRing*[num];
	
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



BOOL	VectorBorder::GetOGRPolygons	(int &num,	OGRPolygon**	&poPolygons)
{
	num = 0;
	if (m_poGeometry == NULL) return FALSE;
	if (m_poGeometry->getGeometryType()==wkbPolygon)
	{
		num = 1;
		poPolygons = new OGRPolygon*[1];
		poPolygons[0] = (OGRPolygon*)m_poGeometry;
	}
	else if (m_poGeometry->getGeometryType()==wkbMultiPolygon)
	{
		num = ((OGRGeometryCollection*)m_poGeometry)->getNumGeometries();
		if (num == 0) return FALSE;
		poPolygons = new OGRPolygon*[num];
		for (int i=0;i<num;i++)
			poPolygons[i] = (OGRPolygon*)((OGRGeometryCollection*)m_poGeometry)->getGeometryRef(i);
	}
	return TRUE;
}

//*/


double* VectorBorder::CalcHorizontalLineIntersection (double y, int &nNumOfPoints)
{
	//list<double> oPoints_;
	if (this->m_poGeometry==NULL) return NULL;
	if (this->m_poGeometry->IsEmpty()) return NULL;
	nNumOfPoints =0;
	
	int nMaxPoints = 0;
	int nNumOfRings = 0;
	OGRPolygon **poPolygons = NULL;
	int nNumOfPolygons = 0;
	double	*pPoints = NULL;


	if (!GetOGRPolygons(nNumOfPolygons,poPolygons)) return NULL;
	
	OGRLinearRing	**poRings = NULL;
	if (!GetOGRRings(nNumOfRings,poRings)) return FALSE;
	for (int i=0;i<nNumOfRings;i++)
		nMaxPoints+=poRings[i]->getNumPoints();
	
	pPoints = new double[nMaxPoints];

	for (int j=0;j<nNumOfRings;j++)
	{
		OGRPoint	p0,p1;
		double t,x,x1,x2,y1,y2;
		for (int i=0;i<poRings[j]->getNumPoints()-1;i++)
		{
			if (i==0) poRings[j]->getPoint(0,&p0);
			else {p0.setX(p1.getX());p0.setY(p1.getY());}
			poRings[j]->getPoint(i+1,&p1);
			x1 = p0.getX();x2=p1.getX();
			y1=p0.getY();  y2=p1.getY();
			t = (y1-y)*(y2-y);
			if (t<0)
			{
				x = (x2*(y1-y)-x1*(y2-y))/(y1-y2);
				pPoints[nNumOfPoints]=x;
				nNumOfPoints++;
			}
			else if (t==0)
			{
				if (y1+y2-2*y==0)
				{
					//oPoints.push_back(x1);
					pPoints[nNumOfPoints]=x2;
					nNumOfPoints++;
				}
				else if (y2==y)
				{
					pPoints[nNumOfPoints]=x2;
					nNumOfPoints++;
					if (i == poRings[j]->getNumPoints()-2) 
					{
						poRings[j]->getPoint(1,&p0);
						t = p0.getY()-y2;
					}
					else
					{
						poRings[j]->getPoint(i+2,&p0);
						t = p0.getY()-y2;
					}
					if ((y2-y1)*t<0) 
					{
						pPoints[nNumOfPoints]=x2;
						nNumOfPoints++;
					}
				}
			}
		}
	}
	delete[]poPolygons;
	delete[]poRings;


	return pPoints;
};



