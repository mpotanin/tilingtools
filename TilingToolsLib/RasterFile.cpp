#include "StdAfx.h"
#include "RasterFile.h"

#include "StringFuncs.h"
#include "FileSystemFuncs.h"


namespace gmx
{


int _stdcall gmxPrintNoProgress ( double dfComplete, const char *pszMessage, void * pProgressArg )
{
	return 1;
}

/* 
void RasterFile::setGeoReference(double dResolution, double dULx, double dULy)
{
	this->resolution_ = dResolution;
	this->ul_x_ = dULx;
	this->ul_y_ = dULy;
	this->is_georeferenced_ = TRUE;
}
*/

/*
void RasterFile::delete_all()
{
	delete(p_gdal_ds_);
	p_gdal_ds_ = NULL;
	raster_file_	= "";
	num_bands_			= 0;
	//m_strImageFormat	= "";
	is_georeferenced_ = FALSE;
}
*/


bool RasterFile::Close()
{
	delete(p_gdal_ds_);
	p_gdal_ds_ = NULL;
	raster_file_	= "";
	num_bands_			= 0;
	//m_strImageFormat	= "";
	is_georeferenced_ = FALSE;
	
	return TRUE;
}


bool RasterFile::SetBackgroundToGDALDataset (GDALDataset *p_ds, BYTE background[3])
{
  ///
  //p_ds->GetRasterBand(1)->GetHistogram(
  ////

  if (!p_ds) return FALSE;
  if (p_ds->GetRasterCount() == 0 || p_ds->GetRasterXSize() == 0 || p_ds->GetRasterYSize()==0 || 
    p_ds->GetRasterBand(1) == NULL || p_ds->GetRasterBand(1)->GetRasterDataType() != GDT_Byte) return NULL;

  RasterBuffer rb;
  if (!rb.CreateBuffer(p_ds->GetRasterCount(),p_ds->GetRasterXSize(),p_ds->GetRasterYSize(),NULL,p_ds->GetRasterBand(1)->GetRasterDataType(),0,NULL))
    return FALSE;

  rb.InitByRGBColor(background);

  if (!p_ds->RasterIO(GF_Write,0,0,p_ds->GetRasterXSize(),p_ds->GetRasterYSize(),
                      rb.get_pixel_data_ref(),p_ds->GetRasterXSize(),p_ds->GetRasterYSize(),
                      rb.get_data_type(),rb.get_num_bands(),NULL,0,0,0)) 
                      return FALSE;
  
  return TRUE;
}


bool RasterFile::Init(string raster_file, bool is_geo_referenced, double shift_x, double shift_y)
{
	Close();
	GDALDriver *poDriver = NULL;
	
	p_gdal_ds_ = (GDALDataset *) GDALOpen(raster_file.c_str(), GA_ReadOnly );
	
	if (p_gdal_ds_==NULL)
	{
		cout<<"Error: RasterFile::init: can't open raster image"<<endl;
		return FALSE;
	}
	
	raster_file_ = raster_file;

	if (!(poDriver = p_gdal_ds_->GetDriver()))
	{
		cout<<"Error: RasterFile::Init: can't get GDALDriver from image"<<endl;
		return FALSE;
	}

	num_bands_	= p_gdal_ds_->GetRasterCount();
	height_ = p_gdal_ds_->GetRasterYSize();
	width_	= p_gdal_ds_->GetRasterXSize();
  int nodata_value_defined;
	nodata_value_ = p_gdal_ds_->GetRasterBand(1)->GetNoDataValue(&nodata_value_defined);
  this->nodata_value_defined_ = nodata_value_defined;
	gdal_data_type_	= p_gdal_ds_->GetRasterBand(1)->GetRasterDataType();


	if (is_geo_referenced)
	{
		double GeoTransform[6];
		if (p_gdal_ds_->GetGeoTransform(GeoTransform)== CE_None)			
		{
			ul_x_ = GeoTransform[0] + shift_x;
			ul_y_ = GeoTransform[3] + shift_y;
			resolution_ = GeoTransform[1];
			is_georeferenced_ = TRUE;
		}
		else
		{
			cout<<"Error: RasterFile::Init: can't read georeference"<<endl;
			return FALSE;
		}
	}
	
	return TRUE;
}


bool	RasterFile::CalcBandStatistics(int band_num, double &min, double &max, double &mean, double &std,  double *p_nodata_val)
{
	if (this->p_gdal_ds_ == NULL) return FALSE;
  if (this->p_gdal_ds_->GetRasterCount()<band_num) return FALSE;
  if (p_nodata_val) this->p_gdal_ds_->GetRasterBand(band_num)->SetNoDataValue(*p_nodata_val);
  return (CE_None==p_gdal_ds_->GetRasterBand(band_num)->ComputeStatistics(0,&min,&max,&mean,&std,NULL,NULL));
}


double RasterFile::get_nodata_value (bool &nodata_defined)
{
  nodata_defined = nodata_value_defined_;
  return nodata_value_;
}


RasterFile::RasterFile()
{
	///*
	//*/
	p_gdal_ds_ = NULL;
	raster_file_	= "";
	num_bands_			= 0;
	//m_strImageFormat	= "";
	is_georeferenced_ = FALSE;
	nodata_value_ = 0;
  nodata_value_defined_ = FALSE;
}

RasterFile::RasterFile(string raster_file, bool is_geo_referenced)
{
	//*/
	p_gdal_ds_ = NULL;
	raster_file_	= "";
	num_bands_			= 0;
	//m_strImageFormat	= "";
	is_georeferenced_ = FALSE;
	nodata_value_ = 0;
	
	Init(raster_file,is_geo_referenced);
}


RasterFile::~RasterFile(void)
{
	Close();
}



void RasterFile::GetPixelSize (int &width, int &height)
{
	if (this->p_gdal_ds_ == NULL)
		width=0;
	else
		width = this->p_gdal_ds_->GetRasterXSize();

	if (this->p_gdal_ds_ == NULL)
		height=0;
	else
		height = this->p_gdal_ds_->GetRasterYSize();
}

/*
void RasterFile::getGeoReference (double &dULx, double &dULy, double &res)
{
	if (this->is_georeferenced_)
	{
		dULx = this->ul_x_;
		dULy = this->ul_y_;
		res = this->resolution_;
	}
	else
	{
		dULx =0;
		dULy =0;
		res =0;
	}
}
*/

/*
double	RasterFile::get_resolution()
{
		return this->resolution_;
}
*/


OGREnvelope RasterFile::GetEnvelope()
{
	OGREnvelope envp;
	envp.MinX = ul_x_;
	envp.MaxY = ul_y_;
	envp.MaxX = ul_x_ + width_*resolution_;
	envp.MinY = ul_y_ - height_*resolution_;

	return envp;
};


GDALDataset*	RasterFile::get_gdal_ds_ref()
{
	return this->p_gdal_ds_;
}


bool RasterFile::ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference *p_ogr_sr)
{
	FILE *fp = OpenFile(tab_file,"r");
	if (!fp) return FALSE;
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *pchar_tab_file_data = new char[size];
	fread(pchar_tab_file_data,sizeof(char),size,fp);
	string str_tab_file_data(pchar_tab_file_data);
	delete[]pchar_tab_file_data;
	if (str_tab_file_data.find("CoordSys Earth Projection") == string::npos) return FALSE;
	int start_pos	= str_tab_file_data.find("CoordSys Earth Projection");
	int end_pos		= (str_tab_file_data.find('\n',start_pos) != string::npos)		? 
														str_tab_file_data.size() : 
														str_tab_file_data.find('\n',start_pos);
	string strMapinfoProj = str_tab_file_data.substr(start_pos, end_pos-start_pos+1);
	if (!OGRERR_NONE==p_ogr_sr->importFromMICoordSys(strMapinfoProj.c_str())) return FALSE;
	
	return TRUE;
}


