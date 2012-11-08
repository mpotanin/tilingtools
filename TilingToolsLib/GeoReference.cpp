#include "stdafx.h"
#include "GeoReference.h"
#include "FileSystemFuncs.h"
#include "str.h"

GeoReference::GeoReference ()
{
	M_PI = 3.14159265358979;
	this->SetWGS84Datum();
	this->SetMercatorProjection();
	/*
	MERCATOR		= "MERCATOR";
	UTM				= "UTM";
	GAUSS_KRUGER	= "GAUSS-KRUGER";
	WGS84			= "WGS84";
	KRASOVSKY_1940	= "Krasovsky_1940";
	*/
}
GeoReference::~GeoReference(){}
double GeoReference::deg_rad(double ang)
{
	return ang * (M_PI/180.0);
}

double GeoReference::deg_decimal(double rad)
{ 
	return (rad/M_PI) * 180.0;
}

double GeoReference::merc_x(double lon)
{
	//double r_major = 6378137.000;
	double r_major = this->sm_a;
	return r_major * deg_rad(lon);
}


double GeoReference::from_merc_x(double x)
{
	//double r_major = 6378137.000;
	double r_major = this->sm_a;
	return deg_decimal(x/r_major);
}

	//	Converts from latitude to y coordinate 
double GeoReference::merc_y(double lat)
{
	if (lat > 89.5)
		lat = 89.5;
	if (lat < -89.5)
		lat = -89.5;
	//double r_major = 6378137.000;
	double r_major = this->sm_a;
	//double r_minor = 6356752.3142;
	double r_minor = this->sm_b;

	double temp = r_minor / r_major;
	double es = 1.0 - (temp * temp);
	double eccent = sqrt(es);
	double phi = deg_rad(lat);
	double sinphi = sin(phi);
	double con = eccent * sinphi;
	double com = .5 * eccent;
	con = pow(((1.0-con)/(1.0+con)), com);
	double ts = tan(.5 * ((M_PI*0.5) - phi))/con;
	double y = 0 - r_major * log(ts);
	return y;
}



	//	Converts from y coordinate to latitude 
double GeoReference::from_merc_y (double y)
{
	//double r_major = 6378137.000;
	double r_major = this->sm_a;
	//double r_minor = 6356752.3142;
	double r_minor = this->sm_b;


	double temp = r_minor / r_major;
	double es = 1.0 - (temp * temp);
	double eccent = sqrt(es);
	double ts = exp(-y/r_major);
	double HALFPI = 1.5707963267948966;

	double eccnth, Phi, con, dphi;
	eccnth = 0.5 * eccent;

	Phi = HALFPI - 2.0 * atan(ts);

	double N_ITER = 15;
	double TOL = 1e-7;
	double i = N_ITER;
	dphi = 0.1;
	while ((fabs(dphi)>TOL)&&(--i>0))
	{
		con = eccent * sin (Phi);
		dphi = HALFPI - 2.0 * atan(ts * pow((1.0 - con)/(1.0 + con), eccnth)) - Phi;
		Phi += dphi;
	}

	return deg_decimal(Phi);
}



void GeoReference::merc(double lon, double lat, double &x, double &y)
{
	x = merc_x(lon);
	y = merc_y(lat);
}


void GeoReference::from_merc(double x, double y, double &lon, double &lat) 
{
	lon = from_merc_x(x);
	lat = from_merc_y(y);
}


double GeoReference::DegToRad (double deg)
{
	return (deg / 180.0 * pi);
}


double GeoReference::RadToDeg (double rad)
{
	return (rad / pi * 180.0);
}



