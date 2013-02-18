#pragma once

#ifndef IMAGE_TILLING_TILENAME_H
#define IMAGE_TILLING_TILENAME_H
#include "stdafx.h"
#include "RasterBuffer.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"

namespace GMT
{


typedef enum { 
	WORLD_MERCATOR=0,                         
	WEB_MERCATOR=1
} MercatorProjType;




class MercatorTileGrid
{

public:
	static const int TILE_SIZE = 256;

	static void setMercatorSpatialReference (MercatorProjType mercType, OGRSpatialReference	*poSR)
	{
		if (poSR == NULL) return;		

		if (mercType == WORLD_MERCATOR)
		{
			poSR->SetWellKnownGeogCS("WGS84");
			poSR->SetMercator(0,0,1,0,0);
		}
		else
		{
			poSR->importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
		}
		return;
	}

	static double	getULX()
	{
		return -20037508.342789243076588408880691;
	}

	static double	getULY()
	{
		return 20037508.342789243076588408880691;
	}
	
	static int calcZoomByResolution (double dRes)
	{
		double a = calcResolutionByZoom(0);
		for (int i=0;i<22;i++)
		{
			if (fabs(a-dRes)<0.0001) return i;
			a/=2;
		}
		return -1;
	}

	static double calcResolutionByZoom (int nZoom)
	{
		return (nZoom<0) ? 0 : getULY()/(1<<(nZoom+7));
	}

	static int calcZoomByEnvelope (OGREnvelope envelope)
	{
		return calcZoomByResolution ((envelope.MaxX-envelope.MinX)/TILE_SIZE);
	}

	static OGREnvelope calcEnvelopeByTile (int nZoom, int nX, int nY)
	{
		double size = calcResolutionByZoom(nZoom)*TILE_SIZE;
		OGREnvelope oEnvelope;
			
		oEnvelope.MinX = getULX() + nX*size;
		oEnvelope.MaxY = getULY() - nY*size;
		oEnvelope.MaxX = oEnvelope.MinX + size;
		oEnvelope.MinY = oEnvelope.MaxY - size;
		
		return oEnvelope;
	}

	static __int64 TileID(int z, int x, int y)
	{
		__int64 tileID = 0;
		for (int i=0;i<z;i++)
			tileID+=(((__int64)1)<<(2*i));
		return (tileID + ((__int64)y)*(1<<z) + x);
	}

	static void TileXYZ(__int64 tileID, int &z, int &x, int &y)
	{
		__int64 n = 0;
		for (int i=0;i<z;i++)
		{
			if (n+((__int64)1)<<(2*i)>tileID)
			{
				z = i;
				break;
			}
			else n+=((__int64)1)<<(2*i)>tileID;
		}
		y = (tileID-n)/(((__int64)1)<<(2*z));
		y = (tileID-n)%(((__int64)1)<<(2*z));
	}


	static OGREnvelope calcEnvelopeByTileRange (int nZoom, int minx, int miny, int maxx, int maxy)
	{
		double size = calcResolutionByZoom(nZoom)*TILE_SIZE;
		OGREnvelope oEnvelope;
			
		oEnvelope.MinX = getULX() + minx*size;
		oEnvelope.MaxY = getULY() - miny*size;
		oEnvelope.MaxX = getULX() + (maxx+1)*size;
		oEnvelope.MinY = getULY() - (maxy+1)*size;
		
		return oEnvelope;
	}


	static OGREnvelope calcPixelEnvelope(OGREnvelope oImageEnvelope, int z)
	{
		OGREnvelope oPixelEnvelope;
		const double E=1e-4;
	
		oPixelEnvelope.MinX = floor(((oImageEnvelope.MinX-getULX()-E)/calcResolutionByZoom(z))+0.5);
		oPixelEnvelope.MinY = floor(((getULY()-oImageEnvelope.MaxY-E)/calcResolutionByZoom(z))+0.5);
		oPixelEnvelope.MaxX = oPixelEnvelope.MinX + floor((oImageEnvelope.MaxX - oImageEnvelope.MinX)/calcResolutionByZoom(z)+0.5);
		oPixelEnvelope.MaxY = oPixelEnvelope.MinY + floor((oImageEnvelope.MaxY - oImageEnvelope.MinY)/calcResolutionByZoom(z)+0.5);
		
		return oPixelEnvelope;
	}

	static void calcTileByPoint (double x, double y, int z, int &nX, int &nY)
	{
		//double E = 1e-4;
		nX = (int)floor((x-getULX())/(TILE_SIZE*calcResolutionByZoom(z)));
		nY = (int)floor((getULY()-y)/(TILE_SIZE*calcResolutionByZoom(z)));
	}

