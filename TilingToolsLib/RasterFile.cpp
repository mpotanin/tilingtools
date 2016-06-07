#include "StdAfx.h"
#include "RasterFile.h"

#include "StringFuncs.h"
#include "FileSystemFuncs.h"



DWORD WINAPI GMXAsyncWarpChunkAndMakeTiling (LPVOID lpParam)
{
  gmx::CURR_WORK_THREADS++;

  GMXAsyncChunkTilingParams   *p_chunk_tiling_params = (GMXAsyncChunkTilingParams*)lpParam;
  
  gmx::TilingParameters		*p_tiling_params = p_chunk_tiling_params->p_tiling_params_;
  gmx::BundleTiler		*p_bundle = p_chunk_tiling_params->p_bundle_;
  gmx::ITileContainer			  *p_tile_container = p_chunk_tiling_params->p_tile_container_;
  int                   zoom = p_chunk_tiling_params->z_;
  OGREnvelope           chunk_envp = p_chunk_tiling_params->chunk_envp_;
   
  gmx::RasterBuffer *p_merc_buffer = new gmx::RasterBuffer();
  WaitForSingleObject(gmx::WARP_SEMAPHORE,INFINITE);

  //ToDo...

  int bands_num=0;
  int **pp_band_mapping=0;
  //ToDo 
  if (p_tiling_params->p_band_mapping_)
   p_tiling_params->p_band_mapping_->GetBandMappingData(bands_num,pp_band_mapping);
    
  bool warp_result = p_bundle->WarpChunkToBuffer(zoom,
                                                chunk_envp,
                                                p_merc_buffer,
                                                bands_num,
                                                pp_band_mapping,
                                                p_tiling_params->gdal_resampling_,
                                                p_tiling_params->p_transparent_color_,
                                                p_tiling_params->p_background_color_,
                                                p_tiling_params->max_work_threads_!=1);
  ReleaseSemaphore(gmx::WARP_SEMAPHORE,1,NULL);

  
  if (!warp_result)	
  {
	  cout<<"Error: BaseZoomTiling: warping to merc fail"<<endl;
    gmx::CURR_WORK_THREADS--;
	  return FALSE;
	}

  if (p_chunk_tiling_params->need_stretching_)	
  {
    if (! p_merc_buffer->StretchDataTo8Bit (
                          p_chunk_tiling_params->p_stretch_min_values_,
                          p_chunk_tiling_params->p_stretch_max_values_))
    {
      cout<<"Error: can't stretch raster values to 8 bit"<<endl;
      gmx::CURR_WORK_THREADS--;
			return FALSE;
		}
	}

  int                   tiles_expected = p_chunk_tiling_params->tiles_expected_;
  int                   *p_tiles_generated = p_chunk_tiling_params->p_tiles_generated_;

  if (!p_bundle->RunTilingFromBuffer(p_tiling_params,
										p_merc_buffer,
										chunk_envp,
                    zoom,
										tiles_expected,
										p_tiles_generated,
										p_tile_container))
	{
			cout<<"Error: BaseZoomTiling: GMXRunTilingFromBuffer fail"<<endl;
			gmx::CURR_WORK_THREADS--;
      return FALSE;
	}
 
	delete(p_merc_buffer);
  gmx::CURR_WORK_THREADS--;
  return TRUE;
}