double GeoReference::ArcLengthOfMeridian (double phi)
{
	double alpha, beta, gamma, delta, epsilon, n;
	double result;

	/* Precalculate n */
	n = (sm_a - sm_b) / (sm_a + sm_b);

	/* Precalculate alpha */
	alpha = ((sm_a + sm_b) / 2.0)
	   * (1.0 + (pow (n, 2.0) / 4.0) + (pow (n, 4.0) / 64.0));

	/* Precalculate beta */
	beta = (-3.0 * n / 2.0) + (9.0 * pow (n, 3.0) / 16.0)
	   + (-3.0 * pow (n, 5.0) / 32.0);

	/* Precalculate gamma */
	gamma = (15.0 * pow (n, 2.0) / 16.0)
		+ (-15.0 * pow (n, 4.0) / 32.0);

	/* Precalculate delta */
	delta = (-35.0 * pow (n, 3.0) / 48.0)
		+ (105.0 * pow (n, 5.0) / 256.0);

	/* Precalculate epsilon */
	epsilon = (315.0 * pow (n, 4.0) / 512.0);

/* Now calculate the sum of the series and return */
result = alpha
	* (phi + (beta * sin (2.0 * phi))
		+ (gamma * sin (4.0 * phi))
		+ (delta * sin (6.0 * phi))
		+ (epsilon * sin (8.0 * phi)));

	return result;
}

double GeoReference::UTMCentralMeridian (double zone)
{
	double cmeridian;

	cmeridian = DegToRad (zone_adjust + (zone * 6.0));
	
	return cmeridian;
}

double GeoReference::FootpointLatitude (double y)
{
	double y_, alpha_, beta_, gamma_, delta_, epsilon_, n;
	double result;
    
	/* Precalculate n (Eq. 10.18) */
	n = (sm_a - sm_b) / (sm_a + sm_b);
    	
	/* Precalculate alpha_ (Eq. 10.22) */
	/* (Same as alpha in Eq. 10.17) */
	alpha_ = ((sm_a + sm_b) / 2.0)
		* (1 + (pow (n, 2.0) / 4) + (pow (n, 4.0) / 64));
    
	/* Precalculate y_ (Eq. 10.23) */
	y_ = y / alpha_;
    
	/* Precalculate beta_ (Eq. 10.22) */
	beta_ = (3.0 * n / 2.0) + (-27.0 * pow (n, 3.0) / 32.0)
		+ (269.0 * pow (n, 5.0) / 512.0);
    
	/* Precalculate gamma_ (Eq. 10.22) */
	gamma_ = (21.0 * pow (n, 2.0) / 16.0)
		+ (-55.0 * pow (n, 4.0) / 32.0);
    	
	/* Precalculate delta_ (Eq. 10.22) */
	delta_ = (151.0 * pow (n, 3.0) / 96.0)
		+ (-417.0 * pow (n, 5.0) / 128.0);
    	
	/* Precalculate epsilon_ (Eq. 10.22) */
	epsilon_ = (1097.0 * pow (n, 4.0) / 512.0);
    	
	/* Now calculate the sum of the series (Eq. 10.21) */
	result = y_ + (beta_ * sin (2.0 * y_))
		+ (gamma_ * sin (4.0 * y_))
		+ (delta_ * sin (6.0 * y_))
		+ (epsilon_ * sin (8.0 * y_));
    
	return result;
}

