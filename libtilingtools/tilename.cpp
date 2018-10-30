#include "tilename.h"
#include "rasterfile.h"

namespace gmx
{


StandardTileName::StandardTileName (string base_folder, string str_template)
{
	if (!ValidateTemplate(str_template)) return;
	if (!GMXFileSys::FileExists(base_folder)) return;

  if (!TileName::TileTypeByExtension(GMXFileSys::GetExtension(str_template),tile_type_))
  {
    cout<<"ERROR: can't parse tile type from input template: "<<str_template<<endl;
    return;
  }

	base_folder_	= base_folder;
	zxy_pos_[0] = (zxy_pos_[1] = (zxy_pos_[2] = 0));

	if (str_template[0] == L'/' || str_template[0] == L'\\') 	str_template = str_template.substr(1,str_template.length()-1);
	GMXString::ReplaceAll(str_template,"\\","/");
	str_template_ = str_template;
		
	//ReplaceAll(strTemplate,"\\","\\\\");
	int n = 0;
	int num = 2;
	//int k;
	while (str_template.find(L'{',n)!=std::string::npos)
	{
		string str = str_template.substr(str_template.find(L'{',n),str_template.find(L'}',n)-str_template.find(L'{',n)+1);
		if (str == "{z}")
			zxy_pos_[0] = (zxy_pos_[0] == 0) ? num : zxy_pos_[0];
		else if (str == "{x}")
			zxy_pos_[1] = (zxy_pos_[1] == 0) ? num : zxy_pos_[1];
		else if (str == "{y}")
			zxy_pos_[2] = (zxy_pos_[2] == 0) ? num : zxy_pos_[2];
		num++;
		n = str_template.find(L'}',n) + 1;
	}

	GMXString::ReplaceAll(str_template,"{z}","(\\d+)");
	GMXString::ReplaceAll(str_template,"{x}","(\\d+)");
	GMXString::ReplaceAll(str_template,"{y}","(\\d+)");
	rx_template_ = ("(.*[\\/])" + str_template) + "(.*)";
  //rx_template_ = str_template;
}

bool	StandardTileName::ValidateTemplate	(string str_template)
{

	if (str_template.find("{z}",0)==string::npos)
	{
		//cout<<"Error: bad tile name template: missing {z}"<<endl;
		return FALSE;
	}
	if (str_template.find("{x}",0)==string::npos)
	{
		//cout<<"Error: bad tile name template: missing {x}"<<endl;
		return FALSE;
	}
	if (str_template.find("{y}",0)==string::npos) 
	{
		//cout<<"Error: bad tile name template: missing {y}"<<endl;
		return FALSE;
	}

	if (str_template.find(".",0)==string::npos) 
	{
		//cout<<"Error: bad tile name template: missing extension"<<endl;
		return FALSE;
	}
		
  TileType tt;
  if (!TileName::TileTypeByExtension(GMXFileSys::GetExtension(str_template),tt))
  {
		cout<<"ERROR: not valid tile type in template: "<<str_template<<endl;
		return FALSE;
	}
	return TRUE;
}

string	StandardTileName::GetTileName (int zoom, int nX, int nY)
{
	string tile_name = str_template_;
	GMXString::ReplaceAll(tile_name,"{z}",GMXString::ConvertIntToString(zoom));
	GMXString::ReplaceAll(tile_name,"{x}",GMXString::ConvertIntToString(nX));
	GMXString::ReplaceAll(tile_name,"{y}",GMXString::ConvertIntToString(nY));
	return tile_name;
}

bool StandardTileName::ExtractXYZ (string tile_name, int &z, int &x, int &y)
{
	if (!regex_match(tile_name,rx_template_)) return FALSE;
	match_results<string::const_iterator> mr;
	regex_search(tile_name, mr, rx_template_);
  for (int i=0;i<mr.size();i++)
  {
    string str = mr[i].str();
    i=i;
  }

	if ((mr.size()<=zxy_pos_[0])||(mr.size()<=zxy_pos_[1])||(mr.size()<=zxy_pos_[2])) return FALSE;
	z = atoi(mr[zxy_pos_[0]].str().c_str());
	x = atoi(mr[zxy_pos_[1]].str().c_str());
	y = atoi(mr[zxy_pos_[2]].str().c_str());
		
	return TRUE;
}


bool StandardTileName::CreateFolder (int zoom, int x, int y)
{
	string tile_name = GetTileName(zoom,x,y);
	int n = 0;
	while (tile_name.find("/",n)!=std::string::npos)
	{
		if (!GMXFileSys::FileExists(GMXFileSys::GetAbsolutePath(base_folder_,tile_name.substr(0,tile_name.find("/",n)))))
			if (!GMXFileSys::CreateDir(GMXFileSys::GetAbsolutePath(base_folder_,tile_name.substr(0,tile_name.find("/",n))).c_str())) return FALSE;	
		n = (tile_name.find("/",n)) + 1;
	}
	return TRUE;
}


bool ESRITileName::CreateFolder (int zoom, int x, int y)
{
	string tile_name = GetTileName(zoom,x,y);
	int n = 0;
	while (tile_name.find("/",n)!=std::string::npos)
	{
		if (!GMXFileSys::FileExists(GMXFileSys::GetAbsolutePath(base_folder_,tile_name.substr(0,tile_name.find("/",n)))))
			if (!GMXFileSys::CreateDir(GMXFileSys::GetAbsolutePath(base_folder_,tile_name.substr(0,tile_name.find("/",n))).c_str())) return FALSE;	
		n = (tile_name.find("/",n)) + 1;
	}
	return TRUE;
}


bool	ESRITileName::ValidateTemplate	(string str_template)
{
  str_template = GMXString::MakeLower(str_template);
	if (str_template.find("{l}",0)==string::npos)
	{
		//cout<<"Error: bad tile name template: missing {L}"<<endl;
		return FALSE;
	}
	if (str_template.find("{c}",0)==string::npos)
	{
		//cout<<"Error: bad tile name template: missing {C}"<<endl;
		return FALSE;
	}
	if (str_template.find("{r}",0)==string::npos) 
	{
		//cout<<"Error: bad tile name template: missing {R}"<<endl;
		return FALSE;
	}

	if (str_template.find(".",0)==string::npos) 
	{
		//cout<<"Error: bad tile name template: missing extension"<<endl;
		return FALSE;
	}
		
	TileType tt;
  if (!TileName::TileTypeByExtension(GMXFileSys::GetExtension(str_template),tt))
  {
		cout<<"ERROR: not valid tile type in template: "<<str_template<<endl;
		return FALSE;
	}
	return TRUE;
}


ESRITileName::ESRITileName (string base_folder, string str_template)
{
  GMXString::ReplaceAll(str_template,"\\","/");
  GMXString::ReplaceAll(str_template,"{l}","{L}");
  GMXString::ReplaceAll(str_template,"{r}","{R}");
  GMXString::ReplaceAll(str_template,"{c}","{C}");

	if (!ValidateTemplate(str_template)) return;
	if (!GMXFileSys::FileExists(base_folder)) return;

	if (!TileName::TileTypeByExtension(GMXFileSys::GetExtension(str_template),tile_type_))
  {
    cout<<"ERROR: can't parse tile type from input template: "<<str_template<<endl;
    return;
  }
	
  base_folder_	= base_folder;
	
	if (str_template[0] == L'/') 	str_template = str_template.substr(1,str_template.length()-1);
  str_template_ = str_template;
		
  GMXString::ReplaceAll(str_template,"{L}","(L\\d{2})");
  GMXString::ReplaceAll(str_template,"{C}","(C[A-Fa-f0-9]{8,8})");
  GMXString::ReplaceAll(str_template,"{R}","(R[A-Fa-f0-9]{8,8})");
	rx_template_ = ("(.*)" + str_template) + "(.*)";
}


string	ESRITileName::GetTileName (int zoom, int nX, int nY)
{
	string tile_name = str_template_;
	GMXString::ReplaceAll(tile_name,"{L}","L"+GMXString::ConvertIntToString(zoom,FALSE,2));
	GMXString::ReplaceAll(tile_name,"{C}","C"+GMXString::ConvertIntToString(nX,TRUE,8));
	GMXString::ReplaceAll(tile_name,"{R}","R"+GMXString::ConvertIntToString(nY,TRUE,8));

  return tile_name;
}


bool ESRITileName::ExtractXYZ (string tile_name, int &z, int &x, int &y)
{
	if (!regex_match(tile_name,rx_template_)) return FALSE;
	match_results<string::const_iterator> mr;
	regex_search(tile_name, mr, rx_template_);
  
  for (int i=0;i<mr.size();i++)
  {
    if (mr[i].str()[0] == 'L') z = strtol(mr[i].str().substr(1).c_str(), 0, 10); 
    if (mr[i].str()[0] == 'R') y = strtol(mr[i].str().substr(1).c_str(), 0, 16);
    if (mr[i].str()[0] == 'C') x = strtol(mr[i].str().substr(1).c_str(), 0, 16);
  }

 	return TRUE;
}


KosmosnimkiTileName::KosmosnimkiTileName (string tiles_folder, TileType tile_type)
{
	base_folder_	= tiles_folder;
	tile_type_		= tile_type;
}


	
string	KosmosnimkiTileName::GetTileName (int zoom, int x, int y)
{
	if (zoom>0)
	{
		x = x-(1<<(zoom-1));
		y = (1<<(zoom-1))-y-1;
	}
	sprintf(buf,"%d\\%d\\%d_%d_%d.%s",zoom,x,zoom,x,y,this->ExtensionByTileType(tile_type_).c_str());
	return buf;
}

bool KosmosnimkiTileName::ExtractXYZ (string tile_name, int &z, int &x, int &y)
{
	tile_name = GMXFileSys::RemovePath(tile_name);
	tile_name = GMXFileSys::RemoveExtension(tile_name);
	int k;

	regex pattern("[0-9]{1,2}_-{0,1}[0-9]{1,7}_-{0,1}[0-9]{1,7}");
	//wregex pattern("(\d+)_-?(\d+)_-{0,1}[0-9]{1,7}");

	if (!regex_match(tile_name,pattern)) return FALSE;

	z = atoi(tile_name.substr(0,tile_name.find('_')).c_str());
	tile_name = tile_name.substr(tile_name.find('_')+1);
		
	x = atoi(tile_name.substr(0,tile_name.find('_')).c_str());
	y = atoi(tile_name.substr(tile_name.find('_')+1).c_str());

	if (z>0)
	{
		x+=(1<<(z-1));
		y=(1<<(z-1))-y-1;
	}

	return TRUE;
}


bool KosmosnimkiTileName::CreateFolder (int zoom, int x, int y)
{
	if (zoom>0)
	{
		x = x-(1<<(zoom-1));
		y = (1<<(zoom-1))-y-1;
	}

	sprintf(buf,"%d",zoom);
	string str = GMXFileSys::GetAbsolutePath(base_folder_, buf);
	if (!GMXFileSys::FileExists(str))
	{
		if (!GMXFileSys::CreateDir(str.c_str())) return FALSE;	
	}

	sprintf(buf,"%d",x);
	str = GMXFileSys::GetAbsolutePath(str,buf);
	if (!GMXFileSys::FileExists(str))
	{
		if (!GMXFileSys::CreateDir(str.c_str())) return FALSE;	
	}
		
	return TRUE;
}




MercatorTileMatrixSet::MercatorTileMatrixSet(MercatorProjType merc_type)
{
  merc_type_ = merc_type;
  if (merc_type_ == WORLD_MERCATOR)
  {
    merc_srs_.SetWellKnownGeogCS("WGS84");
    merc_srs_.SetMercator(0, 0, 1, 0, 0);
  }
  else
  {
    merc_srs_.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
  }
};


MercatorProjType MercatorTileMatrixSet::merc_type()
{
  return merc_type_;
};

OGRSpatialReference* MercatorTileMatrixSet::GetTilingSRSRef()
{
  return &merc_srs_;
}

double MercatorTileMatrixSet::CalcPixelSizeByZoom(int zoom)
{
  return (zoom<0) ? 0 : ULY() / (1 << (zoom + 7));
}

OGREnvelope MercatorTileMatrixSet::CalcEnvelopeByTile(int zoom, int x, int y)
{
  double size = CalcPixelSizeByZoom(zoom) * 256;
  OGREnvelope envp;

  envp.MinX = ULX() + x*size;
  envp.MaxY = ULY() - y*size;
  envp.MaxX = envp.MinX + size;
  envp.MinY = envp.MaxY - size;

  return envp;
}

OGREnvelope MercatorTileMatrixSet::CalcEnvelopeByTileRange(int zoom, int minx, int miny, int maxx, int maxy)
{
  double size = CalcPixelSizeByZoom(zoom) * 256;
  OGREnvelope envp;

  envp.MinX = ULX() + minx*size;
  envp.MaxY = ULY() - miny*size;
  envp.MaxX = ULX() + (maxx + 1)*size;
  envp.MinY = ULY() - (maxy + 1)*size;

  return envp;
}

void MercatorTileMatrixSet::CalcTileByPoint(double merc_x, double merc_y, int z, int &x, int &y)
{
  //double E = 1e-4;
  x = (int)floor((merc_x - ULX()) / (256 * CalcPixelSizeByZoom(z)));
  y = (int)floor((ULY() - merc_y) / (256 * CalcPixelSizeByZoom(z)));
}

bool MercatorTileMatrixSet::CalcTileRange(OGREnvelope envp, int z, int &min_x, int &min_y, int &max_x, int &max_y)
{
  double E = 1e-6;
  CalcTileByPoint(envp.MinX + E, envp.MaxY - E, z, min_x, min_y);
  CalcTileByPoint(envp.MaxX - E, envp.MinY + E, z, max_x, max_y);
  return true;
}

bool MercatorTileMatrixSet::DoesOverlap180Degree(OGRGeometry	*p_ogr_geom_merc)
{
  if (!p_ogr_geom_merc) return false;
  OGRLinearRing	**pp_ogr_rings;
  int num_rings = 0;

  if (!(pp_ogr_rings = VectorOperations::GetLinearRingsRef(p_ogr_geom_merc, num_rings))) return false;

  int n = 0;
  for (int i = 0; i<num_rings; i++)
  {
    for (int k = 0; k<pp_ogr_rings[i]->getNumPoints() - 1; k++)
    {
      if (fabs(pp_ogr_rings[i]->getX(k) - pp_ogr_rings[i]->getX(k + 1))>-ULX())
      {
        delete[]pp_ogr_rings;
        return true;
      }
    }
  }

  delete[]pp_ogr_rings;
  return false;
}

bool MercatorTileMatrixSet::GetRasterEnvelope(RasterFile* p_rf, OGREnvelope &envp)
{
  if (!p_rf) return false;
  GDALDataset* p_rf_ds = p_rf->get_gdal_ds_ref();
  if (!p_rf_ds) return false;

  char* pszMercWKT = NULL;

  if (CE_None != GetTilingSRSRef()->exportToWkt(&pszMercWKT)) return false;

  char* pszSrcWKT = 0;
  OGRSpatialReference raster_srs;
  if (!p_rf->GetSRS(raster_srs, this))
  {
    OGRFree(pszMercWKT);
    return false;
  }
  if (CE_None != raster_srs.exportToWkt(&pszSrcWKT))
  {
    OGRFree(pszMercWKT);
    return false;
  }

  void* hTransformArg = 0;
  if (!(hTransformArg = GDALCreateGenImgProjTransformer(p_rf_ds, pszSrcWKT, 0, pszMercWKT, 1, 0.125, 0)))
  {
    OGRFree(pszMercWKT);
    OGRFree(pszSrcWKT);
    return false;
  }

  OGRFree(pszMercWKT);
  //OGRFree(pszSrcWKT);


  double adfDstGeoTransform[6];
  int nPixels = 0, nLines = 0;
  if (CE_None != GDALSuggestedWarpOutput(p_rf_ds, GDALGenImgProjTransform, hTransformArg, adfDstGeoTransform, &nPixels, &nLines))
  {
    GDALDestroyGenImgProjTransformer(hTransformArg);
    return false;
  }
  
  GDALDestroyGenImgProjTransformer(hTransformArg);

  OGRSpatialReference merc_srs_shifted;
  if (merc_type_ == WORLD_MERCATOR)
  {
    merc_srs_shifted.SetWellKnownGeogCS("WGS84");
    merc_srs_shifted.SetMercator(0, 90, 1, 0, 0);
  }
  else
  {
    merc_srs_shifted.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=90.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
  }
  if (CE_None != merc_srs_shifted.exportToWkt(&pszMercWKT)) return false;
  
  if (!(hTransformArg = GDALCreateGenImgProjTransformer(p_rf_ds, pszSrcWKT, 0, pszMercWKT, 1, 0.125, 0)))
    return false;


  OGRFree(pszMercWKT);
  OGRFree(pszSrcWKT);

  double adfDstGeoTransform2[6];
  int nPixels2 = 0, nLines2 = 0;
  if (CE_None != GDALSuggestedWarpOutput(p_rf_ds, GDALGenImgProjTransform, hTransformArg, adfDstGeoTransform2, &nPixels2, &nLines2))
  {
    GDALDestroyGenImgProjTransformer(hTransformArg);
    return false;
  }

  GDALDestroyGenImgProjTransformer(hTransformArg);


  if ((((double)nPixels) / ((double)nLines)) > 2 * (((double)nPixels2) / ((double)nLines2)))
  {
    envp.MinX = adfDstGeoTransform2[0] + (-0.5*ULX());
    envp.MaxY = adfDstGeoTransform2[3];
    envp.MaxX = adfDstGeoTransform2[0] + nPixels2*adfDstGeoTransform2[1] + (-0.5*ULX());
    envp.MinY = adfDstGeoTransform2[3] + nLines2*adfDstGeoTransform2[5];
  }
  else
  {
    envp.MinX = adfDstGeoTransform[0];
    envp.MaxY = adfDstGeoTransform[3];
    envp.MaxX = adfDstGeoTransform[0] + nPixels*adfDstGeoTransform[1];
    envp.MinY = adfDstGeoTransform[3] + nLines*adfDstGeoTransform[5];
  }

  return true;
}


bool MercatorTileMatrixSet::AdjustForOverlapping180Degree(OGRGeometry	*p_ogr_geom_merc)
{
  if (!p_ogr_geom_merc) return false;
  OGRLinearRing	**pp_ogr_rings;
  int num_rings = 0;

  if (!(pp_ogr_rings = VectorOperations::GetLinearRingsRef(p_ogr_geom_merc, num_rings))) return false;

  int n = 0;
  for (int i = 0; i<num_rings; i++)
  {
    for (int k = 0; k<pp_ogr_rings[i]->getNumPoints(); k++)
    {
      if (pp_ogr_rings[i]->getX(k)<0)
        pp_ogr_rings[i]->setPoint(k, -2 * MercatorTileMatrixSet::ULX() +
        pp_ogr_rings[i]->getX(k), pp_ogr_rings[i]->getY(k));
    }
  }

  delete[]pp_ogr_rings;

  return true;
};

double MercatorTileMatrixSet::DegToRad(double ang)
{
  return ang * (3.14159265358979 / 180.0);
}

double MercatorTileMatrixSet::RadToDeg(double rad)
{
  return (rad / 3.14159265358979) * 180.0;
}


//	Converts from longitude to x coordinate 
double MercatorTileMatrixSet::MercX(double lon, MercatorProjType merc_type)
{
  return 6378137.0 * DegToRad(lon);
}