bool	RasterFile::GetSpatialRef(OGRSpatialReference	&ogr_sr)
{
	const char* strProjRef = GDALGetProjectionRef(this->p_gdal_ds_);

	if (OGRERR_NONE == ogr_sr.SetFromUserInput(strProjRef)) return TRUE;
  else if (FileExists(RemoveExtension(this->raster_file_)+".prj"))
	{
		string prjFile		= RemoveExtension(this->raster_file_)+".prj";
		if (OGRERR_NONE==ogr_sr.SetFromUserInput(prjFile.c_str())) return TRUE;	
	}
	else if (FileExists(RemoveExtension(this->raster_file_)+".tab"))
  {
    string tabFile = RemoveExtension(this->raster_file_)+".tab";
    if (ReadSpatialRefFromMapinfoTabFile(tabFile,&ogr_sr)) return TRUE;
  }
 
	return FALSE;
}

bool	RasterFile::GetDefaultSpatialRef (OGRSpatialReference	&ogr_sr, MercatorProjType merc_type)
{
	if (fabs(this->ul_x_)<180 && fabs(this->ul_y_)<90)
	{
		ogr_sr.SetWellKnownGeogCS("WGS84"); 
	}
	else
	{
		if (merc_type == WORLD_MERCATOR)
		{
			ogr_sr.SetWellKnownGeogCS("WGS84");
			ogr_sr.SetMercator(0,0,1,0,0);
		}
		else
		{
			ogr_sr.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
		}
	}

	return TRUE;
}