void GeoReference::MapLatLonToXY (double phi, double lambda, double lambda0, double &x, double &y)
{
	double N, nu2, ep2, t, t2, l;
	double l3coef, l4coef, l5coef, l6coef, l7coef, l8coef;
	double tmp;

	/* Precalculate ep2 */
	ep2 = (pow (sm_a, 2.0) - pow (sm_b, 2.0)) / pow (sm_b, 2.0);

	/* Precalculate nu2 */
	nu2 = ep2 * pow (cos (phi), 2.0);

	/* Precalculate N */
	N = pow (sm_a, 2.0) / (sm_b * sqrt (1 + nu2));

	/* Precalculate t */
	t = tan (phi);
	t2 = t * t;
	tmp = (t2 * t2 * t2) - pow (t, 6.0);

	/* Precalculate l */
	l = lambda - lambda0;

	/* Precalculate coefficients for l**n in the equations below
	   so a normal human being can read the expressions for easting
	   and northing
	   -- l**1 and l**2 have coefficients of 1.0 */
	l3coef = 1.0 - t2 + nu2;

	l4coef = 5.0 - t2 + 9 * nu2 + 4.0 * (nu2 * nu2);

	l5coef = 5.0 - 18.0 * t2 + (t2 * t2) + 14.0 * nu2
		- 58.0 * t2 * nu2;

	l6coef = 61.0 - 58.0 * t2 + (t2 * t2) + 270.0 * nu2
		- 330.0 * t2 * nu2;

	l7coef = 61.0 - 479.0 * t2 + 179.0 * (t2 * t2) - (t2 * t2 * t2);

	l8coef = 1385.0 - 3111.0 * t2 + 543.0 * (t2 * t2) - (t2 * t2 * t2);

	/* Calculate easting (x) */
	x = N * cos (phi) * l
		+ (N / 6.0 * pow (cos (phi), 3.0) * l3coef * pow (l, 3.0))
		+ (N / 120.0 * pow (cos (phi), 5.0) * l5coef * pow (l, 5.0))
		+ (N / 5040.0 * pow (cos (phi), 7.0) * l7coef * pow (l, 7.0));

	/* Calculate northing (y) */
	y = ArcLengthOfMeridian (phi)
		+ (t / 2.0 * N * pow (cos (phi), 2.0) * pow (l, 2.0))
		+ (t / 24.0 * N * pow (cos (phi), 4.0) * l4coef * pow (l, 4.0))
		+ (t / 720.0 * N * pow (cos (phi), 6.0) * l6coef * pow (l, 6.0))
		+ (t / 40320.0 * N * pow (cos (phi), 8.0) * l8coef * pow (l, 8.0));

	return;
}
	    
	    
void GeoReference::MapXYToLatLon (double x, double y, double lambda0, double &phi, double &lambda)
{
	double phif, Nf, Nfpow, nuf2, ep2, tf, tf2, tf4, cf;
	double x1frac, x2frac, x3frac, x4frac, x5frac, x6frac, x7frac, x8frac;
	double x2poly, x3poly, x4poly, x5poly, x6poly, x7poly, x8poly;
	
	/* Get the value of phif, the footpoint latitude. */
	phif = FootpointLatitude (y);
    	
	/* Precalculate ep2 */
	ep2 = (pow (sm_a, 2.0) - pow (sm_b, 2.0))
		  / pow (sm_b, 2.0);
    	
	/* Precalculate cos (phif) */
	cf = cos (phif);
    	
	/* Precalculate nuf2 */
	nuf2 = ep2 * pow (cf, 2.0);
    	
	/* Precalculate Nf and initialize Nfpow */
	Nf = pow (sm_a, 2.0) / (sm_b * sqrt (1 + nuf2));
	Nfpow = Nf;
    	
	/* Precalculate tf */
	tf = tan (phif);
	tf2 = tf * tf;
	tf4 = tf2 * tf2;
    
	/* Precalculate fractional coefficients for x**n in the equations
	   below to simplify the expressions for latitude and longitude. */
	x1frac = 1.0 / (Nfpow * cf);
    
	Nfpow *= Nf;   /* now equals Nf**2) */
	x2frac = tf / (2.0 * Nfpow);
    
	Nfpow *= Nf;   /* now equals Nf**3) */
	x3frac = 1.0 / (6.0 * Nfpow * cf);
    
	Nfpow *= Nf;   /* now equals Nf**4) */
	x4frac = tf / (24.0 * Nfpow);
    
	Nfpow *= Nf;   /* now equals Nf**5) */
	x5frac = 1.0 / (120.0 * Nfpow * cf);
    
	Nfpow *= Nf;   /* now equals Nf**6) */
	x6frac = tf / (720.0 * Nfpow);
    
	Nfpow *= Nf;   /* now equals Nf**7) */
	x7frac = 1.0 / (5040.0 * Nfpow * cf);
    
	Nfpow *= Nf;   /* now equals Nf**8) */
	x8frac = tf / (40320.0 * Nfpow);
    
	/* Precalculate polynomial coefficients for x**n.
	   -- x**1 does not have a polynomial coefficient. */
	x2poly = -1.0 - nuf2;
    
	x3poly = -1.0 - 2 * tf2 - nuf2;
    
	x4poly = 5.0 + 3.0 * tf2 + 6.0 * nuf2 - 6.0 * tf2 * nuf2
		- 3.0 * (nuf2 *nuf2) - 9.0 * tf2 * (nuf2 * nuf2);
    
	x5poly = 5.0 + 28.0 * tf2 + 24.0 * tf4 + 6.0 * nuf2 + 8.0 * tf2 * nuf2;
    
	x6poly = -61.0 - 90.0 * tf2 - 45.0 * tf4 - 107.0 * nuf2
		+ 162.0 * tf2 * nuf2;
    
	x7poly = -61.0 - 662.0 * tf2 - 1320.0 * tf4 - 720.0 * (tf4 * tf2);
    
	x8poly = 1385.0 + 3633.0 * tf2 + 4095.0 * tf4 + 1575 * (tf4 * tf2);
    	
	/* Calculate latitude */
	phi = phif + x2frac * x2poly * (x * x)
		+ x4frac * x4poly * pow (x, 4.0)
		+ x6frac * x6poly * pow (x, 6.0)
		+ x8frac * x8poly * pow (x, 8.0);
    	
	/* Calculate longitude */
	lambda = lambda0 + x1frac * x
		+ x3frac * x3poly * pow (x, 3.0)
		+ x5frac * x5poly * pow (x, 5.0)
		+ x7frac * x7poly * pow (x, 7.0);
    	
	return;
}

