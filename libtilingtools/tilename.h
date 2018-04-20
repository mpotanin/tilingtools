#pragma once
#include "stdafx.h"
#include "stringfuncs.h"
#include "filesystemfuncs.h"
#include "vectorborder.h"
#include "rasterbuffer.h"

namespace gmx
{

class RasterFile;

class ITileMatrixSet 
{
public:
  virtual OGRSpatialReference* GetTilingSRSRef() =0;	
  virtual double CalcPixelSizeByZoom (int zoom) = 0;
  virtual OGREnvelope CalcEnvelopeByTile (int zoom, int x, int y) =0;
  virtual OGREnvelope CalcEnvelopeByTileRange (int zoom, int minx, int miny, int maxx, int maxy) =0;
	virtual bool CalcTileRange (OGREnvelope envp, int z, int &min_x, int &min_y, int &max_x, int &max_y) =0;
  virtual bool GetRasterEnvelope(RasterFile* p_rf, OGREnvelope &envp) =0;
    

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

class MercatorTileMatrixSet : public ITileMatrixSet
{
public:
  MercatorTileMatrixSet(MercatorProjType merc_type = WEB_MERCATOR);

private:
  MercatorProjType merc_type_;
  OGRSpatialReference merc_srs_;

public:
  MercatorProjType merc_type ();
  
  OGRSpatialReference* GetTilingSRSRef();
	double CalcPixelSizeByZoom (int zoom);

	OGREnvelope CalcEnvelopeByTile (int zoom, int x, int y);

	OGREnvelope CalcEnvelopeByTileRange (int zoom, int minx, int miny, int maxx, int maxy);

  void CalcTileByPoint (double merc_x, double merc_y, int z, int &x, int &y);
	bool CalcTileRange (OGREnvelope envp, int z, int &min_x, int &min_y, int &max_x, int &max_y);
  bool DoesOverlap180Degree (OGRGeometry	*p_ogr_geom_merc);

  bool GetRasterEnvelope(RasterFile* p_rf, OGREnvelope &envp);

		
  bool			AdjustForOverlapping180Degree (OGRGeometry	*p_ogr_geom_merc);

  
  static double DegToRad(double ang);
  static double RadToDeg(double rad);

	  //	Converts from longitude to x coordinate 
  static	double MercX(double lon, MercatorProjType merc_type);


	  //	Converts from x coordinate to longitude 
  static 	double MecrToLong(double MercX, MercatorProjType merc_type);
	  //	Converts from latitude to y coordinate 
  static 	double MercY(double lat, MercatorProjType merc_type);
	  //	Converts from y coordinate to latitude 
  static	double MercToLat (double MercY, MercatorProjType merc_type);

	
protected:
  double	ULX();
	double	ULY();

};





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
      case PSEUDO_PNG_TILE:
        return "png";
		}
		return "";
	}

  static bool TileTypeByExtension (string tile_extension, TileType &tile_type)
	{
    tile_extension = GMXString::MakeLower(tile_extension);
		if ((tile_extension == "jpg") || (tile_extension == "jpeg") || (tile_extension == ".jpg"))
      tile_type = JPEG_TILE;
    else if ((tile_extension == "png") || (tile_extension == ".png"))
      tile_type = gmx::PNG_TILE;
    else if ((tile_extension == "jp2") || (tile_extension == ".jp2")||(tile_extension == "jpeg2000"))
      tile_type = gmx::JP2_TILE;
    else if ((tile_extension == "tif") || (tile_extension == ".tif")||(tile_extension == "tiff"))
      tile_type = gmx::TIFF_TILE;
    else return false;

    return true;
 }
  
  

////////////////////////////////////////////////////////////////////////////////////////////
///Эти методы нужно реализовать в производном классе (см. KosmosnimkiTileName)
////////////////////////////////////////////////////////////////////////////////////////////
	virtual	string		GetTileName				(int zoom, int x, int y) = 0;
	virtual	bool	ExtractXYZ	(string tile_name, int &z, int &x, int &y) = 0;
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

	bool ExtractXYZ (string tile_name, int &z, int &x, int &y);
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
	bool ExtractXYZ (string tile_name, int &z, int &x, int &y);
	bool CreateFolder (int zoom, int x, int y);
};


class ESRITileName : public TileName
{
public:
	ESRITileName (string base_folder, string str_template);
	static bool	ValidateTemplate	(string str_template);
	string	GetTileName (int zoom, int nX, int nY);

	bool ExtractXYZ (string tile_name, int &z, int &x, int &y);
	bool CreateFolder (int zoom, int nX, int nY);
protected:
	string	str_template_;
	regex	rx_template_;
};


}