OGREnvelope*	RasterFile::CalcMercEnvelope (MercatorProjType	merc_type)
{
	const int num_points = 100;
	OGRSpatialReference ogr_sr;

	if (!GetSpatialRef(ogr_sr)) GetDefaultSpatialRef(ogr_sr,merc_type);

	OGRLinearRing lr;

	OGREnvelope		src_envp = GetEnvelope();
	lr.addPoint(src_envp.MinX,src_envp.MaxY);
	for (int i=1; i<num_points;i++)
		lr.addPoint(src_envp.MinX + i*(src_envp.MaxX-src_envp.MinX)/num_points , src_envp.MaxY);
	
	lr.addPoint(src_envp.MaxX,src_envp.MaxY);
	for (int i=1; i<num_points;i++)
		lr.addPoint(src_envp.MaxX,src_envp.MaxY - i*(src_envp.MaxY-src_envp.MinY)/num_points);

	lr.addPoint(src_envp.MaxX,src_envp.MinY);
	for (int i=1; i<num_points;i++)
		lr.addPoint(src_envp.MaxX - i*(src_envp.MaxX-src_envp.MinX)/num_points , src_envp.MinY);
	
	lr.addPoint(src_envp.MinX,src_envp.MinY);
	for (int i=1; i<num_points;i++)
		lr.addPoint(src_envp.MinX,src_envp.MinY + i*(src_envp.MaxY-src_envp.MinY)/num_points);
	lr.closeRings();

	lr.assignSpatialReference(&ogr_sr);
	
	OGRSpatialReference ogr_merc_sr;
	MercatorTileGrid::SetMercatorSpatialReference(merc_type,&ogr_merc_sr);
	
	bool	intersects180 = VectorBorder::Intersects180Degree(&lr,&ogr_sr);
	if (OGRERR_NONE != lr.transformTo(&ogr_merc_sr)) return NULL;
	
	if (intersects180) VectorBorder::AdjustFor180DegreeIntersection(&lr);


	OGREnvelope *p_result_envp = new OGREnvelope();
	lr.getEnvelope(p_result_envp);

	return	p_result_envp;
}	




/*
void RasterFile::readMetaData ()
{
	const char *proj_data = this->p_gdal_ds_->GetProjectionRef();
	OGRSpatialReference oSpRef;
	OGRErr err = oSpRef.SetWellKnownGeogCS(proj_data);
};
*/


RasterFileBundle::RasterFileBundle(void)
{
	//this->m_poImages = NULL;
	//this->m_nLength = 0;
}

void RasterFileBundle::Close(void)
{
	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
		delete((*iter).second.second);
		delete((*iter).second.first);
	}
	data_list_.empty();

}

RasterFileBundle::~RasterFileBundle(void)
{
	Close();
}


int	RasterFileBundle::Init (list<string> file_list, MercatorProjType merc_type, string vector_file, double shift_x, double shift_y)
{
	Close();

	if (file_list.size() == 0) return 0;

	merc_type_ = merc_type;

 	for (std::list<string>::iterator iter = file_list.begin(); iter!=file_list.end(); iter++)
	{
    if (file_list.size() > 1 && vector_file == "")
      AddItemToBundle((*iter),VectorBorder::GetVectorFileNameByRasterFileName(*iter),shift_x,shift_y);
    else AddItemToBundle((*iter),vector_file,shift_x,shift_y);
	}
	return data_list_.size();
}


GDALDataType RasterFileBundle::GetRasterFileType()
{
  RasterFile rf;
  if (data_list_.size()==0) return GDT_Byte;
  else if (!rf.Init(*GetFileList().begin(),true)) return GDT_Byte;
  else return rf.get_gdal_ds_ref()->GetRasterBand(1)->GetRasterDataType();

}

bool	RasterFileBundle::AddItemToBundle (string raster_file, string vector_file, double shift_x, double shift_y)
{	
	RasterFile image;

	if (!image.Init(raster_file,TRUE,shift_x,shift_y))
	{
		cout<<"Error: can't init. image: "<<raster_file<<endl;
		return 0;
	}

	VectorBorder	*border = VectorBorder::CreateFromVectorFile(vector_file,merc_type_);
	pair<string,pair<OGREnvelope*,VectorBorder*>> p;
	p.first			= raster_file;
	p.second.first	= image.CalcMercEnvelope(merc_type_);
	p.second.second = border;
	if ((p.second.first == NULL) && (p.second.second == NULL)) return FALSE;
	data_list_.push_back(p);
	return TRUE;
}

list<string>	RasterFileBundle::GetFileList()
{
	std::list<string> file_list;
	for (std::list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end(); iter++)
		file_list.push_back((*iter).first);

	return file_list;
	//return this->m_strFilesList;
}

OGREnvelope RasterFileBundle::CalcMercEnvelope()
{
	OGREnvelope	envp;


	envp.MaxY=(envp.MaxX = -1e+100);envp.MinY=(envp.MinX = 1e+100);
	if (data_list_.size() == 0) return envp;

	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
		if ((*iter).second.second != NULL)
			envp = VectorBorder::CombineOGREnvelopes(envp, 
             VectorBorder::InetersectOGREnvelopes((*iter).second.second->GetEnvelope(),
                                                  *(*iter).second.first
                                                  ));
    else if ((*iter).second.first != NULL)
			envp = VectorBorder::CombineOGREnvelopes(envp,*(*iter).second.first);
	}
	return envp; 
}