namespace gmx
{

extern int MAX_BUFFER_WIDTH = 16;
extern int MAX_WORK_THREADS = 2;
extern int MAX_WARP_THREADS = 1;
extern int CURR_WORK_THREADS = 0;
extern HANDLE WARP_SEMAPHORE = NULL;


int _stdcall gmxPrintNoProgress ( double dfComplete, const char *pszMessage, void * pProgressArg )
{
	return 1;
}


bool RasterFile::Close()
{
  if (p_gdal_ds_) delete(p_gdal_ds_);
  p_gdal_ds_ = NULL;
	
	raster_file_="";
	num_bands_= 0;
	nodata_value_=0;
  nodata_value_defined_=FALSE;
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


bool RasterFile::Init(string raster_file)
{
	Close();
	p_gdal_ds_ = (GDALDataset *) GDALOpen(raster_file.c_str(), GA_ReadOnly );
	
	if (p_gdal_ds_==NULL)
	{
		cout<<"Error: RasterFile::init: can't open raster image"<<endl;
    Close();
		return FALSE;
	}
	
	raster_file_ = raster_file;

	num_bands_	= p_gdal_ds_->GetRasterCount();
  int nodata_value_defined;
	nodata_value_ = p_gdal_ds_->GetRasterBand(1)->GetNoDataValue(&nodata_value_defined);
  this->nodata_value_defined_ = nodata_value_defined;
	gdal_data_type_	= p_gdal_ds_->GetRasterBand(1)->GetRasterDataType();
  
  OGREnvelope raster_srs_envp;
  if (!GetEnvelope(raster_srs_envp))
  {
 		cout<<"Error: RasterFile::init: not valid geotransform in the raster"<<endl;
    Close();
		return FALSE;
  }
		
 	return TRUE;
}


RasterFileCutline*  RasterFile::GetRasterFileCutline(ITileGrid *p_tile_grid, string cutline_file)
{
  if (!p_tile_grid) return 0;
  OGRSpatialReference *p_tiling_srs = p_tile_grid->GetTilingSRS();
  if (!p_tiling_srs) return 0;
  RasterFileCutline* p_rfc = new RasterFileCutline();
  OGREnvelope raster_srs_envp;
  if (!GetEnvelope(raster_srs_envp))
  {
 		cout<<"Error: RasterFile::GetRasterFileCutline: not valid geotransform in the raster"<<endl;
    delete(p_rfc);
		return 0;
  }

  OGRSpatialReference raster_srs;
  if (!GetSRS(raster_srs)) 
    this->GetDefaultSpatialRef(raster_srs,p_tiling_srs);
  
  OGRPolygon* p_poly_envp = VectorOperations::CreateOGRPolygonByOGREnvelope(raster_srs_envp);
  VectorOperations::AddIntermediatePoints(p_poly_envp);
  p_poly_envp->assignSpatialReference(&raster_srs);
 
  if(CE_None!=p_poly_envp->transformTo(p_tiling_srs))
  {
    delete(p_rfc);
    delete (p_poly_envp);
    return 0;
  }

  if (cutline_file!="")
  {
    if (!(p_rfc->tiling_srs_cutline_ = (OGRMultiPolygon*)VectorOperations::ReadAndTransformGeometry(cutline_file,p_tiling_srs)))
    {
      delete(p_rfc);
      delete (p_poly_envp);
      return 0;
    }
  }

  bool rfc_overlaps = (p_rfc->tiling_srs_cutline_) ?
                       p_tile_grid->DoesOverlap180Degree(p_poly_envp) :   
                       ( p_tile_grid->DoesOverlap180Degree(p_poly_envp) || 
                        p_tile_grid->DoesOverlap180Degree(p_rfc->tiling_srs_cutline_));
  if (rfc_overlaps)
  {
    p_tile_grid->AdjustForOverlapping180Degree(p_poly_envp);
    if (p_rfc->tiling_srs_cutline_)
      p_tile_grid->AdjustForOverlapping180Degree(p_rfc->tiling_srs_cutline_);
  }
  
  p_poly_envp->getEnvelope(&p_rfc->tiling_srs_envp_);
  delete (p_poly_envp);    


  if (p_rfc->tiling_srs_cutline_)
  {
    OGRMultiPolygon *p_pixel_line_geom = 0;
    if(!(p_pixel_line_geom = (OGRMultiPolygon*)VectorOperations::ReadAndTransformGeometry(cutline_file,&raster_srs)))
    {
      delete(p_rfc);  
      return 0;
    }
   
    double geotransform[6];
    this->p_gdal_ds_->GetGeoTransform(geotransform);
    double d = geotransform[1]*geotransform[5]-geotransform[2]*geotransform[4];
    if (fabs(d)<1e-7)
    {
      delete(p_rfc);  
      return 0;
    }

    p_rfc->p_pixel_line_cutline_ = new OGRMultiPolygon();

    for (int j=0;j<p_pixel_line_geom->getNumGeometries();j++)
    {
      OGRLinearRing	*p_ogr_ring = 
        ((OGRPolygon*)p_pixel_line_geom->getGeometryRef(j))->getExteriorRing();
	    p_ogr_ring->closeRings();
      for (int i=0;i<p_ogr_ring->getNumPoints();i++)
	    {
        double x = p_ogr_ring->getX(i)	-	geotransform[0];
		    double y = p_ogr_ring->getY(i)	-	geotransform[3];
		
		    double l = (geotransform[1]*y-geotransform[4]*x)/d;
		    double p = (x - l*geotransform[2])/geotransform[1];

		    p_ogr_ring->setPoint(i,(int)(p+0.5),(int)(l+0.5));
	    }
      p_rfc->p_pixel_line_cutline_->addGeometryDirectly(new OGRPolygon());
      ((OGRPolygon*)p_rfc->p_pixel_line_cutline_->getGeometryRef(j))->addRing(p_ogr_ring);
    }
    delete(p_pixel_line_geom);
  }

  return p_rfc;
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
	p_gdal_ds_=NULL;
	raster_file_="";
	num_bands_= 0;
	nodata_value_=0;
  nodata_value_defined_=FALSE;
}


RasterFile::~RasterFile(void)
{
	Close();
}



bool RasterFile::GetPixelSize (int &width, int &height)
{  
	if (this->p_gdal_ds_ == NULL)
  {	
    width=0;
    height=0;
    return false;
  }
	else
  {
    width = this->p_gdal_ds_->GetRasterXSize();
		height = this->p_gdal_ds_->GetRasterYSize();
    return true;
  }
}



bool RasterFile::GetEnvelope(OGREnvelope &envp)
{
  double geotransform[6];
  if (CE_None!=p_gdal_ds_->GetGeoTransform(geotransform)) return false;
  
  envp.MinX = geotransform[0];
	envp.MaxY = geotransform[3];
	envp.MaxX = geotransform[0] + p_gdal_ds_->GetRasterXSize()*geotransform[1];
	envp.MinY = geotransform[3] - p_gdal_ds_->GetRasterYSize()*geotransform[1];

	return true;
};


GDALDataset*	RasterFile::get_gdal_ds_ref()
{
	return this->p_gdal_ds_;
}


bool RasterFile::ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference &srs)
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
	if (OGRERR_NONE!=srs.importFromMICoordSys(strMapinfoProj.c_str())) return FALSE;
	
