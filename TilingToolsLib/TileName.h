#pragma once

#ifndef IMAGE_TILLING_TILENAME_H
#define IMAGE_TILLING_TILENAME_H
#include "stdafx.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"
#include "VectorBorder.h"

namespace gmx
{


class ITileGrid 
{
public:
  virtual OGRSpatialReference* GetTilingSRS() =0;	
  virtual int CalcZoomByResolution (double res) =0;
  virtual double CalcResolutionByZoom (int zoom) = 0;
  virtual OGREnvelope CalcEnvelopeByTile (int zoom, int x, int y) =0;
  virtual OGREnvelope CalcEnvelopeByTileRange (int zoom, int minx, int miny, int maxx, int maxy) =0;
	virtual bool CalcTileRange (OGREnvelope envp, int z, int &min_x, int &min_y, int &max_x, int &max_y) =0;
  
  //virtual bool AdjustIfOverlapAbscissa(OGRGeometry *poGeometry) {return true;};

  virtual bool AdjustForOverlapping180Degree(OGRGeometry *poGeometry) {return true;};
  virtual bool DoesOverlap180Degree (OGRGeometry *poGeometry) {return false;};
  virtual bool AdjustIfOverlap180Degree(OGRGeometry *poGeometry) 
  {
    return DoesOverlap180Degree(poGeometry) ? AdjustForOverlapping180Degree(poGeometry) : true; 
  };

};

typedef enum { 
	NDEF_PROJ_TYPE=-1,
  WORLD_MERCATOR=0,                         
	WEB_MERCATOR=1
} MercatorProjType;

class MercatorTileGrid : public ITileGrid
{
public:
  MercatorTileGrid(MercatorProjType merc_type) 
  {
    merc_type_=merc_type;
    if (merc_type_ == WORLD_MERCATOR)
		{
			merc_srs_.SetWellKnownGeogCS("WGS84");
			merc_srs_.SetMercator(0,0,1,0,0);
		}
		else
		{
			merc_srs_.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
		}
  };

private:
  MercatorProjType merc_type_;
  OGRSpatialReference merc_srs_;

public:
  OGRSpatialReference* GetTilingSRS()
  {
    return &merc_srs_;
  }

	int CalcZoomByResolution (double res)
	{
		double a = CalcResolutionByZoom(0);
		for (int i=0;i<23;i++)
		{
			if (fabs(a-res)<0.0001) return i;
			a/=2;
		}
		return -1;
	}

	double CalcResolutionByZoom (int zoom)
	{
		return (zoom<0) ? 0 : ULY()/(1<<(zoom+7));
	}

	OGREnvelope CalcEnvelopeByTile (int zoom, int x, int y)
	{
		double size = CalcResolutionByZoom(zoom)*256;
		OGREnvelope envp;
			
		envp.MinX = ULX() + x*size;
		envp.MaxY = ULY() - y*size;
		envp.MaxX = envp.MinX + size;
		envp.MinY = envp.MaxY - size;
		
		return envp;
	}

	OGREnvelope CalcEnvelopeByTileRange (int zoom, int minx, int miny, int maxx, int maxy)
	{
		double size = CalcResolutionByZoom(zoom)*256;
		OGREnvelope envp;
			
		envp.MinX = ULX() + minx*size;
		envp.MaxY = ULY() - miny*size;
		envp.MaxX = ULX() + (maxx+1)*size;
		envp.MinY = ULY() - (maxy+1)*size;
		
		return envp;
	}

  void CalcTileByPoint (double merc_x, double merc_y, int z, int &x, int &y)
	{
		//double E = 1e-4;
		x = (int)floor((merc_x-ULX())/(256*CalcResolutionByZoom(z)));
		y = (int)floor((ULY()-merc_y)/(256*CalcResolutionByZoom(z)));
	}

	bool CalcTileRange (OGREnvelope envp, int z, int &min_x, int &min_y, int &max_x, int &max_y)
	{
		double E = 1e-6;
		CalcTileByPoint(envp.MinX+E,envp.MaxY-E,z,min_x,min_y);
		CalcTileByPoint(envp.MaxX-E,envp.MinY+E,z,max_x,max_y);
    return true;
	}