int	RasterFileBundle::CalcNumberOfTiles (int zoom)
{
	int n = 0;
	double		res = MercatorTileGrid::CalcResolutionByZoom(zoom);

	int minx,maxx,miny,maxy;
	MercatorTileGrid::CalcTileRange(CalcMercEnvelope(),zoom,minx,miny,maxx,maxy);
	
	for (int curr_x = minx; curr_x<=maxx; curr_x++)
	{
		for (int curr_y = miny; curr_y<=maxy; curr_y++)
		{
			OGREnvelope tile_envp = MercatorTileGrid::CalcEnvelopeByTile(zoom,curr_x,curr_y);
			if (Intersects(tile_envp)) n++;
		}
	}

	return n;
}

double RasterFileBundle::GetNodataValue(bool &nodata_defined)
{
  if (data_list_.size()==0) return NULL;
  RasterFile rf((*data_list_.begin()).first,1);
  return rf.get_nodata_value(nodata_defined);
}




int		RasterFileBundle::CalcBestMercZoom()
{
	if (data_list_.size()==0) return -1;

	if ((*data_list_.begin()).second.first == NULL) return -1;

	RasterFile rf((*data_list_.begin()).first,1);
	int width_src = 0, height_src = 0;
	rf.GetPixelSize(width_src,height_src);
	if (width_src<=0 || height_src <= 0) return false;
	OGREnvelope envp_merc(*(*data_list_.begin()).second.first);

	double res_src = min((envp_merc.MaxX - envp_merc.MinX)/width_src,(envp_merc.MaxY - envp_merc.MinY)/height_src);
	if (res_src<=0) return -1;

	for (int z=0; z<32; z++)
	{
		if (MercatorTileGrid::CalcResolutionByZoom(z) <res_src || 
			(fabs(MercatorTileGrid::CalcResolutionByZoom(z)-res_src)/MercatorTileGrid::CalcResolutionByZoom(z))<0.2) return z;
	}

	return -1;
}


list<string>	 RasterFileBundle::GetFileListByEnvelope(OGREnvelope envp_merc)
{
	std::list<string> file_list;
	
	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
		if ((*iter).second.second->Intersects(envp_merc)) file_list.push_back((*iter).first);

	}
	
	return file_list;
}


bool	RasterFileBundle::Intersects(OGREnvelope envp_merc)
{
	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
		if ((*iter).second.second != NULL)
    {
			if ((*iter).second.second->Intersects(envp_merc) && 
          (*iter).second.first->Intersects(envp_merc) ) return TRUE;
    }
    else if ((*iter).second.first != NULL)
    {
			if ((*iter).second.first->Intersects(envp_merc)) return TRUE;
    }
	}
	
	return FALSE;
}

///*
bool RasterFileBundle::CalclValuesForStretchingTo8Bit (double *&p_min_values,
                                                       double *&p_max_values,
                                                       double *p_nodata_val,
                                                       BandMapping    *p_band_mapping)
{
  if (this->data_list_.size()==0) return FALSE;
  p_min_values = (p_max_values = 0);
  int bands_num;
  double min,max,mean,std;
   
  if (!p_band_mapping)
  {
    RasterFile rf;
    if (!rf.Init(*GetFileList().begin(),true)) return FALSE;
    bands_num = rf.get_gdal_ds_ref()->GetRasterCount();
    p_min_values = new double[bands_num];
    p_max_values = new double[bands_num]; 
    for (int b=0;b<bands_num;b++)
    {
      if (!rf.CalcBandStatistics(b+1,min,max,mean,std,p_nodata_val))
      {
        delete[]p_min_values;delete[]p_max_values;
        p_min_values=0;p_max_values=0;
        return FALSE;
      }
     	p_min_values[b] = mean - 2*std;
  		p_max_values[b] = mean + 2*std;
    }
  }
  else
  {
    if (!(bands_num = p_band_mapping->GetBandsNum())) return FALSE;
    p_min_values = new double[bands_num];
    p_max_values = new double[bands_num]; 
    list<string> file_list = GetFileList();
    
    for (int b=0;b<bands_num;b++)
      p_min_values[b]=(p_max_values[b]=0);

    for (int b=0;b<bands_num;b++)
    {
      for (list<string>::iterator iter=file_list.begin();iter!=file_list.end();iter++)
      {
        int _bands_num;
        int *p_bands=0;
        p_band_mapping->GetBands((*iter),_bands_num,p_bands);
        if (p_bands[b]>0)
        {
          RasterFile rf;
          bool error=false;
          if (!rf.Init(*iter,true)) error=true;
          else if (!rf.CalcBandStatistics(p_bands[b],min,max,mean,std,p_nodata_val)) error=true;
          
          if (error)
          {
            delete[]p_min_values;delete[]p_max_values;
            return FALSE;
          }
          else
          {
            p_min_values[b] = mean - 2*std;
  		      p_max_values[b] = mean + 2*std;
            delete[]p_bands;
            break;
          }
        }
        else delete[]p_bands;
    
      }
    }
  }


  return TRUE;
}
//*/