	return TRUE;
}


bool	RasterFile::GetSRS(OGRSpatialReference  &srs)
{
 const char* strProjRef      = this->p_gdal_ds_->GetProjectionRef();

	if (OGRERR_NONE == srs.SetFromUserInput(strProjRef)) return true;
  else if (FileExists(RemoveExtension(this->raster_file_)+".prj"))
	{
		string prjFile		= RemoveExtension(this->raster_file_)+".prj";
		if (OGRERR_NONE==srs.SetFromUserInput(prjFile.c_str())) return true;	
	}
	else if (FileExists(RemoveExtension(this->raster_file_)+".tab"))
  {
    string tabFile = RemoveExtension(this->raster_file_)+".tab";
    if (ReadSpatialRefFromMapinfoTabFile(tabFile,srs)) return true;
  }
 
  return false;
}

bool	RasterFile::GetDefaultSpatialRef (OGRSpatialReference	&srs, OGRSpatialReference  *p_tiling_srs)
{
  OGREnvelope envp;
  if(!GetEnvelope(envp)) return false;
  if (fabs(envp.MaxX)<=180 && fabs(envp.MaxY)<=90)
	  srs.SetWellKnownGeogCS("WGS84"); 
  else if (!p_tiling_srs) return false;
  else
  {
    char* p_srs_proj4;
    p_tiling_srs->exportToProj4(&p_srs_proj4);
    srs.importFromProj4(p_srs_proj4);
    OGRFree(p_srs_proj4);
	}

	return TRUE;
}


BundleTiler::BundleTiler(void)
{
  p_tile_grid_=0;
}

void BundleTiler::Close(void)
{
	for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	{
		delete((*iter).second);
	}
	item_list_.empty();
  p_tile_grid_=0;
}

BundleTiler::~BundleTiler(void)
{
	Close();
}

bool BundleTiler::AdjustCutlinesForOverlapping180Degree()
{
  OGREnvelope bundle_envp = CalcEnvelope();
  int min_x,min_y,max_x,max_y;
  p_tile_grid_->CalcTileRange(bundle_envp,4,min_x,min_y,max_x,max_y);
  if ((bundle_envp.MaxX>0 && bundle_envp.MinX<0)&&(max_x-min_x>1<<3))
  {
    int min_x0,min_y0,max_x0,max_y0;
    OGREnvelope zero_point_envp;
    zero_point_envp.MinX = -0.001;zero_point_envp.MinY = -0.001;
    zero_point_envp.MaxX = 0.001;zero_point_envp.MaxY = 0.001;
    p_tile_grid_->CalcTileRange(zero_point_envp,5,min_x0,min_y0,max_x0,max_y0);
    for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	  {
      p_tile_grid_->CalcTileRange((*iter).second->tiling_srs_envp_,
                                  5,min_x,min_y,max_x,max_y);
      if (min_x<=max_x0 && max_x>=min_x0) return true;
    }
    
    for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	  {
      OGRPolygon* p_poly_envp = 
        VectorOperations::CreateOGRPolygonByOGREnvelope((*iter).second->tiling_srs_envp_);
      p_tile_grid_->AdjustForOverlapping180Degree(p_poly_envp);
      p_poly_envp->getEnvelope(&(*iter).second->tiling_srs_envp_);
      delete(p_poly_envp);
      if ((*iter).second->tiling_srs_cutline_)
        p_tile_grid_->AdjustForOverlapping180Degree((*iter).second->tiling_srs_cutline_);
    }
  }
  return true;
}


