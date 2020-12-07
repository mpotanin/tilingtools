#include "rasterfile.h"
//#include "stringfuncs.h"
//#include "filesystemfuncs.h"


int TTXPrintProgressStub(double dfComplete, const char *pszMessage, void * pProgressArg)
{
	return 1;
}


namespace ttx
{

bool RasterFile::Close()
{
	if (p_gdal_ds_) delete(p_gdal_ds_);
	p_gdal_ds_ = 0;
	
	raster_file_="";
	num_bands_= 0;
	nodata_value_=0;
	nodata_value_defined_=false;
	set_srs_ = "";

	return true;
}


bool RasterFile::SetBackgroundToGDALDataset (GDALDataset *p_ds, unsigned char background[3])
{
  ///
  //p_ds->GetRasterBand(1)->GetHistogram(
  ////

	if (!p_ds) return false;
	if (p_ds->GetRasterCount() == 0 || p_ds->GetRasterXSize() == 0 || p_ds->GetRasterYSize()==0 || 
		p_ds->GetRasterBand(1) == 0 || p_ds->GetRasterBand(1)->GetRasterDataType() != GDT_Byte) return 0;

	RasterBuffer rb;
	if (!rb.CreateBuffer(p_ds->GetRasterCount(),p_ds->GetRasterXSize(),
		p_ds->GetRasterYSize(),0,p_ds->GetRasterBand(1)->GetRasterDataType(),0,0))
    return false;

	rb.InitByRGBColor(background);

	if (!p_ds->RasterIO(GF_Write,0,0,p_ds->GetRasterXSize(),p_ds->GetRasterYSize(),
                      rb.get_pixel_data_ref(),p_ds->GetRasterXSize(),p_ds->GetRasterYSize(),
                      rb.get_data_type(),rb.get_num_bands(),0,0,0,0)) 
                      return false;
  
	return true;
}


bool RasterFile::Init(string raster_file, string set_srs)
{
	Close();
	p_gdal_ds_ = (GDALDataset *) GDALOpen(raster_file.c_str(), GA_ReadOnly );
	

	if (p_gdal_ds_==0)
	{
		cout<<"ERROR: RasterFile::init: can't open raster image"<<endl;
    Close();
		return false;
	}

	if (set_srs != "") set_srs_=set_srs; //p_gdal_ds_->SetProjection(set_srs.c_str());
	
	raster_file_ = raster_file;

	num_bands_	= p_gdal_ds_->GetRasterCount();
	int nodata_value_defined;
	nodata_value_ = p_gdal_ds_->GetRasterBand(1)->GetNoDataValue(&nodata_value_defined);
	this->nodata_value_defined_ = nodata_value_defined;
	gdal_data_type_	= p_gdal_ds_->GetRasterBand(1)->GetRasterDataType();
  
	return true;
}


RasterFileCutline*  RasterFile::GetRasterFileCutline(ITileMatrixSet *p_tile_mset, string cutline_file, double clip_offset)
{
	if (!p_tile_mset) return 0;

	RasterFileCutline* p_rfc = new RasterFileCutline();
    
	if (!p_tile_mset->GetRasterEnvelope(this,p_rfc->tiling_srs_envp_))
	{
		cout<<"ERROR: ITileMatrixSet::GetRasterEnvelope fail: not valid raster file georeference"<<endl;
		delete(p_rfc);
		return 0;
	}

  
	if (cutline_file != "")
	{
		OGRSpatialReference *p_tiling_srs = p_tile_mset->GetTilingSRSRef();
		if (!p_tiling_srs) return 0;

		if (!(p_rfc->tiling_srs_cutline_ = 
				(OGRMultiPolygon*)VectorOperations::ReadIntoSingleMultiPolygon(cutline_file, p_tiling_srs)))
			cout << "ERROR: unable to read geometry from vector: " << cutline_file<<endl;
		else
		{
			if (clip_offset > 1e-10)
			{
				OGRMultiPolygon *p_geom_clip = (OGRMultiPolygon*)p_rfc->tiling_srs_cutline_->Buffer(-clip_offset);
				delete(p_rfc->tiling_srs_cutline_);
				p_rfc->tiling_srs_cutline_ = p_geom_clip;
			}

			if (p_tile_mset->DoesOverlap180Degree(p_rfc->tiling_srs_cutline_))
				p_tile_mset->AdjustForOverlapping180Degree(p_rfc->tiling_srs_cutline_);

			OGRSpatialReference raster_srs;
			if (this->GetSRS(raster_srs)) //ToDo - account for defaultsrs
			{
				OGRMultiPolygon* p_raster_srs_geom = (OGRMultiPolygon*)p_rfc->tiling_srs_cutline_->clone();
				p_raster_srs_geom->assignSpatialReference(p_tiling_srs);
				if (OGRERR_NONE != p_raster_srs_geom->transformTo(&raster_srs))
				{
					cout << "ERROR: unable transform vector to raster SRS: " << cutline_file << endl;
					delete(p_raster_srs_geom);
					p_raster_srs_geom = 0;
				}
				else
				{
					double geotransform[6];
					this->p_gdal_ds_->GetGeoTransform(geotransform);
					p_rfc->p_pixel_line_cutline_ = VectorOperations::ConvertFromSRSToPixelLine(p_raster_srs_geom,geotransform);
					delete(p_raster_srs_geom);
				}
			}
		}
	}

	return p_rfc;
}




bool	RasterFile::CalcBandStatistics(int band_num, double &min, double &max, double &mean, double &std,  double *p_nodata_val)
{
	if (this->p_gdal_ds_ == 0) return false;
	if (this->p_gdal_ds_->GetRasterCount()<band_num) return false;
	if (p_nodata_val) this->p_gdal_ds_->GetRasterBand(band_num)->SetNoDataValue(*p_nodata_val);
	return (CE_None==p_gdal_ds_->GetRasterBand(band_num)->ComputeStatistics(0,&min,&max,&mean,&std,0,0));
}


double RasterFile::get_nodata_value (bool &nodata_defined)
{
	nodata_defined = nodata_value_defined_;
	return nodata_value_;
}


RasterFile::RasterFile()
{
	p_gdal_ds_=0;
	raster_file_="";
	num_bands_= 0;
	nodata_value_=0;
	nodata_value_defined_=false;
}


RasterFile::~RasterFile(void)
{
	Close();
}



bool RasterFile::GetPixelSize (int &width, int &height)
{  
	if (this->p_gdal_ds_ == 0)
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




GDALDataset*	RasterFile::get_gdal_ds_ref()
{
	return this->p_gdal_ds_;
}


bool RasterFile::ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference &srs)
{
	FILE *fp = MPLFileSys::OpenFile(tab_file,"r");
	if (!fp) return false;
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *pchar_tab_file_data = new char[size];
	fread(pchar_tab_file_data,sizeof(char),size,fp);
	string str_tab_file_data(pchar_tab_file_data);
	delete[]pchar_tab_file_data;
	if (str_tab_file_data.find("CoordSys Earth Projection") == string::npos) return false;
	int start_pos	= str_tab_file_data.find("CoordSys Earth Projection");
	int end_pos		= (str_tab_file_data.find('\n',start_pos) != string::npos)		? 
														str_tab_file_data.size() : 
														str_tab_file_data.find('\n',start_pos);
	string strMapinfoProj = str_tab_file_data.substr(start_pos, end_pos-start_pos+1);
	if (OGRERR_NONE!=srs.importFromMICoordSys(strMapinfoProj.c_str())) return false;
	
	return true;
}

int RasterFile::GetBandsCount()
{
	return p_gdal_ds_->GetRasterCount();
}

GDALDataType RasterFile::GetDataType()
{
	return p_gdal_ds_->GetRasterBand(1)->GetRasterDataType();
}

bool RasterFile::GetGeoTransform(double* padblGeoTransform)
{
	if (!p_gdal_ds_) return 0;
	return (OGRERR_NONE == p_gdal_ds_->GetGeoTransform(padblGeoTransform));
}

bool	RasterFile::GetSRS(OGRSpatialReference  &srs, ITileMatrixSet* p_tile_mset)
{
	if (set_srs_ != "")
    if (OGRERR_NONE == srs.SetFromUserInput(set_srs_.c_str())) return true;
  
	const char* strProjRef      = this->p_gdal_ds_->GetProjectionRef();

	if (OGRERR_NONE == srs.SetFromUserInput(strProjRef)) return true;
	else if (MPLFileSys::FileExists(MPLFileSys::RemoveExtension(this->raster_file_)+".prj"))
	{
		string prjFile		= MPLFileSys::RemoveExtension(this->raster_file_)+".prj";
		if (OGRERR_NONE==srs.SetFromUserInput(prjFile.c_str())) return true;	
	}
	else if (MPLFileSys::FileExists(MPLFileSys::RemoveExtension(this->raster_file_)+".tab"))
	{
		string tabFile = MPLFileSys::RemoveExtension(this->raster_file_)+".tab";
		if (ReadSpatialRefFromMapinfoTabFile(tabFile,srs)) return true;
	}
  
	if (p_tile_mset) return GetDefaultSpatialRef(srs,p_tile_mset->GetTilingSRSRef());
  

	return false;
}

bool	RasterFile::GetDefaultSpatialRef (OGRSpatialReference	&srs, OGRSpatialReference  *p_tiling_srs)
{

	OGREnvelope envp;
	double geotransform[6];
	if (!p_gdal_ds_) return false;

	if (CE_None!=p_gdal_ds_->GetGeoTransform(geotransform)) return false;
	int w,h;
	this->GetPixelSize(w,h);
	if ((fabs(geotransform[0])<200) && (fabs(geotransform[0] + w*geotransform[1])<200))
		srs.SetWellKnownGeogCS("WGS84"); 
	else if (!p_tiling_srs) return false;
	else
	{
		char* p_srs_proj4;
		p_tiling_srs->exportToProj4(&p_srs_proj4);
		srs.importFromProj4(p_srs_proj4);
		OGRFree(p_srs_proj4);
	}

	return true;
}


int BundleTiler::CallRunChunk( BundleTiler* p_bundle,
                                ttx::TilingParameters* p_tiling_params,
                                ttx::ITileContainer* p_tile_container,
                                int zoom,
                                OGREnvelope chunk_envp,
                                int tiles_expected,
                                int* p_tiles_generated,
                                bool is_scalinig_needed,
                                double* p_scale_values,
                                double* p_offset_values
                              )
{
	return p_bundle->RunChunk(p_tiling_params,p_tile_container,zoom,chunk_envp,tiles_expected,
		p_tiles_generated,is_scalinig_needed,p_scale_values,p_offset_values);
}


int BundleTiler::RunChunk (ttx::TilingParameters* p_tiling_params,
                           ttx::ITileContainer* p_tile_container,
                           int zoom,
                           OGREnvelope chunk_envp,
                           int tiles_expected,
                           int* p_tiles_generated,
                           bool is_scalinig_needed,
                           double* p_scale_values,
                           double* p_offset_values
                          )
{
 
	ttx::RasterBuffer *p_merc_buffer = new ttx::RasterBuffer();

	//ToDo...
	int bands_num=p_tiling_params->p_bundle_input_->GetBandsNum();
	map<string,int*> band_mapping = p_tiling_params->p_bundle_input_->GetBandMapping();

	int* p_ndval = 0;
	unsigned char* p_background_color = 0;

	if (p_tiling_params->p_background_color_ || p_tiling_params->p_nd_rgbcolors_)
	{
		p_background_color = new unsigned char[3];
		memcpy(p_background_color, 
			(p_tiling_params->p_background_color_ ? p_tiling_params->p_background_color_ : p_tiling_params->p_nd_rgbcolors_[0]),
			3);
	}

	if (p_tiling_params->nd_num_)
	{
		p_ndval = new int;
		p_ndval[0] = p_tiling_params->p_nd_rgbcolors_[0][0];
	}

  

	bool warp_result = WarpChunkToBuffer(zoom,
										chunk_envp,
										p_merc_buffer,
										bands_num,
										bands_num == 0 ? 0 : &band_mapping,
										p_tiling_params->gdal_resampling_,
										p_ndval,
										p_background_color);

	delete(p_ndval);
	delete(p_background_color);
    
	if (!warp_result)	
	{
		cout<<"ERROR: BaseZoomTiling: warping to merc fail"<<endl;
		return 1;
	}

	if (is_scalinig_needed)
	{
		if (! p_merc_buffer->ScaleDataTo8Bit (p_scale_values,p_offset_values))
		{
			cout<<"ERROR: can't stretch raster values to 8 bit"<<endl;
    		return 1;
		}
	}
  
	if (!RunTilingFromBuffer(p_tiling_params,
							p_merc_buffer,
							chunk_envp,
							zoom,
							tiles_expected,
							p_tiles_generated,
							p_tile_container))
	{
		cout<<"ERROR: BaseZoomTiling: GMXRunTilingFromBuffer fail"<<endl;
		return 1;
	}
	delete(p_merc_buffer);
	return 0;
}


BundleTiler::BundleTiler(void)
{
	m_bUseWarpClipHack = false;
	p_tile_mset_=0;
}

void BundleTiler::Close(void)
{
	for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	{
		delete((*iter).second);
	}
	item_list_.empty();
	p_tile_mset_=0;
}

BundleTiler::~BundleTiler(void)
{
	Close();
}

bool BundleTiler::AdjustCutlinesForOverlapping180Degree()
{
	OGREnvelope bundle_envp = CalcEnvelope();
	int min_x,min_y,max_x,max_y;
	p_tile_mset_->CalcTileRange(bundle_envp,4,min_x,min_y,max_x,max_y);
  

	if ((bundle_envp.MaxX>0 && bundle_envp.MinX<0)&&(max_x-min_x>1<<3))
	{
		int min_x0,min_y0,max_x0,max_y0;
		OGREnvelope zero_point_envp;
		zero_point_envp.MinX = -0.001;zero_point_envp.MinY = -0.001;
		zero_point_envp.MaxX = 0.001;zero_point_envp.MaxY = 0.001;
		p_tile_mset_->CalcTileRange(zero_point_envp,5,min_x0,min_y0,max_x0,max_y0);
		for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
		{
			p_tile_mset_->CalcTileRange((*iter).second->tiling_srs_envp_,
									5,min_x,min_y,max_x,max_y);
			if (min_x<=max_x0 && max_x>=min_x0) return true;
		}
    
		for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
		{
			OGRPolygon* p_poly_envp = 
			VectorOperations::CreateOGRPolygonByOGREnvelope((*iter).second->tiling_srs_envp_);
			p_tile_mset_->AdjustForOverlapping180Degree(p_poly_envp);
			p_poly_envp->getEnvelope(&(*iter).second->tiling_srs_envp_);
			delete(p_poly_envp);
			if ((*iter).second->tiling_srs_cutline_)
			p_tile_mset_->AdjustForOverlapping180Degree((*iter).second->tiling_srs_cutline_);
		}
	}

	return true;
}


int	BundleTiler::Init ( list<pair<string,string>> raster_vector, 
                        ITileMatrixSet* p_tile_mset,
                        string user_input_srs,
                        double clip_offset)
{
	Close();
  
	if (!p_tile_mset) return 0;
	else
	{
		char  *p_srs_proj4;
		if (OGRERR_NONE!=p_tile_mset->GetTilingSRSRef()->exportToProj4(&p_srs_proj4)) 
		  return 0;
		p_tile_mset_=p_tile_mset;
		OGRFree(p_srs_proj4);
	}

	if (raster_vector.size() == 0) return 0;

	clip_offset_ = clip_offset;
	set_srs_ = user_input_srs;

 	for (auto iter : raster_vector)
	{
		if (iter.second=="")
			AddItemToBundle(iter.first,
                      VectorOperations::GetVectorFileNameByRasterFileName(iter.first));
		else AddItemToBundle(iter.first,iter.second);
	}

	AdjustCutlinesForOverlapping180Degree();

	OGREnvelope oBundleEnvp = CalcEnvelope();
	if ((oBundleEnvp.MaxX > -p_tile_mset_->ULX()) && 
    (raster_vector.size()>1) && (raster_vector.begin()->second!="") &&
    (raster_vector.begin()->second == (raster_vector.begin()++)->second))
	{
		m_bUseWarpClipHack = true;
	}
  
	return item_list_.size();
}


GDALDataType BundleTiler::GetRasterFileType()
{
	RasterFile rf;
	rf.Init(*GetFileList().begin());
	return rf.GetDataType();
	/*
	  if (item_list_.size()==0) return GDT_Byte;
	  else if (!rf.Init(*GetFileList().begin())) return GDT_Byte;
	  else return rf.get_gdal_ds_ref()->GetRasterBand(1)->GetRasterDataType();
	*/

}

bool	BundleTiler::AddItemToBundle (string raster_file, string vector_file)
{	
	RasterFile image;

	if (!image.Init(raster_file, set_srs_))
	{
		cout<<"ERROR: can't init. image: "<<raster_file<<endl;
		return 0;
	}

	pair<string,RasterFileCutline*> p;
	p.first			= raster_file;
	p.second = image.GetRasterFileCutline(p_tile_mset_, vector_file, clip_offset_);
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
		if ((*iter).second->tiling_srs_cutline_ != 0)
		{
		  OGREnvelope envp_cutline;
		  (*iter).second->tiling_srs_cutline_->getEnvelope(&envp_cutline);
		  envp = VectorOperations::MergeEnvelopes(envp, 
												  VectorOperations::InetersectEnvelopes(envp_cutline,
																					  (*iter).second->tiling_srs_envp_)
												  );
		}
		else envp = VectorOperations::MergeEnvelopes(envp,(*iter).second->tiling_srs_envp_);
	}
	return envp; 
}