DWORD WINAPI GMXAsyncWarpMulti (LPVOID lpParam)
{
  GMXAsyncWarpMultiParams   *p_warp_multi_params = (GMXAsyncWarpMultiParams*)lpParam;
  if (CE_None != p_warp_multi_params->p_warp_operation_->ChunkAndWarpMulti(0,
                                                            0,
                                                            p_warp_multi_params->buf_width_,
                                                            p_warp_multi_params->buf_height_))
    (*p_warp_multi_params->p_warp_error)=true;
  p_warp_multi_params->is_done_ = 1;
  return true;
}

bool RasterFileBundle::WarpToMercBuffer (int zoom,	
                                            OGREnvelope	  envp_merc, 
                                            RasterBuffer *p_dst_buffer, 
                                            int           output_bands_num,
                                            int           **pp_band_mapping,
                                            string        resampling_alg, 
                                            BYTE          *p_nodata,
                                            BYTE          *p_background_color,
                                            bool           warp_multithread)
{

	if (data_list_.size()==0) return FALSE;
	GDALDataset	*p_src_ds = (GDALDataset*)GDALOpen((*data_list_.begin()).first.c_str(),GA_ReadOnly );
	if (p_src_ds==NULL)
	{
		cout<<"Error: can't open raster file: "<<(*data_list_.begin()).first<<endl;
		return FALSE;
	}
  GDALDataType	dt		= GDALGetRasterDataType(GDALGetRasterBand(p_src_ds,1));
	int       bands_num_src   = p_src_ds->GetRasterCount();
  int				bands_num_dst	= (output_bands_num==0) ? bands_num_src : output_bands_num;
 
    
  int		nodata_val_from_file_defined_int = false;
  double			nodata_val_from_file = (int) p_src_ds->GetRasterBand(1)->GetNoDataValue(&nodata_val_from_file_defined_int);
  bool nodata_val_from_file_defined = nodata_val_from_file_defined_int;
	
	double		res			=  MercatorTileGrid::CalcResolutionByZoom(zoom);
	int				buf_width	= int(((envp_merc.MaxX - envp_merc.MinX)/res)+0.5);
	int				buf_height	= int(((envp_merc.MaxY - envp_merc.MinY)/res)+0.5);

  srand(0);
  string			tiff_in_mem = ("/vsimem/tiffinmem" + ConvertIntToString(rand()));
	
  GDALDataset*	p_vrt_ds = (GDALDataset*)GDALCreate(
								GDALGetDriverByName("GTiff"),
								tiff_in_mem.c_str(),
								buf_width,
								buf_height,
								bands_num_dst,
								dt,
								NULL
								);


	if (p_src_ds->GetRasterBand(1)->GetColorTable())
		p_vrt_ds->GetRasterBand(1)->SetColorTable(p_src_ds->GetRasterBand(1)->GetColorTable());
	GDALClose(p_src_ds);

	double			geotransform[6];
	geotransform[0] = envp_merc.MinX;
	geotransform[1] = res;
	geotransform[2] = 0;
	geotransform[3] = envp_merc.MaxY;
	geotransform[4] = 0;
	geotransform[5] = -res;
	p_vrt_ds->SetGeoTransform(geotransform);
	OGRSpatialReference output_ogr_sr;
	char *p_dst_wkt = NULL;
	if (merc_type_ == WORLD_MERCATOR)
	{
		output_ogr_sr.SetWellKnownGeogCS("WGS84");
		output_ogr_sr.SetMercator(0,0,1,0,0);
	}
	else
	{
		output_ogr_sr.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
	}
	output_ogr_sr.exportToWkt( &p_dst_wkt );
	p_vrt_ds->SetProjection(p_dst_wkt);


  if (p_background_color)
  {
    RasterFile::SetBackgroundToGDALDataset(p_vrt_ds,p_background_color);
  }
  else if ((p_nodata || nodata_val_from_file_defined) && (bands_num_dst<=3) ) 
  {
    BYTE rgb[3];
    if (p_nodata) memcpy(rgb,p_nodata,3);
    else rgb[0] = (rgb[1] = (rgb[2] = (int)nodata_val_from_file));
    
    RasterFile::SetBackgroundToGDALDataset(p_vrt_ds,rgb);
  }


  
  int file_num = -1;

	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
    file_num++;
		//check if image envelope Intersects destination buffer envelope
		if ((*iter).second.first != NULL)
		{
			if (!(*iter).second.first->Intersects(envp_merc)) continue;
		}
		
		if ((*iter).second.second != NULL)
		{
			if (!(*iter).second.second->Intersects(envp_merc)) continue;
		}

		// Open input raster and create source dataset
		if (data_list_.size()==0) return FALSE;
		p_src_ds = (GDALDataset*)GDALOpen((*iter).first.c_str(),GA_ReadOnly );

	
		if (p_src_ds==NULL)
		{
			cout<<"Error: can't open raster file: "<<(*iter).first<<endl;
			continue;
		}
			
		// Get Source coordinate system and set destination  
		char *p_src_wkt	= NULL;
		RasterFile	input_rf((*iter).first,TRUE);
		OGRSpatialReference input_ogr_sr;
		if (!input_rf.GetSpatialRef(input_ogr_sr))
		{
			if(!input_rf.GetDefaultSpatialRef(input_ogr_sr,merc_type_))
			{
				cout<<"Error: can't read spatial reference from input file: "<<(*data_list_.begin()).first<<endl;
				return FALSE;
			}
		}
		input_ogr_sr.exportToWkt(&p_src_wkt);
		CPLAssert( p_src_wkt != NULL && strlen(p_src_wkt) > 0 );

    GDALWarpOptions *p_warp_options = GDALCreateWarpOptions();
    p_warp_options->papszWarpOptions = NULL;
    if (warp_multithread) 
      p_warp_options->papszWarpOptions = CSLSetNameValue(p_warp_options->papszWarpOptions,"NUM_THREADS", "ALL_CPUS");

		p_warp_options->hSrcDS = p_src_ds;
		p_warp_options->hDstDS = p_vrt_ds;
		p_warp_options->dfWarpMemoryLimit = 150000000; 
		double			error_threshold = 0.125;
    
    p_warp_options->panSrcBands = new int[bands_num_dst];
    p_warp_options->panDstBands = new int[bands_num_dst];
		if (pp_band_mapping)
    {
      int warp_bands_num =0;
      for (int i=0; i<bands_num_dst; i++)
      {
        if (pp_band_mapping[file_num][i]>0)
        {
          p_warp_options->panSrcBands[warp_bands_num] = pp_band_mapping[file_num][i];
          p_warp_options->panDstBands[warp_bands_num] = i+1;
          warp_bands_num++;
        }
      }
      p_warp_options->nBandCount = warp_bands_num;
    }
    else
    {
      p_warp_options->nBandCount = bands_num_dst;
      for( int i = 0; i < bands_num_dst; i++ )
        p_warp_options->panSrcBands[i] = (p_warp_options->panDstBands[i] = i+1);
    }

		if ((*iter).second.second)
		{
			VectorBorder	*p_vb = (*iter).second.second;
			//OGRGeometry		*p_ogr_geom = p_vb->GetOGRGeometryTransformed(&input_ogr_sr);
			double	gdal_transform[6];
			if (CE_None == input_rf.get_gdal_ds_ref()->GetGeoTransform(gdal_transform))
			{
				if (!((gdal_transform[0] == 0.) &&(gdal_transform[1]==1.)))
					p_warp_options->hCutline = p_vb->GetOGRGeometryTransformedToPixelLine(&input_ogr_sr,gdal_transform);
			}
		}
  
    if (p_nodata)
    {
      p_warp_options->padfSrcNoDataReal = new double[bands_num_dst];
      p_warp_options->padfSrcNoDataImag = new double[bands_num_dst];
      if (bands_num_dst==3)
      {
        p_warp_options->padfSrcNoDataReal[0] = p_nodata[0];
        p_warp_options->padfSrcNoDataReal[1] = p_nodata[1];
        p_warp_options->padfSrcNoDataReal[2] = p_nodata[2];
        p_warp_options->padfSrcNoDataImag[0] = (p_warp_options->padfSrcNoDataImag[1] = (p_warp_options->padfSrcNoDataImag[2] = 0));
      }
      else
      {
        for (int i=0;i<bands_num_dst;i++)
        {
          p_warp_options->padfSrcNoDataReal[i] = p_nodata[0];
          p_warp_options->padfSrcNoDataImag[i] = 0;
        }
      }
    }
    else if (nodata_val_from_file_defined)
    {
      p_warp_options->padfSrcNoDataReal = new double[bands_num_dst];
      p_warp_options->padfSrcNoDataImag = new double[bands_num_dst];
      for (int i=0;i<bands_num_dst;i++)
      {
          p_warp_options->padfSrcNoDataReal[i] = nodata_val_from_file;
          p_warp_options->padfSrcNoDataImag[i] = 0;
      }
    }
    
    p_warp_options->pfnProgress = gmxPrintNoProgress;  

		p_warp_options->pTransformerArg = 
				GDALCreateApproxTransformer( GDALGenImgProjTransform, 
											  GDALCreateGenImgProjTransformer(  p_src_ds, 
																				p_src_wkt, 
																				p_vrt_ds,//hDstDS,
																				p_dst_wkt,//GDALGetProjectionRef(hDstDS), 
																				FALSE, 0.0, 1 ),
											 error_threshold );

		p_warp_options->pfnTransformer = GDALApproxTransform;

    resampling_alg = MakeLower(resampling_alg);
    if (  resampling_alg == "near" || 
          resampling_alg == "nearest" || 
          p_vrt_ds->GetRasterBand(1)->GetColorTable()
       ) p_warp_options->eResampleAlg = GRA_NearestNeighbour;
    else if (resampling_alg == "bilinear") p_warp_options->eResampleAlg = GRA_Bilinear;
    else if (resampling_alg == "lanczos") p_warp_options->eResampleAlg = GRA_Lanczos;
    else p_warp_options->eResampleAlg = GRA_Cubic; 
    
     // Initialize and execute the warp operation. 
		GDALWarpOperation gdal_warp_operation;
		gdal_warp_operation.Initialize( p_warp_options );
		
   
    bool  warp_error = FALSE;
    
    if (! warp_multithread)
      warp_error = (CE_None != gdal_warp_operation.ChunkAndWarpImage( 0,0,buf_width,buf_height));
    else
    {
      int warp_attempt=0;
      for (warp_attempt=0; warp_attempt<2;  warp_attempt++)
      {
        unsigned long thread_id;
      
        GMXAsyncWarpMultiParams warp_multi_params;
        warp_multi_params.buf_height_=buf_height;
        warp_multi_params.buf_width_=buf_width;
        warp_multi_params.p_warp_operation_= &gdal_warp_operation;
        warp_multi_params.is_done_=0;
        warp_multi_params.p_warp_error=&warp_error;
        HANDLE hThread = CreateThread(NULL,0,GMXAsyncWarpMulti,&warp_multi_params,0,&thread_id); 
        for (int i=0;i<120;i++)
        {
          if (warp_multi_params.is_done_==1) break;
          else Sleep(250);
        }
        if (warp_multi_params.is_done_==0)
        {
          TerminateThread(hThread,1);
          CloseHandle(hThread);
        }
        else
        {
          CloseHandle(hThread);
          break;
        }
      }
      if (warp_attempt==2) warp_error = true;
    }
    
    GDALDestroyApproxTransformer(p_warp_options->pTransformerArg );
    if (p_warp_options->hCutline)
    {
      ((OGRGeometry*)p_warp_options->hCutline)->empty();
      p_warp_options->hCutline = NULL;
    }

    if (p_warp_options->padfSrcNoDataReal)
    {
      delete[]p_warp_options->padfSrcNoDataReal;
      delete[]p_warp_options->padfSrcNoDataImag;
      p_warp_options->padfSrcNoDataReal = NULL;
      p_warp_options->padfSrcNoDataImag = NULL;
    }

    delete[]p_warp_options->panSrcBands;
    delete[]p_warp_options->panDstBands;
    p_warp_options->panSrcBands = NULL;
    p_warp_options->panDstBands = NULL;
   
		GDALDestroyWarpOptions( p_warp_options );
		OGRFree(p_src_wkt);
		input_rf.Close();
		GDALClose( p_src_ds );
    if (warp_error) 
    {
      cout<<"Error: warping raster block of image: "<<(*iter).first<<endl;
      return FALSE;
    }
	}

	p_dst_buffer->CreateBuffer(bands_num_dst,buf_width,buf_height,NULL,dt,FALSE,p_vrt_ds->GetRasterBand(1)->GetColorTable());

  p_vrt_ds->RasterIO(	GF_Read,0,0,buf_width,buf_height,p_dst_buffer->get_pixel_data_ref(),
						buf_width,buf_height,p_dst_buffer->get_data_type(),
						p_dst_buffer->get_num_bands(),NULL,0,0,0);
  
  OGRFree(p_dst_wkt);
	GDALClose(p_vrt_ds);
	VSIUnlink(tiff_in_mem.c_str());
	return TRUE;
}