int	BundleTiler::Init (list<string> file_list, ITileGrid* p_tile_grid, string vector_file)
{
	Close();
  
  if (!p_tile_grid) return 0;
  else
  {
    char  *p_srs_proj4;
    if (OGRERR_NONE!=p_tile_grid->GetTilingSRS()->exportToProj4(&p_srs_proj4)) 
      return 0;
    p_tile_grid_=p_tile_grid;
    OGRFree(p_srs_proj4);
  }

	if (file_list.size() == 0) return 0;

 	for (std::list<string>::iterator iter = file_list.begin(); iter!=file_list.end(); iter++)
	{
    if (file_list.size() > 1 && vector_file == "")
      AddItemToBundle((*iter),VectorOperations::GetVectorFileNameByRasterFileName(*iter));
    else AddItemToBundle((*iter),vector_file);
	}

  AdjustCutlinesForOverlapping180Degree();

  return item_list_.size();
}


GDALDataType BundleTiler::GetRasterFileType()
{
  RasterFile rf;
  if (item_list_.size()==0) return GDT_Byte;
  else if (!rf.Init(*GetFileList().begin())) return GDT_Byte;
  else return rf.get_gdal_ds_ref()->GetRasterBand(1)->GetRasterDataType();

}

bool	BundleTiler::AddItemToBundle (string raster_file, string vector_file)
{	
	RasterFile image;

	if (!image.Init(raster_file))
	{
		cout<<"Error: can't init. image: "<<raster_file<<endl;
		return 0;
	}

  pair<string,RasterFileCutline*> p;
	p.first			= raster_file;
  p.second    = image.GetRasterFileCutline(p_tile_grid_,vector_file);
  if (!p.second) return false;
  item_list_.push_back(p);
	return true;
}

list<string>	BundleTiler::GetFileList()
{
	std::list<string> file_list;
	for (std::list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end(); iter++)
		file_list.push_back((*iter).first);

	return file_list;
}

OGREnvelope BundleTiler::CalcEnvelope()
{
	OGREnvelope	envp;
  
	envp.MaxY=(envp.MaxX = -1e+100);envp.MinY=(envp.MinX = 1e+100);
	if (item_list_.size() == 0) return envp;

	for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	{
    if ((*iter).second->tiling_srs_cutline_ != NULL)
    {
      OGREnvelope envp_cutline;
      (*iter).second->tiling_srs_cutline_->getEnvelope(&envp_cutline);
      envp = VectorOperations::CombineOGREnvelopes(envp, 
                                              VectorOperations::InetersectOGREnvelopes(envp_cutline,
                                                                                  (*iter).second->tiling_srs_envp_)
                                              );
    }
    else envp = VectorOperations::CombineOGREnvelopes(envp,((*iter).second->tiling_srs_envp_));
	}
	return envp; 
}


int	BundleTiler::CalcNumberOfTiles (int zoom)
{
	int n = 0;
  double		res = p_tile_grid_->CalcResolutionByZoom(zoom);

	int minx,maxx,miny,maxy;
	p_tile_grid_->CalcTileRange(CalcEnvelope(),zoom,minx,miny,maxx,maxy);
	
	for (int curr_x = minx; curr_x<=maxx; curr_x++)
	{
		for (int curr_y = miny; curr_y<=maxy; curr_y++)
		{
			OGREnvelope tile_envp = p_tile_grid_->CalcEnvelopeByTile(zoom,curr_x,curr_y);
			if (Intersects(tile_envp)) n++;
		}
	}

	return n;
}

double BundleTiler::GetNodataValue(bool &nodata_defined)
{
  if (item_list_.size()==0) return NULL;
  RasterFile rf;
  rf.Init((*item_list_.begin()).first);
  return rf.get_nodata_value(nodata_defined);
}




int		BundleTiler::CalcBestMercZoom()
{
	if (item_list_.size()==0) return -1;

	RasterFile rf;
  rf.Init((*item_list_.begin()).first);
	int width_src = 0, height_src = 0;
	rf.GetPixelSize(width_src,height_src);
	if (width_src<=0 || height_src <= 0) return false;
  OGREnvelope envp = (*item_list_.begin()).second->tiling_srs_envp_;

	double res_src = min((envp.MaxX - envp.MinX)/width_src,(envp.MaxY - envp.MinY)/height_src);
	if (res_src<=0) return -1;

	for (int z=0; z<32; z++)
	{
		if (p_tile_grid_->CalcResolutionByZoom(z) <res_src || 
			(fabs(p_tile_grid_->CalcResolutionByZoom(z)-res_src)/p_tile_grid_->CalcResolutionByZoom(z))<0.2) return z;
	}

	return -1;
}