double GeoReference::LatLonToUTMXY (double lat, double lon, double zone, double &x, double &y)
{
	MapLatLonToXY (lat, lon, UTMCentralMeridian (zone), x, y);

	/* Adjust easting and northing for UTM system. */
	x = x * UTMScaleFactor + easting;
	y = y * UTMScaleFactor;
	if (y < 0.0)
		y = y + 10000000.0;

	return zone;
}


void GeoReference::UTMXYToLatLon (double x, double y, double zone, double southhemi, double &lat, double &lon)
{
	double cmeridian;
    	
	x -= easting;
	x /= UTMScaleFactor;
    	
	/* If in southern hemisphere, adjust y accordingly. */
	if (southhemi)
	y -= 10000000.0;
    		
	y /= UTMScaleFactor;
    
	cmeridian = UTMCentralMeridian (zone);
	MapXYToLatLon (x, y, cmeridian, lat, lon);
    	
	return;
}


void GeoReference::SetMercatorProjection ()
{
	this->m_Projection = this->MERCATOR;
}

void GeoReference::SetUTMProjection ()
{
	UTMScaleFactor = 0.9996;
	easting		= 500000;
	zone_adjust = -183.0;
	this->m_Projection = this->UTM;
}

void GeoReference::SetGKProjection ()
{
	UTMScaleFactor = 1.0;
	easting		= 7500000;
	zone_adjust = -3.0;
	this->m_Projection = this->GAUSS_KRUGER;
}

void GeoReference::SetKrassovskyDatum ()
{
	sm_a = 6378245.0;
	sm_b = 6356863.019;
	sm_EccSquared = 6.693421552039e-03;
	this->m_Datum = this->KRASSOVSKY;
	//this->m_strDatum = this->KRASOVSKY_1940;
	//UTMScaleFactor = 1.;
	//bWGS84Datum = FALSE;
}

void GeoReference::SetWGS84Datum() 
{
	sm_a = 6378137.0;
	sm_b = 6356752.314;
	sm_EccSquared = 6.69437999013e-03;
	this->m_Datum = this->WGS84;
	//this->m_strDatum = this->WGS84;

	//UTMScaleFactor = 0.9996;
	//bWGS84Datum = TRUE;
}