BandMapping::~BandMapping()
{
  bands_num_=0;
  for ( map<string,int*>::iterator iter = data_map_.begin(); iter!=data_map_.end(); iter++)
    delete[](*iter).second;
  data_map_.empty();
}


bool BandMapping::GetBandMappingData (int &output_bands_num, int **&pp_band_mapping)
{
  output_bands_num = 0;
  pp_band_mapping = 0;

  if (!bands_num_) return TRUE;
  else output_bands_num = bands_num_;
  
  int i = 0;
  for ( map<string,int*>::iterator iter = data_map_.begin(); iter!=data_map_.end(); iter++)
  {
    if ((*iter).second)
    {
      if (!pp_band_mapping)
      {
        pp_band_mapping = new int*[data_map_.size()];
        for (int j=0;j<data_map_.size();j++)
          pp_band_mapping[j]=0;
      }
      pp_band_mapping[i] = new int[bands_num_];
      memcpy(pp_band_mapping[i],(*iter).second,bands_num_*sizeof(int));
    }
    i++;
  }
    
  return TRUE;
}


bool BandMapping::GetBands(string file_name, int &bands_num, int *&p_bands)
{
  bands_num=0;
  p_bands = NULL;
  map<string,int*>::iterator iter;

  if ((iter=data_map_.find(file_name))==data_map_.end()) 
    return FALSE;
  else
  {
    if (bands_num_ && (*iter).second)
    {
      bands_num=bands_num_;
      p_bands = new int[bands_num_];
      memcpy(p_bands,(*iter).second,bands_num_*sizeof(int));
    }
    return TRUE;
  }
}