bool	BundleTiler::Intersects(OGREnvelope envp)
{
	for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	{
		if ((*iter).second->Intersects(envp)) return TRUE;
  }
	
	return FALSE;
}

///*ToDo
bool BundleTiler::CalclValuesForStretchingTo8Bit (double *&p_min_values,
                                                       double *&p_max_values,
                                                       double *p_nodata_val,
                                                       BandMapping    *p_band_mapping)
{
  if (this->item_list_.size()==0) return FALSE;
  p_min_values = (p_max_values = 0);
  int bands_num;
  double min,max,mean,std;
   
  if (!p_band_mapping)
  {
    RasterFile rf;
    if (!rf.Init(*GetFileList().begin())) return FALSE;
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
          if (!rf.Init(*iter)) error=true;
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
  p_warp_multi_params->warp_error_ = (CE_None != p_warp_multi_params->p_warp_operation_->ChunkAndWarpMulti(0,
                                                            0,
                                                            p_warp_multi_params->buf_width_,
                                                            p_warp_multi_params->buf_height_));
  return true;
}

bool BundleTiler::WarpChunkToBuffer (int zoom,	
                                            OGREnvelope	    chunk_envp, 
                                            RasterBuffer    *p_dst_buffer, 
                                            int             output_bands_num,
                                            int             **pp_band_mapping,
                                            GDALResampleAlg resample_alg, 
                                            BYTE            *p_nodata,
                                            BYTE            *p_background_color,
                                            bool            warp_multithread)
{
  //initialize output vrt dataset warp to 
	if (item_list_.size()==0) return FALSE;
	GDALDataset	*p_src_ds = (GDALDataset*)GDALOpen((*item_list_.begin()).first.c_str(),GA_ReadOnly );
	if (p_src_ds==NULL)
	{
		cout<<"Error: can't open raster file: "<<(*item_list_.begin()).first<<endl;
		return FALSE;
	}
  GDALDataType	dt		= GetRasterFileType();
  int       bands_num_src   = p_src_ds->GetRasterCount();
  int				bands_num_dst	= (output_bands_num==0) ? bands_num_src : output_bands_num;
 
  bool nodata_val_from_file_defined;
  double			nodata_val_from_file = GetNodataValue(nodata_val_from_file_defined);

	double		res			=  p_tile_grid_->CalcResolutionByZoom(zoom);
	int				buf_width	= int(((chunk_envp.MaxX - chunk_envp.MinX)/res)+0.5);
	int				buf_height	= int(((chunk_envp.MaxY - chunk_envp.MinY)/res)+0.5);

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
	geotransform[0] = chunk_envp.MinX;
	geotransform[1] = res;
	geotransform[2] = 0;
	geotransform[3] = chunk_envp.MaxY;
	geotransform[4] = 0;
	geotransform[5] = -res;
	p_vrt_ds->SetGeoTransform(geotransform);
	char *p_dst_wkt = NULL;
  p_tile_grid_->GetTilingSRS()->exportToWkt( &p_dst_wkt );
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
  OGRGeometry *p_chunk_geom = VectorOperations::CreateOGRPolygonByOGREnvelope(chunk_envp);
	for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	{
    file_num++;
		
    //check if image envelope Intersects destination buffer envelope
    if (!(*iter).second->tiling_srs_envp_.Intersects(chunk_envp)) continue;
			
    if ((*iter).second->tiling_srs_cutline_ != NULL)
		{
			if (!(*iter).second->tiling_srs_cutline_->Intersects(p_chunk_geom)) continue;
		}

		// Open input raster and create source dataset
    RasterFile	input_rf;
    input_rf.Init((*iter).first);
    p_src_ds = input_rf.get_gdal_ds_ref();
    			
		// Get Source coordinate system and set destination  
		char *p_src_wkt	= NULL;
    OGRSpatialReference input_rf_srs;
    if (!input_rf.GetSRS(input_rf_srs)) 
      input_rf.GetDefaultSpatialRef(input_rf_srs,p_tile_grid_->GetTilingSRS());
    
		input_rf_srs.exportToWkt(&p_src_wkt);
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

    if ((*iter).second->tiling_srs_cutline_)
      p_warp_options->hCutline = (*iter).second->p_pixel_line_cutline_->clone();
  
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
          p_warp_options->padfSrcNoDataImag[i] = nodata_val_from_file;
      }
    }
    
    p_warp_options->pfnProgress = gmxPrintNoProgress;  

		p_warp_options->pTransformerArg = 
				GDALCreateApproxTransformer( GDALGenImgProjTransform, 
											  GDALCreateGenImgProjTransformer(  p_src_ds, 
																				p_src_wkt, 
																				p_vrt_ds,
																				p_dst_wkt,
																				FALSE, 0.0, 1 ),
											 error_threshold );

		p_warp_options->pfnTransformer = GDALApproxTransform;

    p_warp_options->eResampleAlg =  resample_alg; 
    
     // Initialize and execute the warp operation. 
		GDALWarpOperation gdal_warp_operation;
		gdal_warp_operation.Initialize( p_warp_options );
		   
    bool  warp_error = warp_multithread ? this->CalcAsyncWarpMulti(&gdal_warp_operation,buf_width,buf_height) :
                                        (CE_None != gdal_warp_operation.ChunkAndWarpImage( 0,0,buf_width,buf_height));  

    GDALDestroyApproxTransformer(p_warp_options->pTransformerArg );
    if (p_warp_options->hCutline)
    {
      delete((OGRGeometry*)p_warp_options->hCutline);
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
		if (warp_error) 
    {
      cout<<"Error: warping raster block of image: "<<(*iter).first<<endl;
      delete(p_chunk_geom);
      OGRFree(p_dst_wkt);
	    GDALClose(p_vrt_ds);
      return FALSE;
    }
	}
  delete(p_chunk_geom);

	p_dst_buffer->CreateBuffer(bands_num_dst,buf_width,buf_height,NULL,dt,FALSE,p_vrt_ds->GetRasterBand(1)->GetColorTable());


  p_vrt_ds->RasterIO(	GF_Read,0,0,buf_width,buf_height,p_dst_buffer->get_pixel_data_ref(),
						buf_width,buf_height,p_dst_buffer->get_data_type(),
						p_dst_buffer->get_num_bands(),NULL,0,0,0);
  

  OGRFree(p_dst_wkt);
	GDALClose(p_vrt_ds);
	VSIUnlink(tiff_in_mem.c_str());
	return TRUE;
}