  //	Converts from x coordinate to longitude 
double MercatorTileMatrixSet::MecrToLong(double MercX, MercatorProjType merc_type)
{
  return RadToDeg(MercX / 6378137.0);
}

//	Converts from latitude to y coordinate 
double MercatorTileMatrixSet::MercY(double lat, MercatorProjType merc_type)
{
  if (merc_type == WORLD_MERCATOR)
  {
    if (lat > 89.5)		lat = 89.5;
    if (lat < -89.5)	lat = -89.5;
    double r_major = 6378137.000;
    double r_minor = 6356752.3142;
    double PI = 3.14159265358979;

    double temp = r_minor / r_major;
    double es = 1.0 - (temp * temp);
    double eccent = sqrt(es);
    double phi = DegToRad(lat);
    double sinphi = sin(phi);
    double con = eccent * sinphi;
    double com = .5 * eccent;
    con = pow(((1.0 - con) / (1.0 + con)), com);
    double ts = tan(.5 * ((PI*0.5) - phi)) / con;
    return 0 - r_major * log(ts);
  }
  else
  {
    double rad = DegToRad(lat);
    return  0.5 * 6378137 * log((1.0 + sin(rad)) / (1.0 - sin(rad)));
  }
}

  //	Converts from y coordinate to latitude 
double MercatorTileMatrixSet::MercToLat(double MercY, MercatorProjType merc_type)
{
  double r_major = 6378137.000;
  double r_minor = 6356752.3142;

  if (merc_type == WORLD_MERCATOR)
  {
    double temp = r_minor / r_major;
    double es = 1.0 - (temp * temp);
    double eccent = sqrt(es);
    double ts = exp(-MercY / r_major);
    double HALFPI = 1.5707963267948966;

    double eccnth, Phi, con, dphi;
    eccnth = 0.5 * eccent;

    Phi = HALFPI - 2.0 * atan(ts);

    double N_ITER = 15;
    double TOL = 1e-7;
    double i = N_ITER;
    dphi = 0.1;
    while ((fabs(dphi)>TOL) && (--i>0))
    {
      con = eccent * sin(Phi);
      dphi = HALFPI - 2.0 * atan(ts * pow((1.0 - con) / (1.0 + con), eccnth)) - Phi;
      Phi += dphi;
    }

    return RadToDeg(Phi);
  }
  else
  {
    return RadToDeg(1.5707963267948966 - (2.0 * atan(exp((-1.0 * MercY) / 6378137.0))));
  }
}


double	MercatorTileMatrixSet::ULX()
{
  return -20037508.3427812843076588408880691;
}

double	MercatorTileMatrixSet::ULY()
{
  return 20037508.3427812843076588408880691;
}

}