list<string> BandMapping::GetFileList ()
{
  list<string> file_list;
  for (map<string,int*>::iterator iter = data_map_.begin(); iter != data_map_.end(); iter++)\
    file_list.push_back((*iter).first);
  return file_list;
}


bool  BandMapping::AddFile(string file_name, int *p_bands)
{
  if ((!p_bands) || (bands_num_==0)) return FALSE;

  int *_p_bands = new int[bands_num_];
  memcpy(_p_bands,p_bands,bands_num_*sizeof(int));
  map<string,int*>::iterator iter;

  if ((iter=data_map_.find(file_name))!=data_map_.end())
     data_map_.insert(pair<string,int*>(file_name,_p_bands));
  else
  {
    delete[](*iter).second;
    (*iter).second = _p_bands;
  }
  return TRUE;
}


bool  BandMapping::InitLandsat8  (string file_param, string bands_param)
{
  regex	rx_landsat8(".*\\?landsat8");
  if (!regex_match(file_param,rx_landsat8)) return FALSE;

  //ToDo
  //ParseBands
  //ReadFiles

  return TRUE;
}



bool  BandMapping::InitByConsoleParams (string file_param, string bands_param)
{
  regex	rx_landsat8(".*\\?landsat8");
  if (regex_match(file_param,rx_landsat8)) return InitLandsat8(file_param,bands_param);
  else
  {
    list<string> file_list;
    int **pp_band_mapping = 0;
    int *p_bands = 0;
    if (!ParseFileParameter(file_param,file_list,bands_num_,pp_band_mapping))
      return FALSE;
    
    if ((!pp_band_mapping) && (bands_param!=""))
    {
      if (!(bands_num_=ParseCommaSeparatedArray(bands_param,p_bands)))
        return FALSE;
    }

    int i=0;
    for (list<string>::iterator iter=file_list.begin();iter!=file_list.end();iter++)
    {
      int *_p_bands=0;
      if (pp_band_mapping)
      {
        if (pp_band_mapping[i])
        {
          _p_bands = new int[bands_num_];
          memcpy(_p_bands,pp_band_mapping[i],bands_num_*sizeof(int));
        }
      }
      else if (p_bands)
      {
        _p_bands = new int[bands_num_];
        memcpy(_p_bands,p_bands,bands_num_*sizeof(int));
      }
      data_map_.insert(pair<string,int*>(*iter,_p_bands));
      i++;
    }
  }

  return TRUE;
}