  bool DoesOverlap180Degree (OGRGeometry	*p_ogr_geom_merc) 
  {
    if (!p_ogr_geom_merc) return false;
    OGRLinearRing	**pp_ogr_rings;
	  int num_rings = 0;
	
	  if (!(pp_ogr_rings = VectorOperations::GetLinearRingsRef(p_ogr_geom_merc,num_rings))) return false;
	
    int n=0;
    for (int i=0;i<num_rings;i++)
	  {
		  for (int k=0;k<pp_ogr_rings[i]->getNumPoints()-1;k++)
		  {
         if (fabs(pp_ogr_rings[i]->getX(k) - pp_ogr_rings[i]->getX(k+1))>-ULX()/2)
         {
           delete[]pp_ogr_rings;
           return true; 
         }
		  }
	  }

    delete[]pp_ogr_rings;
    return false;
  }

		
  bool			AdjustForOverlapping180Degree (OGRGeometry	*p_ogr_geom_merc)
  {
    if (!p_ogr_geom_merc) return false;
    OGRLinearRing	**pp_ogr_rings;
	  int num_rings = 0;
	
	  if (!(pp_ogr_rings = VectorOperations::GetLinearRingsRef(p_ogr_geom_merc,num_rings))) return false;
	
    int n=0;
    for (int i=0;i<num_rings;i++)
	  {
		  for (int k=0;k<pp_ogr_rings[i]->getNumPoints();k++)
		  {
        if (pp_ogr_rings[i]->getX(k)<0)
				    pp_ogr_rings[i]->setPoint(k,-2*MercatorTileGrid::ULX() + 
                                            pp_ogr_rings[i]->getX(k),pp_ogr_rings[i]->getY(k));
		  }
	  }

    delete[]pp_ogr_rings;

    return true;
  };

static double DegToRad(double ang)
{
	return ang * (3.14159265358979/180.0);
}

static double RadToDeg(double rad)
{ 
	return (rad/3.14159265358979) * 180.0;
}


	//	Converts from longitude to x coordinate 
static	double MercX(double lon, MercatorProjType merc_type)
{
	return 6378137.0 * DegToRad(lon);
}


	//	Converts from x coordinate to longitude 
static 	double MecrToLong(double MercX, MercatorProjType merc_type)
{
	return RadToDeg(MercX / 6378137.0);
}

	//	Converts from latitude to y coordinate 
static 	double MercY(double lat, MercatorProjType merc_type)
{
	if (merc_type == WORLD_MERCATOR)
	{
		if (lat > 89.5)		lat = 89.5;
		if (lat < -89.5)	lat = -89.5;
		double r_major	= 6378137.000;
		double r_minor	= 6356752.3142;
		double PI		= 3.14159265358979;
		
		double temp = r_minor / r_major;
		double es = 1.0 - (temp * temp);
		double eccent = sqrt(es);
		double phi = DegToRad(lat);
		double sinphi = sin(phi);
		double con = eccent * sinphi;
		double com = .5 * eccent;
		con = pow(((1.0-con)/(1.0+con)), com);
		double ts = tan(.5 * ((PI*0.5) - phi))/con;
		return 0 - r_major * log(ts);
	}
	else
	{
		double rad = DegToRad(lat);
		return  0.5*6378137*log((1.0 + sin(rad))/(1.0 - sin(rad)));
	}
}

	//	Converts from y coordinate to latitude 
static	double MercToLat (double MercY, MercatorProjType merc_type)
{
	double r_major	= 6378137.000;
	double r_minor	= 6356752.3142;

	if (merc_type == WORLD_MERCATOR)
	{
		double temp = r_minor / r_major;
		double es = 1.0 - (temp * temp);
		double eccent = sqrt(es);
		double ts = exp(-MercY/r_major);
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

		return RadToDeg(Phi);
	}
	else
	{
		return RadToDeg (1.5707963267948966 - (2.0 * atan(exp((-1.0 * MercY) / 6378137.0))));
	}
}


	
private:
  double	ULX()//get_ulx(
	{
		return -20037508.3427812843076588408880691;
	}