int	BundleTiler::CalcNumberOfTiles (int zoom)
{
	int n = 0;
	double		res = p_tile_mset_->CalcPixelSizeByZoom(zoom);

	int minx,maxx,miny,maxy;
	p_tile_mset_->CalcTileRange(CalcEnvelope(),zoom,minx,miny,maxx,maxy);
	
	for (int curr_x = minx; curr_x<=maxx; curr_x++)
	{
		for (int curr_y = miny; curr_y<=maxy; curr_y++)
		{
			OGREnvelope tile_envp = p_tile_mset_->CalcEnvelopeByTile(zoom,curr_x,curr_y);
			if (Intersects(tile_envp)) n++;
		}
	}

	return n;
}

double BundleTiler::GetNodataValue(bool &nodata_defined)
{
	if (item_list_.size()==0) return 0;
	RasterFile rf;
	rf.Init((*item_list_.begin()).first);
	return rf.get_nodata_value(nodata_defined);
}




int		BundleTiler::CalcAppropriateZoom()
{
	if (item_list_.size()==0) return -1;

	RasterFile rf;
	rf.Init((*item_list_.begin()).first, set_srs_);
	OGREnvelope envp;

	if (!p_tile_mset_->GetRasterEnvelope(&rf, envp))
	{
	cout << "ERROR: BundleTiler::CalcAppropriateZoom: not valid raster file georeference: ";
	cout<<(*item_list_.begin()).first<<endl;
	return -1;
	}

	int width_src = 0, height_src = 0;
	rf.GetPixelSize(width_src,height_src);
	if (width_src<=0 || height_src <= 0) return false;
	double res_def = min((envp.MaxX - envp.MinX)/width_src,(envp.MaxY - envp.MinY)/height_src);
	if (res_def<=0) return -1;

	for (int z=0; z<32; z++)
		if (((p_tile_mset_->CalcPixelSizeByZoom(z)-res_def)/p_tile_mset_->CalcPixelSizeByZoom(z))<0.2) return z;

	return -1;
}