bool  BandMapping::ParseFileParameter (string str_file_param, list<string> &file_list, int &output_bands_num, int **&pp_band_mapping)
{
  string _str_file_param = str_file_param + '|';
  output_bands_num = 0;
  pp_band_mapping = 0;
  std::string::size_type stdf;

  while (_str_file_param.length()>1)
  {
    string item = _str_file_param.substr(0,_str_file_param.find('|'));
    if (item.find('?')!=  string::npos)
    {
      int *p_arr = 0;
      int len = gmx::ParseCommaSeparatedArray(item.substr(item.find('?')+1),p_arr,true,0);
      if (p_arr) delete[]p_arr;
      if (len==0) 
      {
        cout<<"Error: can't parse output bands order from: "<<item.substr(item.find('?')+1)<<endl;
        file_list.empty();
        output_bands_num = 0;
        return false;
      }
      output_bands_num = (int)max(output_bands_num,len);
      item = item.substr(0,item.find('?'));
    }

    if (!gmx::FindFilesByPattern(file_list,item))
    {
      cout<<"Error: can't find input files by path: "<<item<<endl;
      file_list.empty();
      output_bands_num = 0;
      return false;
    }
    
    _str_file_param = _str_file_param.substr(_str_file_param.find('|')+1);
  }

  if (output_bands_num>0)
  {
    pp_band_mapping = new int*[file_list.size()];
    for (int i=0;i<file_list.size();i++)
    {
       pp_band_mapping[i] = new int[output_bands_num];
       for (int j=0;j<output_bands_num;j++)
         pp_band_mapping[i][j] = j+1;
    }
  }

  _str_file_param = str_file_param + '|';
  int i=0;
  while (_str_file_param.length()>1)
  {
    string item = _str_file_param.substr(0,_str_file_param.find('|'));
    list<string> _file_list;
    if (item.find('?')!=string::npos)
    {
      gmx::FindFilesByPattern(_file_list,item.substr(0,item.find('?')));
      int *p_arr = 0;
      int len = gmx::ParseCommaSeparatedArray(item.substr(item.find('?')+1),p_arr,true,0);
      if (p_arr)
      {
        for (int j=i;j<i+_file_list.size();j++)
        {
          for (int k=0;k<len;k++)
            pp_band_mapping[j][k]=p_arr[k];
        }
        delete[]p_arr;
      }
    }
    else gmx::FindFilesByPattern(_file_list,item);
       
    i+=_file_list.size();
    _str_file_param = _str_file_param.substr(_str_file_param.find('|')+1);
  }

  return TRUE;
}

}