	double	ULY()
	{
		return 20037508.3427812843076588408880691;
	}

};


typedef enum { 
  NDEF_TILE_TYPE=-1,
	JPEG_TILE=0,
	PNG_TILE=1,
	TIFF_TILE=4,
  JP2_TILE=16,
  PSEUDO_PNG_TILE=32
} TileType;


class TileName
{
public:
  static string TileInfoByTileType(TileType tile_type)
	{
		switch (tile_type)
		{
			case JPEG_TILE:
				return "JPEG";
			case PNG_TILE:
				return "PNG";
      case PSEUDO_PNG_TILE:
				return "PSEUDO_PNG";
			case TIFF_TILE:
				return "TIFF";
      case JP2_TILE:
        return "JPEG2000";
		}
		return "";
	}

	static string ExtensionByTileType(TileType tile_type)
	{
		switch (tile_type)
		{
			case JPEG_TILE:
				return "jpg";
			case PNG_TILE:
				return "png";
			case TIFF_TILE:
				return "tif";
      case JP2_TILE:
        return "jp2";
		}
		return "";
	}

  static bool TileTypeByExtension (string tile_extension, TileType &tile_type)
	{
    tile_extension = MakeLower(tile_extension);
		if ((tile_extension == "jpg") || (tile_extension == "jpeg") || (tile_extension == ".jpg"))
    {
      tile_type = JPEG_TILE;
      return TRUE;
    }
    else if ((tile_extension == "png") || (tile_extension == ".png"))
    {
      tile_type = gmx::PNG_TILE;
      return TRUE;
    }
    else if ((tile_extension == "jp2") || (tile_extension == ".jp2")||(tile_extension == "jpeg2000"))
    {
      tile_type = gmx::JP2_TILE;
      return TRUE;
    }
    else if ((tile_extension == "tif") || (tile_extension == ".tif")||(tile_extension == "tiff"))
    {
      tile_type = gmx::TIFF_TILE;
      return TRUE;
 	  }
    else return FALSE;
 }
  
  

////////////////////////////////////////////////////////////////////////////////////////////
///Эти методы нужно реализовать в производном классе (см. KosmosnimkiTileName)
////////////////////////////////////////////////////////////////////////////////////////////
	virtual	string		GetTileName				(int zoom, int x, int y) = 0;
	virtual	bool	ExtractXYZFromTileName	(string tile_name, int &z, int &x, int &y) = 0;
	virtual	bool	CreateFolder			(int zoom, int x, int y) = 0;
///////////////////////////////////////////////////////////////////////////////////////////

	string GetFullTileName (int zoom, int nX, int nY)
	{
		return base_folder_ + "/" + GetTileName(zoom,nX,nY);
	}

	string GetBaseFolder ()
	{
		return base_folder_;
	}
	
	
	

public:
	string		base_folder_;
	TileType	tile_type_;
	//string	ExtensionByTileType;

protected:
	char buf[1000];
};


class StandardTileName : public TileName
{
public:
	StandardTileName (string base_folder, string str_template);
	static bool	ValidateTemplate	(string str_template);
	string	GetTileName (int zoom, int nX, int nY);

	bool ExtractXYZFromTileName (string tile_name, int &z, int &x, int &y);
	bool CreateFolder (int zoom, int nX, int nY);
protected:
	string	str_template_;
	regex	rx_template_;
	int		zxy_pos_[3];
};


class KosmosnimkiTileName : public TileName
{
public:
	KosmosnimkiTileName (string tiles_folder, TileType tile_type = JPEG_TILE);
	string	GetTileName (int zoom, int x, int y);
	bool ExtractXYZFromTileName (string tile_name, int &z, int &x, int &y);
	bool CreateFolder (int zoom, int x, int y);
};


class ESRITileName : public TileName
{
public:
	ESRITileName (string base_folder, string str_template);
	static bool	ValidateTemplate	(string str_template);
	string	GetTileName (int zoom, int nX, int nY);

	bool ExtractXYZFromTileName (string tile_name, int &z, int &x, int &y);
	bool CreateFolder (int zoom, int nX, int nY);
protected:
	string	str_template_;
	regex	rx_template_;
};


}
#endif