bool BundleTiler::CalcAsyncWarpMulti (GDALWarpOperation* p_warp_operation, int width, int height)
{
  bool warp_error = false;
  for (int warp_attempt=0; warp_attempt<2;  warp_attempt++)
  {
    unsigned long thread_id;
      
    GMXAsyncWarpMultiParams warp_multi_params;
    warp_multi_params.buf_height_=height;
    warp_multi_params.buf_width_=width;
    warp_multi_params.p_warp_operation_= p_warp_operation;
    warp_multi_params.warp_error_=false; 
    HANDLE hThread = CreateThread(NULL,0,GMXAsyncWarpMulti,&warp_multi_params,0,&thread_id); 
    DWORD exit_code;
    int iter_num;
    for (iter_num=0;iter_num<120;iter_num++)
    {
      if (GetExitCodeThread(hThread,&exit_code))
      {
        if (exit_code == STILL_ACTIVE) Sleep(250);
        else 
        {
          CloseHandle(hThread);
          return warp_multi_params.warp_error_;
        }
      }
      else Sleep(250);
    }
    TerminateThread(hThread,1);
    CloseHandle(hThread);
  }
  return false;     
}


bool BundleTiler::RunBaseZoomTiling	(	TilingParameters		*p_tiling_params, 
								            ITileContainer			*p_tile_container)
{
  srand(0);
	int	tiles_generated = 0;
	int zoom = (p_tiling_params->base_zoom_ == 0) ? CalcBestMercZoom() : p_tiling_params->base_zoom_;
	double res = p_tile_grid_->CalcResolutionByZoom(zoom);

  cout<<"calculating number of tiles: ";
	int tiles_expected	= CalcNumberOfTiles(zoom);	
	cout<<tiles_expected<<endl;
  if (tiles_expected == 0) return FALSE;
 
  bool		need_stretching = false;
	double		*p_stretch_min_values = NULL, *p_stretch_max_values = NULL;

  extern int MAX_WARP_THREADS;
  MAX_WARP_THREADS	= p_tiling_params->max_warp_threads_ > 0 ? p_tiling_params->max_warp_threads_ : MAX_WARP_THREADS;
  extern int MAX_WORK_THREADS;
  MAX_WORK_THREADS	= p_tiling_params->max_work_threads_ > 0 ? p_tiling_params->max_work_threads_ : MAX_WORK_THREADS;
  extern __int64 TILE_CACHE_MAX_SIZE;
  TILE_CACHE_MAX_SIZE = p_tiling_params->max_cache_size_ > 0 ? p_tiling_params->max_cache_size_  : TILE_CACHE_MAX_SIZE;
  extern HANDLE WARP_SEMAPHORE;
  WARP_SEMAPHORE = CreateSemaphore(NULL,MAX_WARP_THREADS,MAX_WARP_THREADS,NULL);
    
  if (p_tiling_params->auto_stretching_)
	{
    if (  (p_tiling_params->tile_type_ == JPEG_TILE || p_tiling_params->tile_type_ == PNG_TILE) && 
          (GetRasterFileType()!= GDT_Byte)  
       )
		{
			need_stretching = true;
			cout<<"WARNING: input raster doesn't match 8 bit/band. Auto stretching to 8 bit will be performed"<<endl;
      double nodata_val = (p_tiling_params->p_transparent_color_) ?
                           p_tiling_params->p_transparent_color_[0] : 0;
      if (!CalclValuesForStretchingTo8Bit(p_stretch_min_values,
                                                    p_stretch_max_values,
                                                    (p_tiling_params->p_transparent_color_) ?
                                                    &nodata_val : 0,
                                                    p_tiling_params->p_band_mapping_))
			{
        cout<<"Error: can't calculate parameters of auto stretching to 8 bit"<<endl;
			  return FALSE;
      }
    }
	}
  
	cout<<"0% ";
	fflush(stdout);

	int minx,maxx,miny,maxy;
  p_tile_grid_->CalcTileRange(CalcEnvelope(),zoom,minx,miny,maxx,maxy);

	HANDLE			thread_handle = NULL;
	//unsigned long	thread_id;

  //int num_warp = 0;
  
  bool tiling_error = FALSE;
  
  unsigned long thread_id;
  list<pair<HANDLE,void*>> thread_params_list; 
  

  //ToDo shoud refactor this cycle - thread creation and control 
  for (int curr_min_x = minx; curr_min_x<=maxx; curr_min_x+=MAX_BUFFER_WIDTH)
	{
		int curr_max_x =	(curr_min_x + MAX_BUFFER_WIDTH - 1 > maxx) ? 
							maxx : curr_min_x + MAX_BUFFER_WIDTH - 1;
		
		for (int curr_min_y = miny; curr_min_y<=maxy; curr_min_y+=MAX_BUFFER_WIDTH)
		{
			int curr_max_y =	(curr_min_y + MAX_BUFFER_WIDTH - 1 > maxy) ? 
								maxy : curr_min_y + MAX_BUFFER_WIDTH - 1;
			
			OGREnvelope chunk_envp = p_tile_grid_->CalcEnvelopeByTileRange(	zoom,
																					curr_min_x,
																					curr_min_y,
																					curr_max_x,
																					curr_max_y);
			if (!Intersects(chunk_envp)) continue;
	    
      while (CURR_WORK_THREADS >= MAX_WORK_THREADS)        
        Sleep(100);
      
      //
      //CheckStatusAndCloseThreads
      //TerminateThreads
      
      if (!CheckStatusAndCloseThreads(&thread_params_list))
      {
        TerminateThreads(&thread_params_list);
        cout<<"Error: occured in BaseZoomTiling"<<endl;
        return false;

      }

      GMXAsyncChunkTilingParams	*p_chunk_tiling_params = new  GMXAsyncChunkTilingParams();
          
      p_chunk_tiling_params->p_tiling_params_ = p_tiling_params;
      p_chunk_tiling_params->chunk_envp_ = chunk_envp;
      p_chunk_tiling_params->p_bundle_ = this;
      p_chunk_tiling_params->p_tile_container_ = p_tile_container;
      p_chunk_tiling_params->p_tiles_generated_ = &tiles_generated;
      p_chunk_tiling_params->tiles_expected_ = tiles_expected;
      p_chunk_tiling_params->need_stretching_ = need_stretching;
      p_chunk_tiling_params->p_stretch_min_values_ = p_stretch_min_values;
      p_chunk_tiling_params->p_stretch_max_values_ = p_stretch_max_values;
      p_chunk_tiling_params->z_ = zoom;
      HANDLE hThread = CreateThread(NULL,0,GMXAsyncWarpChunkAndMakeTiling,p_chunk_tiling_params,0,&thread_id);      
      if (hThread)
         thread_params_list.push_back(
        pair<HANDLE,GMXAsyncChunkTilingParams*>(hThread,p_chunk_tiling_params));
      Sleep(100);    
    }
	}

  while (CURR_WORK_THREADS > 0)
    Sleep(100);

  if (!CheckStatusAndCloseThreads(&thread_params_list))
  {
        TerminateThreads(&thread_params_list);
        cout<<"Error: occured in BaseZoomTiling"<<endl;
        return false;
  }

  if (WARP_SEMAPHORE)
    CloseHandle(WARP_SEMAPHORE);

  return true;
}