void GeoReference::SetProjection  (Projection ProjName, int nZone)
{
	if (ProjName == this->UTM )
	{
		//this->m_Projection = this->UTM;
		//this->m_strProjection = this->UTM;
		if (nZone>0) this->m_nZone = nZone;
		else		this->m_nZone = 37;
		this->SetUTMProjection();
	}
	else if (ProjName == this->GAUSS_KRUGER)
	{
		//this->m_Projection = this->GAUSS_KRUGER;
		//this->m_strProjection = this->GAUSS_KRUGER;
		if (nZone>0) this->m_nZone = nZone;
		else		this->m_nZone = 7;
		this->SetGKProjection();
	}
	else
	{
		this->SetMercatorProjection();
		//this->m_Projection = this->MERCATOR;
		//this->m_strProjection = this->MERCATOR;
	}
}

void GeoReference::SetDatum		(Datum DatumName)
{
	if (DatumName == this->WGS84)
	{
		this->m_Datum = this->WGS84;
	}
	else
	{
		this->m_Datum = this->KRASSOVSKY;
	}
}

void GeoReference::ToLatLon(double x, double y, double &lon, double &lat)
{
	if (this->m_Projection == this->MERCATOR)
	{
		//tthis->
		this->from_merc(x,y,lon,lat);
	}
	else if (this->m_Projection == this->UTM)
	{
		this->UTMXYToLatLon(x,y,this->m_nZone,0,lat,lon);
	}
}

void GeoReference::ToXY	(double lon, double lat, double &x, double &y)
{
	if (this->m_Projection == this->MERCATOR)
	{
		this->merc(lon,lat,x,y);
	}
	else if (this->m_Projection == this->UTM)
	{
		this->LatLonToUTMXY(lat,lon,this->m_nZone,x,y);
	}
}

int GeoReference::CalcZoneNum	(double lon)
{
	return (int)floor(((lon - this->zone_adjust)/6) +0.5); 	
}

int GeoReference::GetZoneNum ()
{
	return this->m_nZone;
}

int GeoReference::CalcCentrMrdn	(int nZone)
{
	return (int)(this->zone_adjust + nZone*6);		
}

BOOL GeoReference::WriteTabFile	(wstring strRasterFileName, int nWidth, int nHeight, double dULx, double dULy, double dRes, wstring strTabFileName)
{


	//wstring strAdd;
	_TCHAR	buf[256];
	wstring prj_txt = L"!table\n!version 300\n!Charset WindowsLatin1\n\n";

	prj_txt+=L"Definition Table\n";
	prj_txt+=L"  File\"";
	prj_txt+=strRasterFileName + L"\"\n";
	//prj_txt+=strRasterFileName;
	//prj_txt+="\"\n";
	prj_txt+=L"  Type \"RASTER\"\n";

	swprintf(buf,L"(%lf,%lf) (0,0) Label \"UpLeft\",\n",dULx,dULy);
	
	prj_txt+=buf;
	
	swprintf(buf,L"(%lf,%lf) (%d,0) Label \"UpRight\",\n",dULx+dRes*nWidth,dULy,nWidth);
	prj_txt+=buf;

	swprintf(buf,L"(%lf,%lf) (0,%d) Label \"BottLeft\",\n",dULx,dULy-dRes*nHeight,nHeight);
	prj_txt+=buf;

	swprintf(buf,L"(%lf,%lf) (%d,%d) Label \"BottRight\"\n",dULx+dRes*nWidth,dULy-dRes*nHeight,nWidth,nHeight);
	prj_txt+=buf;

	prj_txt+= L"  CoordSys Earth Projection ";
	if (this->m_Projection == this->MERCATOR) 
	{
		prj_txt+= L"10, ";
	}
	else
	{
		prj_txt+=L"8, ";
	}

	if (this->m_Datum == this->WGS84)
	{
		prj_txt+=L"104, \"m\", ";
	}
	else
	{
		prj_txt+=L"9999, 3, 24, -123, -94, 0.02, -0.25, -0.13, 1.1, 0, \"m\", ";
	}

	if (this->m_Projection == this->MERCATOR)
	{
		prj_txt+=L"0\n";
	}
	else
	{
		swprintf(buf,L"%d, 0, %lf, %lf, 0\n",CalcCentrMrdn(this->m_nZone),this->UTMScaleFactor,this->easting);
		prj_txt+=buf;
	}

	return WriteStringToFile(strTabFileName,prj_txt);
}