bool	BundleTiler::Intersects(OGREnvelope envp)
{
	for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	{
		if ((*iter).second->Intersects(envp)) return true;
  }
	
	return false;
}

///*ToDo!!!!!!!
bool BundleTiler::CalclScalingTo8BitParams (double* &p_scales,
                                                  double* &p_offsets,
                                                  int* p_nodata_val,
                                                  int      output_bands_num,
                                                  map<string,int*>*  p_band_mapping)
{
  if (this->item_list_.size()==0) return false;
  p_scales = (p_offsets = 0);
  double min,max,mean,std;
  double nodata_val = (p_nodata_val) ? (*p_nodata_val) : 0;
   
  if (output_bands_num==0)
  {
    RasterFile rf;
    if (!rf.Init(*GetFileList().begin())) return false;
    int bands_num = rf.get_gdal_ds_ref()->GetRasterCount();
    p_scales = new double[bands_num];
    p_offsets = new double[bands_num]; 


    for (int b=0;b<bands_num;b++)
    {
      if (!rf.CalcBandStatistics(b + 1, min, max, mean, std, (p_nodata_val) ? &nodata_val : 0))
      {
        delete[]p_scales;delete[]p_offsets;
        p_scales = (p_offsets = 0);
        return false;
      }

      if (((min>=0) && (max <= 255)) || (max==min))
      {
        p_scales[b] = 1.;
        p_offsets[b] = 0.;
      }
      else
      {
        p_scales[b] = 255. / (4 * std);
        p_offsets[b] = -255.*(mean-(2*std)) / (4 * std);
      }
    
    }
  }
  else
  {
    p_scales = new double[output_bands_num];
    p_offsets = new double[output_bands_num]; 
    
    for (int b=0;b<output_bands_num;b++)
      p_scales[b]=(p_offsets[b]=0);

    for (int b=0;b<output_bands_num;b++)
    {
      for (map<string,int*>::iterator iter=p_band_mapping->begin();iter!=p_band_mapping->end();iter++)
      {
        if ((*iter).second[b]>0)
        {
          RasterFile rf;
          bool error= rf.Init((*iter).first) ? false : true;
          if (!error)
            error = rf.CalcBandStatistics((*iter).second[b],min,max,mean,std,&nodata_val) ? false : true;
          if (error)
          {
            delete[]p_scales; delete[]p_offsets;
            p_scales = (p_offsets = 0);
            return false;
          }
          else
          {
            if (((min >= 0) && (max <= 255)) || (max == min))
            {
              p_scales[b] = 1.;
              p_offsets[b] = 0.;
            }
            else
            {
              p_scales[b] = 255. / (4*std);
              p_offsets[b] = -255.*(mean - (2 * std)) / (4 * std);
            }
          }
        }  
      }
    }
  }
  
  return true;
}
//*/