	static void calcTileRange (OGREnvelope oEnvelope, int z, int &minX, int &minY, int &maxX, int &maxY)
	{
		double E = 1e-6;
		calcTileByPoint(oEnvelope.MinX+E,oEnvelope.MaxY-E,z,minX,minY);
		calcTileByPoint(oEnvelope.MaxX-E,oEnvelope.MinY+E,z,maxX,maxY);
	}
		

	//	Converts from longitude to x coordinate 
	static double mercX(double lon, MercatorProjType mercType)
	{
		return 6378137.0 * degToRad(lon);
	}


	//	Converts from x coordinate to longitude 
	static double mercToLong(double mercX, MercatorProjType mercType)
	{
		return radToDeg(mercX / 6378137.0);
	}

	//	Converts from latitude to y coordinate 
	static  double mercY(double lat, MercatorProjType mercType)
	{
		if (mercType == WORLD_MERCATOR)
		{
			if (lat > 89.5)		lat = 89.5;
			if (lat < -89.5)	lat = -89.5;
			double r_major	= 6378137.000;
			double r_minor	= 6356752.3142;
			double PI		= 3.14159265358979;
		
			double temp = r_minor / r_major;
			double es = 1.0 - (temp * temp);
			double eccent = sqrt(es);
			double phi = degToRad(lat);
			double sinphi = sin(phi);
			double con = eccent * sinphi;
			double com = .5 * eccent;
			con = pow(((1.0-con)/(1.0+con)), com);
			double ts = tan(.5 * ((PI*0.5) - phi))/con;
			return 0 - r_major * log(ts);
		}
		else
		{
			double rad = degToRad(lat);
			return  0.5*6378137*log((1.0 + sin(rad))/(1.0 - sin(rad)));
		}
	}

	//	Converts from y coordinate to latitude 
	static  double mercToLat (double mercY, MercatorProjType mercType)
	{
		double r_major	= 6378137.000;
		double r_minor	= 6356752.3142;

		if (mercType == WORLD_MERCATOR)
		{
			double temp = r_minor / r_major;
			double es = 1.0 - (temp * temp);
			double eccent = sqrt(es);
			double ts = exp(-mercY/r_major);
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

			return radToDeg(Phi);
		}
		else
		{
			return radToDeg (1.5707963267948966 - (2.0 * atan(exp((-1.0 * mercY) / 6378137.0))));
		}
	}

	static double degToRad(double ang)
	{
		return ang * (3.14159265358979/180.0);
	}

	static double radToDeg(double rad)
	{ 
		return (rad/3.14159265358979) * 180.0;
	}
};
//const double MercatorTileGrid::E = 1e-4;

typedef enum { 
	JPEG_TILE		=0,
	PNG_TILE		=1,
	TIFF_TILE		=4
} TileType;


class TileName
{
public:

	static string tileExtension(TileType tileType)
	{
		switch (tileType)
		{
			case JPEG_TILE:
				return "jpg";
			case PNG_TILE:
				return "png";
			case TIFF_TILE:
				return "tif";
		}
		return "";
	}

////////////////////////////////////////////////////////////////////////////////////////////
///Эти методы нужно реализовать в производном классе (см. KosmosnimkiTileName)
////////////////////////////////////////////////////////////////////////////////////////////
	virtual	string		getTileName				(int nZoom, int nX, int nY) = 0;
	virtual	BOOL		extractXYZFromTileName	(string strTileName, int &z, int &x, int &y) = 0;
	virtual	BOOL		createFolder			(int nZoom, int nX, int nY) = 0;
///////////////////////////////////////////////////////////////////////////////////////////

	string getFullTileName (int nZoom, int nX, int nY)
	{
		return baseFolder + "/" + getTileName(nZoom,nX,nY);
	}

	string getBaseFolder ()
	{
		return baseFolder;
	}
	
	
	

public:
	string		baseFolder;
	TileType	tileType;
	//string	tileExtension;

protected:
	char buf[1000];
};


class StandardTileName : public TileName
{
public:
	StandardTileName (string baseFolder, string strTemplate);
	static BOOL	validateTemplate	(string strTemplate);
	string	getTileName (int nZoom, int nX, int nY);

	BOOL extractXYZFromTileName (string strTileName, int &z, int &x, int &y);
	BOOL createFolder (int nZoom, int nX, int nY);
protected:
	string	strTemplate;
	regex	rxTemplate;
	int		zxyPos[3];
};


class KosmosnimkiTileName : public TileName
{
public:
	KosmosnimkiTileName (string strTilesFolder, TileType tileType = JPEG_TILE);
	string	getTileName (int nZoom, int nX, int nY);
	BOOL extractXYZFromTileName (string strTileName, int &z, int &x, int &y);
	BOOL createFolder (int nZoom, int nX, int nY);
};


}
#endif