BOOL GeoReference::WriteWldFile	(double dULx, double dULy, double dRes, wstring strWldFileName)
{
	_TCHAR buf[256];
	//_T ( "%.1f" )
	swprintf(buf,L"%lf\n0.0\n0.0\n%lf\n%lf\n%lf",dRes,-dRes,dULx,dULy);
	return WriteStringToFile(strWldFileName,buf);
}

BOOL GeoReference::WriteMapFile	(wstring strRasterFileName, int nWidth, int nHeight, double dULx, double dULy, double dRes, wstring strMapFileName)
{
	if (this->m_Datum!=this->WGS84) return FALSE;
	if ((this->m_Projection!=this->MERCATOR)&&(this->m_Projection!=this->UTM)) return FALSE;

	wstring strMapFile;
	
	strMapFile = L"OziExplorer Map Data File Version 2.2\n";
	strMapFile+= strRasterFileName + L"\n";
	strMapFile+= strRasterFileName + L"\n";
	strMapFile+= L"1 ,Map Code,\nWGS 84,WGS 84,   0.0000,   0.0000,WGS 84\nReserved 1\nReserved 2\nMagnetic Variation,,,E\n";
	
	if (this->m_Projection==this->MERCATOR)
	{
		strMapFile+=L"Map Projection,Mercator,PolyCal,No,AutoCalOnly,No,BSBUseWPX,No\n";
		const int nPointsInRow = 3;
		const int nPointsInColumn = 3;
		const int nPoints = nPointsInRow*nPointsInColumn;
		int num[nPoints];
		double dblLat[nPoints];double dblLon[nPoints];
		int nx[nPoints];int ny[nPoints];
		_TCHAR buf[1000];
		for (int i=0;i<nPointsInColumn;i++)
		{
			for (int j=0;j<nPointsInRow;j++)
			{
				if ((i+1)*(j+1)<10) swprintf(buf,L"Point0%d ,xy, ",(i+1)*(j+1));
				else swprintf(buf,L"Point%d ,xy, ",(i+1)*(j+1));
				strMapFile+=buf;
				nx[i*nPointsInRow+j] = ((nWidth-1)/(nPointsInRow-1))*j;
				ny[i*nPointsInRow+j] = ((nHeight-1)/(nPointsInColumn-1))*i;
				this->ToLatLon(dULx+dRes*nx[i*nPointsInRow+j],dULy-dRes*ny[i*nPointsInRow+j],dblLon[i*nPointsInRow+j],dblLat[i*nPointsInRow+j]);
				swprintf(buf,L"%d, %d,in, deg,  ",nx[i*nPointsInRow+j],ny[i*nPointsInRow+j]);
				strMapFile+=buf;
				int nLatDeg; int nLonDeg;
				wstring strN_S;wstring strE_W;
				nLatDeg = (int)floor(fabs(dblLat[i*nPointsInRow+j]));
				nLonDeg = (int)floor(fabs(dblLon[i*nPointsInRow+j]));
				(dblLat[i*nPointsInRow+j]>=0) ? strN_S=L"N" : strN_S=L"S";
				(dblLon[i*nPointsInRow+j]>=0) ? strE_W=L"E" : strE_W=L"W";
				double dblLatMinDec = (fabs(dblLat[i*nPointsInRow+j]) - nLatDeg)*60;
				double dblLonMinDec = (fabs(dblLon[i*nPointsInRow+j]) - nLonDeg)*60;
				swprintf(buf,L"%d, %lf,%s, %d, %lf,%s, grid,   ,           ,           ,N\n",nLatDeg,dblLatMinDec,strN_S.data(),nLonDeg,dblLonMinDec,strE_W.data());
				strMapFile+=buf;
			}
		}
		for (int i = (nPointsInRow*nPointsInColumn);i<30;i++)
		{
			if (i+1<10) swprintf(buf,L"Point0%d ,xy, ",i+1);
			else swprintf(buf,L"Point%d ,xy, ",i+1);
			strMapFile+=buf;
			strMapFile+=L"    ,     ,in, deg,    ,        ,,    ,        ,, grid,   ,           ,           ,\n";
		}
		strMapFile+=L"Projection Setup,,,,,,,,,,\n";
		strMapFile+=L"Map Feature = MF ; Map Comment = MC     These follow if they exist\n";
		strMapFile+=L"Track File = TF      These follow if they exist\n";
		strMapFile+=L"Moving Map Parameters = MM?    These follow if they exist\n";
		strMapFile+=L"Moving Map Parameters = MM?    These follow if they exist\n";
		strMapFile+=L"MM0,Yes\nMMPNUM,4\n";
		int n=1;
		for (int i=0; i<nPointsInColumn;i+=nPointsInColumn-1)
		{
			for (int j=0; j<nPointsInRow;j+=nPointsInRow-1)
			{
				swprintf(buf,L"MMPXY,%d,%d,%d\n",n,nx[i*nPointsInRow+j],ny[i*nPointsInRow+j]);
				strMapFile+=buf;
				n++;
			}
		}
		n=1;
		for (int i=0; i<nPointsInColumn;i+=nPointsInColumn-1)
		{
			for (int j=0; j<nPointsInRow;j+=nPointsInRow-1)
			{
				swprintf(buf,L"MMMPLL,%d, %lf, %lf\n",n,dblLat[i*nPointsInRow+j],dblLon[i*nPointsInRow+j]);
				strMapFile+=buf;
				n++;
			}
		}
		swprintf(buf,L"MM1B,%lf\n",dRes);
		strMapFile+=buf;
		strMapFile+=L"MOP,Map Open Position,0,0\n";
		swprintf(buf,L"IWH,Map Image Width/Height,%d,%d\n",nWidth,nHeight);
		strMapFile+=buf;
	}
	/*
	else if (this->m_Projection == this->UTM)
	{
		
		num[0]=1;	num[1]=2;	num[2]=3;	num[3] = 4;
		int nx[4];	int ny[4];  
		nx[0] =0; ny[0]=0; nx[1] = nWidth-1; ny[1] = 0; nx[2] = nWidth-1; ny[2] = nHeight-1; nx[3] =0; ny[3]=nHeight-1; 
	
		double dBRx = dULx + dRes*(nWidth-1);
		double dBRy = dULy - dRes*(nHeight-1);
		int x[4]; int y[4]; double xx[4]; double yy[4];
		double lat[4]; double lon[4];

		strMapFile+="Map Projection,(UTM) Universal Transverse Mercator,PolyCal,No,AutoCalOnly,No,BSBUseWPX,No\n";
		this->ToLatLon(dULx,dULy,lon[0],lat[0]);
		this->ToLatLon(dBRx,dBRy,lon[2],lat[2]);
		lon[1]=lon[2];lon[3]=lon[0];
		lat[1]=lat[0];lat[3]=lat[2];
		x[0] = floor(dULx+0.5); y[0]=floor(dULy+0.5);
		x[2] = floor(dBRx+0.5); y[2]=floor(dBRy+0.5);
		x[1]=x[2];x[3]=x[0];y[1]=y[0];y[3]=y[2];
		for (int i=0;i<4;i++)
		{
			wstring str;
			str.Format("Point0%d,xy,\t%d,%d,\tin,deg,,,N,,,E,grid,\t%d,%d,%d,N\n",num[i],nx[i],ny[i],this->m_nZone,x[i],y[i]);
			strMapFile+=str;
		}
	}

	strMapFile+="Projection Setup,     0.000000000,  -120.000000000,     0.999600000,            0.00,     -4000000.00,    34.000000000,    40.500000000,,,\n";
	strMapFile+="Map Feature = MF ; Map Comment = MC     These follow if they exist\n";
	strMapFile+="Track File = TF      These follow if they exist\n";
	strMapFile+="MM0,Yes\n";
	strMapFile+="MMPNUM,4\n";
	for (int i=0;i<4;i++)
	{
		wstring str;
		str.Format("MMPXY,%d,%d,%d\n",num[i],nx[i],ny[i]);
		strMapFile+=str;
	}
	for (int i=0;i<4;i++)
	{
		wstring str;
		str.Format("MMPLL,1,37.042329,56.801590\n",num[i],lon[i],lat[i]);
		strMapFile+=str;
	}
	*/

	return WriteStringToFile(strMapFileName,strMapFile);
}