bool BundleTiler::CheckStatusAndCloseThreads(list<pair<HANDLE,void*>>* p_thread_list)
{
  
  for (list<pair<HANDLE,void*>>::iterator iter = 
      p_thread_list->begin();iter!=p_thread_list->end();iter++)
  {
    DWORD exit_code;
    if (GetExitCodeThread((*iter).first,&exit_code))
    {
      if (exit_code != STILL_ACTIVE)
      {
        if (exit_code==1)
        {
          CloseHandle((*iter).first);
          delete((GMXAsyncChunkTilingParams*)(*iter).second);
          p_thread_list->remove(*iter);
          break;
        }
        else return false;
      }
    }
  }
  return true;
}


bool BundleTiler::TerminateThreads(list<pair<HANDLE,void*>>* p_thread_list)
{
  for (list<pair<HANDLE,void*>>::iterator iter = 
      p_thread_list->begin();iter!=p_thread_list->end();iter++)
  {
    TerminateThread((*iter).first,1);
    CloseHandle((*iter).first);
  }

  return true;
}


bool BundleTiler::RunTilingFromBuffer (TilingParameters			*p_tiling_params, 
						                             RasterBuffer					*p_buffer, 
						                             OGREnvelope      buffer_envelope,
						                             int							zoom,
						                             int							tiles_expected, 
						                             int							*p_tiles_generated,
						                             ITileContainer					*p_tile_container)
{  
  int min_x,max_x,min_y,max_y;
  if (!p_tile_grid_->CalcTileRange(buffer_envelope,zoom,min_x,min_y,max_x,max_y))
    return false;
  
  

	for (int x = min_x; x <= max_x; x += 1)
	{
		for (int y = min_y; y <= max_y; y += 1)
		{
			OGREnvelope tile_envelope = p_tile_grid_->CalcEnvelopeByTile(zoom,x,y);
	    if (!Intersects(tile_envelope)) continue;
             
      int x_offset = (int)(((tile_envelope.MinX-buffer_envelope.MinX)/
                            p_tile_grid_->CalcResolutionByZoom(zoom))+0.5);
      int y_offset = (int)(((buffer_envelope.MaxY-tile_envelope.MaxY)/
                            p_tile_grid_->CalcResolutionByZoom(zoom))+0.5);
      int tile_size_x = (int)(((tile_envelope.MaxX-tile_envelope.MinX)/
                          p_tile_grid_->CalcResolutionByZoom(zoom))+0.5);
      int tile_size_y = (int)(((tile_envelope.MaxY-tile_envelope.MinY)/
                          p_tile_grid_->CalcResolutionByZoom(zoom))+0.5);
      RasterBuffer tile_buffer;
			void *p_tile_pixel_data = p_buffer->GetPixelDataBlock(x_offset, y_offset,tile_size_x,tile_size_y);
			tile_buffer.CreateBuffer(	p_buffer->get_num_bands(),
											tile_size_x,
											tile_size_y,
											p_tile_pixel_data,
											p_buffer->get_data_type(),
											FALSE,
											p_buffer->get_color_table_ref());
      delete[]p_tile_pixel_data;
      
      if (p_tiling_params->p_transparent_color_ != NULL  && 
          p_tiling_params->tile_type_ == PNG_TILE)
        tile_buffer.CreateAlphaBandByRGBColor(p_tiling_params->p_transparent_color_, 
                                              p_tiling_params->nodata_tolerance_);
      if (p_tile_container != NULL)
			{
				
				void *p_data=NULL;
				int size = 0;
				switch (p_tiling_params->tile_type_)
				{
					case JPEG_TILE:
						{
              tile_buffer.SaveToJpegData(p_data,size,p_tiling_params->jpeg_quality_);
              break;
						}
					case PNG_TILE:
						{
							tile_buffer.SaveToPngData(p_data,size);
							break;
						}
          case PSEUDO_PNG_TILE:
					{
						tile_buffer.SaveToPseudoPngData(p_data,size);
						break;
					}
          case JP2_TILE:
            {
              tile_buffer.SaveToJP2Data(p_data,size,p_tiling_params->jpeg_quality_);
              break;
            }
					default:
						tile_buffer.SaveToTiffData(p_data,size);
				}
        
        if (!p_tile_container->AddTile(zoom,x,y,(BYTE*)p_data,size))
        {
          if (p_data) delete[]((BYTE*)p_data);
          cout<<"Error: AddTile: writing tile to container"<<endl;
          return FALSE;
        }
        
				delete[]((BYTE*)p_data);
				(*p_tiles_generated)++;
			}
			
			GMXPrintTilingProgress(tiles_expected,(*p_tiles_generated));
		}
	}
	
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