/*//legacy version

bool BundleTiler::CalclScalingTo8BitParams (double *&p_min_values,
double *&p_max_values,
double *p_nodata_val,
int      output_bands_num,
map<string,int*>*  p_band_mapping)
{
if (this->item_list_.size()==0) return false;
p_min_values = (p_max_values = 0);
double min,max,mean,std;

if (output_bands_num==0)
{
RasterFile rf;
if (!rf.Init(*GetFileList().begin())) return false;
int bands_num = rf.get_gdal_ds_ref()->GetRasterCount();
p_min_values = new double[bands_num];
p_max_values = new double[bands_num];
for (int b=0;b<bands_num;b++)
{
if (!rf.CalcBandStatistics(b+1,min,max,mean,std,p_nodata_val))
{
delete[]p_min_values;delete[]p_max_values;
p_min_values=0;p_max_values=0;
return false;
}
p_min_values[b] = mean - 2*std;
p_max_values[b] = mean + 2*std;
}
}
else
{
p_min_values = new double[output_bands_num];
p_max_values = new double[output_bands_num];

for (int b=0;b<output_bands_num;b++)
p_min_values[b]=(p_max_values[b]=0);


for (int b=0;b<output_bands_num;b++)
{
for (map<string,int*>::iterator iter=p_band_mapping->begin();iter!=p_band_mapping->end();iter++)
{
if ((*iter).second[b]>0)
{
RasterFile rf;
bool error= rf.Init((*iter).first) ? false : true;
if (!error)
error = rf.CalcBandStatistics((*iter).second[b],min,max,mean,std,p_nodata_val) ? false : true;
if (error)
{
delete[]p_min_values;delete[]p_max_values;
return false;
}
else
{
p_min_values[b] = mean - 2*std;
p_max_values[b] = mean + 2*std;
break;
}
}
}
}
}

return true;
}
*/