BOOL GeoReference::WritePrjFile	(wstring strPrjFile)
{
	///*
	SetOGRSpatialReference();
	char* prj_txt = NULL;	
	this->m_oOGRSpatial.exportToWkt(&prj_txt);
	if (prj_txt==NULL) return FALSE;

	string strTextUTF8(prj_txt);
	wstring strText;
	utf8toWStr(strText, strTextUTF8);
	
	//delete[]prj_txt;
	return WriteStringToFile(strPrjFile,strText);
	//*/

	//return TRUE;
}

BOOL GeoReference::WriteAuxFile	(wstring strAuxFile)
{
	///*
	SetOGRSpatialReference();
	char* prj_txt = NULL;	
	this->m_oOGRSpatial.exportToWkt(&prj_txt);
	if (prj_txt==NULL) return FALSE;

	string strTextUTF8(prj_txt);
	wstring strText;
	utf8toWStr(strText, strTextUTF8);

	strText.insert(0,L"<PAMDataset>\n<SRS>");
	strText+=(L"</SRS>\n</PAMDataset>");
	//delete[]prj_txt;
	return WriteStringToFile(strAuxFile,strText);
	//*/

	//return TRUE;
}


BOOL GeoReference::WriteKmlFile	(wstring strKmlFile, wstring strTitle, wstring strJpegFile, double north, double west, double south, double east)
{
	wstring strKmlText;
	strKmlText = L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://earth.google.com/kml/2.2\">\n";
	strKmlText+=L"<GroundOverlay>\n";
	strKmlText+=L"<name>";
	strKmlText+=strTitle;
	strKmlText+=L"</name>\n";
	strKmlText+=L"<Icon>\n<href>";
	strKmlText+=strJpegFile;
	strKmlText+=L"</href>\n</Icon>\n";
	strKmlText+=L"<LatLonBox>\n";
	//strKmlText+="<north>"
	_TCHAR buf[256];
	swprintf(buf,L"<north>%lf</north>\n",north);
	wstring	strNorth=buf;
	swprintf(buf,L"<south>%lf</south>\n",south);
	wstring	strSouth = buf;
	swprintf(buf,L"<east>%lf</east>\n",east);
	wstring	strEast = buf;
	swprintf(buf,L"<west>%lf</west>\n",west);
	wstring	strWest = buf;
	strKmlText+=strNorth+strSouth+strWest+strEast;
	strKmlText+=L"</LatLonBox>\n";
	strKmlText+=L"</GroundOverlay>\n";
	strKmlText+=L"</kml>\n";

	return WriteStringToFile(strKmlFile,strKmlText);
}

BOOL GeoReference::SetOGRSpatialReference ()
{
	OGRErr oerr;
	//this->m_oOGRSpatial.

	if (this->m_Datum == this->WGS84) this->m_oOGRSpatial.SetWellKnownGeogCS("WGS84");
	else this->m_oOGRSpatial.SetWellKnownGeogCS("EPSG:4178");

	if (this->m_Projection == this->MERCATOR)
	{
		if (!OGRERR_NONE==this->m_oOGRSpatial.SetMercator(0,0,1,0,0)) return FALSE;
	}
	else if (this->m_Projection == this->UTM)
	{
		if (!OGRERR_NONE==this->m_oOGRSpatial.SetUTM(this->m_nZone)) return FALSE;
	}
	else if (this->m_Projection == this->GAUSS_KRUGER)
	{
		if (!OGRERR_NONE==this->m_oOGRSpatial.SetTM(0,this->CalcCentrMrdn(this->m_nZone),1,this->easting,0)) return FALSE;
	}
	return TRUE;
}

	