bool BundleTiler::WarpChunkToBuffer (int zoom,	
                                    OGREnvelope chunk_envp, 
                                    RasterBuffer* p_dst_buffer, 
                                    int output_bands_num,
                                    map<string,int*>* p_band_mapping,
                                    GDALResampleAlg resample_alg, 
                                    int* p_ndval,
                                    unsigned char* p_background_color)
{
  //initialize output vrt dataset warp to 
	if (item_list_.size()==0) return false;
  
	GDALDataset	*p_src_ds = (GDALDataset*)GDALOpen((*item_list_.begin()).first.c_str(),GA_ReadOnly );
	if (p_src_ds==0)
	{
		cout<<"ERROR: can't open raster file: "<<(*item_list_.begin()).first<<endl;
		return false;
	}
	GDALDataType dt	= GetRasterFileType();
	int bands_num_src = p_src_ds->GetRasterCount();
	int	bands_num_dst = (output_bands_num==0) ? bands_num_src : output_bands_num;
 
	bool nodata_val_from_file_defined;
	double nodata_val_from_file = GetNodataValue(nodata_val_from_file_defined);

	double res = p_tile_mset_->CalcPixelSizeByZoom(zoom);
	int	buf_width = int(((chunk_envp.MaxX - chunk_envp.MinX)/res)+0.5);
	int	buf_height = int(((chunk_envp.MaxY - chunk_envp.MinY)/res)+0.5);

	
	string tiff_in_mem = ("/vsimem/tiffinmem_" + 
						MPLString::ConvertIntToString((int)chunk_envp.MinX) + 
						"_" + MPLString::ConvertIntToString((int)chunk_envp.MaxY));
  
	GDALDataset* p_vrt_ds = (GDALDataset*)GDALCreate(
													GDALGetDriverByName("GTiff"),
													tiff_in_mem.c_str(),
													buf_width,
													buf_height,
													bands_num_dst,
													dt,
													0
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
	char *p_dst_wkt = 0;
	p_tile_mset_->GetTilingSRSRef()->exportToWkt( &p_dst_wkt );
	p_vrt_ds->SetProjection(p_dst_wkt);
  
	if (p_background_color)
		RasterFile::SetBackgroundToGDALDataset(p_vrt_ds,p_background_color);
  
   
  int file_num = -1;
  OGRGeometry *p_chunk_geom = VectorOperations::CreateOGRPolygonByOGREnvelope(chunk_envp);
	for (list<pair<string,RasterFileCutline*>>::iterator iter = item_list_.begin(); iter!=item_list_.end();iter++)
	{
    file_num++;
		
    //check if image envelope Intersects destination buffer envelope
    if (!(*iter).second->tiling_srs_envp_.Intersects(chunk_envp)) continue;
			
    if ((*iter).second->tiling_srs_cutline_ != 0)
		{
			if (!(*iter).second->tiling_srs_cutline_->Intersects(p_chunk_geom)) continue;
		}

		// Open input raster and create source dataset
    RasterFile	input_rf;
    input_rf.Init((*iter).first,set_srs_);
    p_src_ds = input_rf.get_gdal_ds_ref();
    			
		// Get Source coordinate system and set destination  
		char *p_src_wkt	= 0;
    OGRSpatialReference input_rf_srs;
    bool input_srs_defined = input_rf.GetSRS(input_rf_srs,this->p_tile_mset_) ? true : false;
    if (input_srs_defined)
    {
      input_rf_srs.exportToWkt(&p_src_wkt);
      CPLAssert(p_src_wkt != 0 && strlen(p_src_wkt) > 0);
    }
    else p_src_wkt = 0;   
  
    GDALWarpOptions *p_warp_options = GDALCreateWarpOptions();
    p_warp_options->papszWarpOptions = 0;
  
   	p_warp_options->hSrcDS = p_src_ds;
		p_warp_options->hDstDS = p_vrt_ds;

    p_warp_options->dfWarpMemoryLimit = 250000000; 
    
    double			error_threshold = 0.125;
    
    p_warp_options->panSrcBands = new int[bands_num_dst];
    p_warp_options->panDstBands = new int[bands_num_dst];
		if (p_band_mapping)
    {
      int warp_bands_num =0;
      for (int i=0; i<bands_num_dst; i++)
      {
        if ((*p_band_mapping)[(*iter).first][i]>0)
        {
          p_warp_options->panSrcBands[warp_bands_num] = (*p_band_mapping)[(*iter).first][i];
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
    
    //debug
    //p_warp_options->hCutline = 0;
    if (m_bUseWarpClipHack)
      p_warp_options->hCutline = 0;
    else
      p_warp_options->hCutline = (*iter).second->p_pixel_line_cutline_ ?
                               (*iter).second->p_pixel_line_cutline_->clone() : 0;
    //end-debug

    if (p_ndval || nodata_val_from_file_defined)
    {
		p_warp_options->padfSrcNoDataReal = new double[bands_num_dst];
		p_warp_options->padfSrcNoDataImag = new double[bands_num_dst];
		p_warp_options->padfDstNoDataImag= new double[bands_num_dst];
		p_warp_options->padfDstNoDataReal = new double[bands_num_dst];

		for (int i=0;i<bands_num_dst;i++)
		{
			p_warp_options->padfSrcNoDataReal[i] = nodata_val_from_file_defined ? nodata_val_from_file : (*p_ndval);
			p_warp_options->padfSrcNoDataImag[i] = nodata_val_from_file_defined ? nodata_val_from_file : 0;
			p_warp_options->padfDstNoDataImag[i] = nodata_val_from_file_defined ? nodata_val_from_file : 0;
			p_warp_options->padfDstNoDataReal[i] = nodata_val_from_file_defined ? nodata_val_from_file : (*p_ndval);
		}
    }
        
    p_warp_options->pfnProgress = TTXPrintProgressStub;
    //p_warp_options->pfnProgress = 0;

    p_warp_options->pTransformerArg = p_src_wkt ?
				GDALCreateApproxTransformer( GDALGenImgProjTransform, 
											               GDALCreateGenImgProjTransformer(  p_src_ds, 
																				              p_src_wkt, 
																				              p_vrt_ds,
																				              p_dst_wkt,
																				              false, 0.0, 1),
                                     error_threshold ) :
        GDALCreateApproxTransformer(GDALGenImgProjTransform,
                                     GDALCreateGenImgProjTransformer(p_src_ds,
                                     0,
                                     p_vrt_ds,
                                     p_dst_wkt,
                                     true, 0.0, 1),
                                     error_threshold);

		p_warp_options->pfnTransformer = GDALApproxTransform;
    p_warp_options->eResampleAlg =  resample_alg; 
    
     // Initialize and execute the warp operation. 
		GDALWarpOperation gdal_warp_operation;
		gdal_warp_operation.Initialize( p_warp_options );
		   
    bool  warp_error=(CE_None!=gdal_warp_operation.ChunkAndWarpImage( 0,0,buf_width,buf_height));  

    GDALDestroyApproxTransformer(p_warp_options->pTransformerArg );
    if (p_warp_options->hCutline)
    {
      delete((OGRGeometry*)p_warp_options->hCutline);
      p_warp_options->hCutline = 0;
    }

    if (p_warp_options->padfSrcNoDataReal)
    {
		delete[]p_warp_options->padfSrcNoDataReal;
		delete[]p_warp_options->padfSrcNoDataImag;
		delete[]p_warp_options->padfDstNoDataReal;
		delete[]p_warp_options->padfDstNoDataImag;
      
		p_warp_options->padfSrcNoDataReal = 0;
		p_warp_options->padfSrcNoDataImag = 0;
		p_warp_options->padfDstNoDataReal = 0;
		p_warp_options->padfDstNoDataImag = 0;
    }

    delete[]p_warp_options->panSrcBands;
    delete[]p_warp_options->panDstBands;
    p_warp_options->panSrcBands = 0;
    p_warp_options->panDstBands = 0;
   
		GDALDestroyWarpOptions( p_warp_options );
		OGRFree(p_src_wkt);
		input_rf.Close();
		if (warp_error) 
    {
      cout<<"ERROR: warping raster block of image: "<<(*iter).first<<endl;
      delete(p_chunk_geom);
      OGRFree(p_dst_wkt);
	    GDALClose(p_vrt_ds);
      return false;
    }
	}
  delete(p_chunk_geom);

	p_dst_buffer->CreateBuffer(bands_num_dst,buf_width,buf_height,0,dt,false,p_vrt_ds->GetRasterBand(1)->GetColorTable());

  p_vrt_ds->RasterIO(	GF_Read,0,0,buf_width,buf_height,p_dst_buffer->get_pixel_data_ref(),
						buf_width,buf_height,p_dst_buffer->get_data_type(),
						p_dst_buffer->get_num_bands(),0,0,0,0);

  OGRFree(p_dst_wkt);
	GDALClose(p_vrt_ds);
	VSIUnlink(tiff_in_mem.c_str());
	return true;
}

bool BundleTiler::RunBaseZoomTiling	(	TilingParameters		*p_tiling_params, 
								        ITileContainer			*p_tile_container)
{
	srand(0);
	int	tiles_generated = 0;
	int zoom = (p_tiling_params->base_zoom_ == 0) ? CalcAppropriateZoom() : p_tiling_params->base_zoom_;
	double res = p_tile_mset_->CalcPixelSizeByZoom(zoom);

	cout<<"calculating number of tiles: ";
	int tiles_expected	= CalcNumberOfTiles(zoom);	
	cout<<tiles_expected<<endl;
	if (tiles_expected == 0) return false;
 
	bool		is_scaling_needed = false;
	double		*p_scale_values = 0, *p_offset_values = 0;

	const int MAX_WORK_THREADS = p_tiling_params->max_work_threads_ > 0 ? p_tiling_params->max_work_threads_ : 2;
	const int TILES_CHUNK_WIDTH = p_tiling_params->tile_chunk_size_ != 0 ? p_tiling_params->tile_chunk_size_ :
                                                                       (p_tiling_params->max_work_threads_ > 1) ? 8 : 16;
    
	if (  (p_tiling_params->tile_type_ == JPEG_TILE || p_tiling_params->tile_type_ == PNG_TILE) && 
          (GetRasterFileType()!= GDT_Byte)  )
	{
		is_scaling_needed = true;
		cout<<"WARNING: input raster doesn't match 8 bit/band. Auto scaling to 8 bit will be performed"<<endl;

		int nodata_val = (p_tiling_params->nd_num_) ? p_tiling_params->p_nd_rgbcolors_[0][0] : 0;
		map<string, int*> band_mapping = p_tiling_params->p_bundle_input_->GetBandMapping();
		if (!CalclScalingTo8BitParams(p_scale_values,
                                      p_offset_values,
                                      (p_tiling_params->nd_num_) ?
                                       &nodata_val : 0,
										p_tiling_params->p_bundle_input_->GetBandsNum(),
										p_tiling_params->p_bundle_input_->GetBandsNum() ?
										&band_mapping : 0))
		{
			cout<<"ERROR: can't calculate parameters of auto scaling to 8 bit"<<endl;
			return false;
		}
	}
  
	cout<<"0% ";
	fflush(stdout);

	int minx,maxx,miny,maxy;
	p_tile_mset_->CalcTileRange(CalcEnvelope(),zoom,minx,miny,maxx,maxy);

	list<future<int>> tiling_threads;
  
	//ToDo shoud refactor this cycle - thread creation and control 
	for (int curr_min_x = minx; curr_min_x<=maxx; curr_min_x+=TILES_CHUNK_WIDTH)
	{
		int curr_max_x =	(curr_min_x + TILES_CHUNK_WIDTH - 1 > maxx) ? 
							maxx : curr_min_x + TILES_CHUNK_WIDTH - 1;
		
		for (int curr_min_y = miny; curr_min_y<=maxy; curr_min_y+=TILES_CHUNK_WIDTH)
		{
			int curr_max_y =	(curr_min_y + TILES_CHUNK_WIDTH - 1 > maxy) ? 
								maxy : curr_min_y + TILES_CHUNK_WIDTH - 1;
			
			OGREnvelope chunk_envp = p_tile_mset_->CalcEnvelopeByTileRange(	zoom,
																					curr_min_x,
																					curr_min_y,
																					curr_max_x,
																					curr_max_y);
			if (!Intersects(chunk_envp)) continue;
      
			if (tiling_threads.size() >= MAX_WORK_THREADS)
			{
				if (!WaitForTilingThreads(&tiling_threads,MAX_WORK_THREADS))
				{
					TerminateTilingThreads(tiling_threads);
					cout << "ERROR: occured in BaseZoomTiling" << endl;
					return false;
				}
			}

			tiling_threads.push_back(
				std::async(GMXThreading::GetLaunchPolicy(),
							BundleTiler::CallRunChunk,
							this,
							p_tiling_params,
							p_tile_container,
							zoom,
							chunk_envp,
							tiles_expected,
							&tiles_generated,
							is_scaling_needed,
							p_scale_values,
							p_offset_values
							));
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	if (!WaitForTilingThreads(&tiling_threads, 1))
	{
		TerminateTilingThreads(tiling_threads);
		cout << "ERROR: occured in BaseZoomTiling" << endl;
		return false;
	}
    
	return true;
}


bool BundleTiler::WaitForTilingThreads(list<future<int>> *p_tiling_threads, int nMaxThreads)
{
  list<future<int>>::iterator iter;
  while (p_tiling_threads->size() >= nMaxThreads)
  {
    for (iter = p_tiling_threads->begin(); iter != p_tiling_threads->end(); iter++)
    {
      if ((*iter).wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
      {
        if ((*iter).get() != 0) return false;
        else
        {
          p_tiling_threads->erase(iter);
          break;
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return true;
}


bool BundleTiler::TerminateTilingThreads(list<future<int>> &tiling_threads)
{
  for (list<future<int>>::iterator iter = tiling_threads.begin(); iter != tiling_threads.end(); iter++)
  {
#ifdef _WIN32
    //(*iter).first->
#endif
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
  if (!p_tile_mset_->CalcTileRange(buffer_envelope,zoom,min_x,min_y,max_x,max_y))
    return false;
    

	for (int x = min_x; x <= max_x; x += 1)
	{
		for (int y = min_y; y <= max_y; y += 1)
		{
      OGREnvelope tile_envelope = p_tile_mset_->CalcEnvelopeByTile(zoom,x,y);
			if (!Intersects(tile_envelope)) continue;
             
			int x_offset = (int)(((tile_envelope.MinX-buffer_envelope.MinX)/
								p_tile_mset_->CalcPixelSizeByZoom(zoom))+0.5);
			int y_offset = (int)(((buffer_envelope.MaxY-tile_envelope.MaxY)/
								p_tile_mset_->CalcPixelSizeByZoom(zoom))+0.5);
			int tile_size_x = (int)(((tile_envelope.MaxX-tile_envelope.MinX)/
								p_tile_mset_->CalcPixelSizeByZoom(zoom))+0.5);
			int tile_size_y = (int)(((tile_envelope.MaxY-tile_envelope.MinY)/
								p_tile_mset_->CalcPixelSizeByZoom(zoom))+0.5);
			RasterBuffer tile_buffer;
			void *p_tile_pixel_data = p_buffer->GetPixelDataBlock(x_offset, y_offset,tile_size_x,tile_size_y);
			tile_buffer.CreateBuffer(	p_buffer->get_num_bands(),
											tile_size_x,
											tile_size_y,
											p_tile_pixel_data,
											p_buffer->get_data_type(),
											false,
											p_buffer->get_color_table_ref());
			delete[]((unsigned char*)p_tile_pixel_data);
      
			if (p_tiling_params->nd_num_ && (p_tiling_params->tile_type_ == PNG_TILE))
				  tile_buffer.CreateAlphaBandByNodataValues(p_tiling_params->p_nd_rgbcolors_,
														  p_tiling_params->nd_num_,
														  p_tiling_params->nodata_tolerance_);

			if (p_tile_container != 0)
			{
				void *p_data=0;
				int size = 0;
				tile_buffer.SerializeToInMemoryData(p_data, size, 
													p_tiling_params->tile_type_, 
													p_tiling_params->quality_);
        
      
       	if (!p_tile_container->AddTile(zoom,x,y,(unsigned char*)p_data,size))
				{
				  if (p_data) delete[]((unsigned char*)p_data);
				  cout<<"ERROR: AddTile: writing tile to container"<<endl;
				  return false;
				}
        
					delete[]((unsigned char*)p_data);
					(*p_tiles_generated)++;
			}
			
			TTXPrintTilingProgress(tiles_expected,(*p_tiles_generated));
		}
	}
	
	return true;
}


void BundleConsoleInput::ClearAll()
{
  bands_num_=0;
  for ( auto iter : m_listInputData)
    delete[]iter.second.second;
   m_listInputData.empty();
}

BundleConsoleInput::~BundleConsoleInput()
{
  ClearAll();
}

list<string> BundleConsoleInput::GetRasterFiles()
{
  list<string> listRasters;
  for (auto iter : m_listInputData)
    listRasters.push_back(iter.first);
  return listRasters;
}


list<pair<string,string>> BundleConsoleInput::GetFiles()
{
  list<pair<string,string>> raster_vector;
  for (auto iter : m_listInputData)
  {
    pair<string,string> pairRasterVector;
    pairRasterVector.first = iter.first;
    pairRasterVector.second = iter.second.first;
    raster_vector.push_back(pairRasterVector);
  }

  return raster_vector;
}

map<string,int*> BundleConsoleInput::GetBandMapping()
{
  map<string,int*> raster_and_bands;
  for (auto iter : m_listInputData)
    raster_and_bands[iter.first] = (bands_num_ == 0) ? 0 : iter.second.second;
  return raster_and_bands;
}


bool  BundleConsoleInput::InitByConsoleParams (  list<string> listInputParam, 
                                              list<string> listBorderParam, 
                                              list<string> listBandParam)
{
	list<string>::iterator iterBand;
	list<string>::iterator iterBorder;
	int*  panBands = 0;

	bands_num_=0;
	if (listBandParam.size()>0 && listBandParam.size()!=listInputParam.size())
	{
		cout<<"ERROR: not valid option \"-bnd\" count. Option \"-bnd\" count must be zero or equal to option \"-i\" count"<<endl;
		return false;
	}
	else   if (listBorderParam.size()>0 && listBorderParam.size()!=listInputParam.size())
	{
		cout<<"ERROR: not valid option \"-b\" count. Option \"-b\" count must be zero or equal to option \"-i\" count"<<endl;
		return false;
	}
	else if (listBandParam.size()!=0)
	{
		if (!(bands_num_ = MPLString::SplitCommaSeparatedText(*listBandParam.begin()).size()))
		{
			cout<<"ERROR: not valid option \"-bnd\" value: "<<*listBandParam.begin()<<endl;
			return false;
		}
		for (iterBand=(listBandParam.begin()++);iterBand!=listBandParam.end();iterBand++)
		{
			if (bands_num_ != MPLString::SplitCommaSeparatedText(*iterBand).size())
			{
			cout<<"ERROR: not valid option \"-bnd\" value: "<<*iterBand<<endl;
			return false;
			}
		}
	}
  

	if (bands_num_!=0) iterBand=listBandParam.begin();
	if (listBorderParam.size()>0) iterBorder=listBorderParam.begin();
  
	for (list<string>::iterator iterInput=listInputParam.begin();iterInput!=listInputParam.end();iterInput++)
	{
		list<string> listRasterFiles;

		if (!MPLFileSys::FindFilesByPattern(listRasterFiles,(*iterInput)))
		{
			ClearAll();
			cout<<"ERROR: can't find files by path: "<<*iterInput<<endl;
			return false;
		}
        
		list<string> listVectorFiles;
		if (listBorderParam.size()>0 && (*iterBorder)!="")
		{
			if (!MPLFileSys::FindFilesByPattern(listVectorFiles,(*iterBorder)))
			{
				ClearAll();
				cout<<"ERROR: can't find files by path: "<<*iterBorder<<endl;
				return false;
			}
			if (listVectorFiles.size()>1 && listVectorFiles.size()!=listRasterFiles.size())
			{
				ClearAll();
				cout<<"ERROR: vector files count doesn't equal to raster file count"<<endl;
				return false;
			}
		}
       
		if (bands_num_>0)
		{
			list<string> listBands = MPLString::SplitCommaSeparatedText(*iterBand);
			panBands = new int[bands_num_];
			int i=0;
			for (string strBand : listBands)
			{
				if (strBand[0] == ' ') strBand = strBand.substr(1);
				if (strBand != "")
					strBand = strBand[strBand.size() - 1] == ' ' ? strBand.substr(0, strBand.size()-1) : strBand;
				panBands[i] = strBand == "" ? 0 : atoi(strBand.c_str());
				i++;
			}
		}
    
		list<string>::iterator vectorFile = listVectorFiles.begin();

		for (auto rasterFile : listRasterFiles)
		{
			pair<string, pair<string, int*>> pairNewElem;
			pairNewElem.first = rasterFile;
			pairNewElem.second.first = listBorderParam.size()==0 ? "" : *vectorFile;
			if (listVectorFiles.size()>1) vectorFile++;
			pairNewElem.second.second = (bands_num_>0) ? panBands : 0;
			m_listInputData.push_back(pairNewElem);
		}

		if (bands_num_!=0) iterBand++;

		if (listBorderParam.size()>0) iterBorder++;
	}
	return true;
}


}