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


BOOL RasterFile::Close()
{
	delete(p_gdal_ds_);
	p_gdal_ds_ = NULL;
	raster_file_	= "";
	num_bands_			= 0;
	//m_strImageFormat	= "";
	is_georeferenced_ = FALSE;
	
	return TRUE;
}


BOOL RasterFile::Init(string raster_file, BOOL is_geo_referenced, double shift_x, double shift_y)
{
	Close();
	GDALDriver *poDriver = NULL;
	
	p_gdal_ds_ = (GDALDataset *) GDALOpen(raster_file.c_str(), GA_ReadOnly );
	
	if (p_gdal_ds_==NULL)
	{
		cout<<"ERROR: RasterFile::init: can't open raster image"<<endl;
		return FALSE;
	}
	
	raster_file_ = raster_file;

	if (!(poDriver = p_gdal_ds_->GetDriver()))
	{
		cout<<"ERROR: RasterFile::Init: can't get GDALDriver from image"<<endl;
		return FALSE;
	}

	num_bands_	= p_gdal_ds_->GetRasterCount();
	height_ = p_gdal_ds_->GetRasterYSize();
	width_	= p_gdal_ds_->GetRasterXSize();
	nodata_value_ = p_gdal_ds_->GetRasterBand(1)->GetNoDataValue(&this->nodata_value_defined_);
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
			cout<<"ERROR: RasterFile::Init: can't read georeference"<<endl;
			return FALSE;
		}
	}
	
	return TRUE;
}


BOOL	RasterFile::CalcStatistics(int &bands, double *&min, double *&max, double *&mean, double *&std_dev)
{
	if (this->p_gdal_ds_ == NULL) return FALSE;
	bands	= this->num_bands_;
	min		= new double[bands];
	max		= new double[bands];
	mean	= new double[bands];
	std_dev	= new double[bands];

	for (int i=0;i<bands;i++)
	{
		if (CE_None!=this->p_gdal_ds_->GetRasterBand(i+1)->ComputeStatistics(0,&min[i],&max[i],&mean[i],&std_dev[i],NULL,NULL))
		{
			bands= 0;
			delete[]min;	min = NULL;
			delete[]max;	max = NULL;
			delete[]mean;	mean = NULL;
			delete[]std_dev;	std_dev = NULL;
		}
	}
		
	return TRUE;
}


BOOL RasterFile::get_nodata_value (int *p_nodata_value)
{
	if (!nodata_value_defined_) return FALSE;
	(*p_nodata_value) = nodata_value_;
	return TRUE;
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
}

RasterFile::RasterFile(string raster_file, BOOL is_geo_referenced)
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


BOOL RasterFile::ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference *p_ogr_sr)
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


BOOL	RasterFile::GetSpatialRef(OGRSpatialReference	&ogr_sr)
{
	const char* strProjRef = GDALGetProjectionRef(this->p_gdal_ds_);
	if (OGRERR_NONE == ogr_sr.SetFromUserInput(strProjRef)) return TRUE;

	/*
  if (FileExists(RemoveExtension(this->raster_file_)+".prj"))
	{
		string prjFile		= RemoveExtension(this->raster_file_)+".prj";
		return 	(OGRERR_NONE==ogr_sr.SetFromUserInput(prjFile.c_str()));	
	}
	else 
  */
  if (FileExists(RemoveExtension(this->raster_file_)+".tab"))
		return ReadSpatialRefFromMapinfoTabFile(RemoveExtension(this->raster_file_)+".tab",&ogr_sr);

	return FALSE;
}

BOOL	RasterFile::GetDefaultSpatialRef (OGRSpatialReference	&ogr_sr, MercatorProjType merc_type)
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
	
	BOOL	intersects180 = VectorBorder::Intersects180Degree(&lr,&ogr_sr);
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


BundleOfRasterFiles::BundleOfRasterFiles(void)
{
	//this->m_poImages = NULL;
	//this->m_nLength = 0;
}

void BundleOfRasterFiles::Close(void)
{
	
	//m_nLength = 0;
	//m_poImages = NULL;
	//m_strFilesList.MakeEmpty();
	//this->m_oImagesBounds.MakeEmpty();
	//this->m_poImagesBorders.MakeEmpty();

	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
		delete((*iter).second.second);
		delete((*iter).second.first);
	}
	data_list_.empty();

}

BundleOfRasterFiles::~BundleOfRasterFiles(void)
{
	Close();
}


int	BundleOfRasterFiles::Init (string inputPath, MercatorProjType merc_type, string vector_file, double shift_x, double shift_y)
{
	Close();
	list<string> file_list;
	if (!FindFilesInFolderByPattern(file_list,inputPath)) return 0;

	merc_type_ = merc_type;
	for (std::list<string>::iterator iter = file_list.begin(); iter!=file_list.end(); iter++)
	{
		if (file_list.size()==1) AddItemToBundle((*iter),vector_file,shift_x,shift_y);
		else AddItemToBundle((*iter),VectorBorder::GetVectorFileNameByRasterFileName(*iter),shift_x,shift_y);
	}
	return data_list_.size();

	return 1;
}




BOOL	BundleOfRasterFiles::AddItemToBundle (string raster_file, string vector_file, double shift_x, double shift_y)
{	
	RasterFile image;

	if (!image.Init(raster_file,TRUE,shift_x,shift_y))
	{
		cout<<"ERROR: can't init. image: "<<raster_file<<endl;
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

list<string>	BundleOfRasterFiles::GetFileList()
{
	std::list<string> file_list;
	for (std::list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end(); iter++)
		file_list.push_back((*iter).first);

	return file_list;
	//return this->m_strFilesList;
}

OGREnvelope BundleOfRasterFiles::CalcMercEnvelope()
{
	OGREnvelope	envp;


	envp.MaxY=(envp.MaxX = -1e+100);envp.MinY=(envp.MinX = 1e+100);
	if (data_list_.size() == 0) return envp;

	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
		if ((*iter).second.second != NULL)
			envp = VectorBorder::CombineOGREnvelopes(envp,(*iter).second.second->GetEnvelope());
		else if ((*iter).second.first != NULL)
			envp = VectorBorder::CombineOGREnvelopes(envp,*(*iter).second.first);
	}
	return envp; 
}


int	BundleOfRasterFiles::CalcNumberOfTiles (int zoom)
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



int		BundleOfRasterFiles::CalcBestMercZoom()
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


list<string>	 BundleOfRasterFiles::GetFileListByEnvelope(OGREnvelope envp_merc)
{
	std::list<string> file_list;
	
	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
		if ((*iter).second.second->Intersects(envp_merc)) file_list.push_back((*iter).first);

	}
	
	return file_list;
}


BOOL	BundleOfRasterFiles::Intersects(OGREnvelope envp_merc)
{
	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
		if ((*iter).second.second != NULL)
    {
			if ((*iter).second.second->Intersects(envp_merc)) return TRUE;
    }
    else if ((*iter).second.first != NULL)
    {
			if ((*iter).second.first->Intersects(envp_merc)) return TRUE;
    }
	}
	
	return FALSE;
}


BOOL BundleOfRasterFiles::WarpToMercBuffer (int zoom,	
                                            OGREnvelope	envp_merc, 
                                            RasterBuffer *p_buffer, 
                                            string resampling_alg, 
                                            int *p_nodata_value, 
                                            BYTE *p_def_color,
                                            string temp_file_path,
                                            int srand_seed)
{
	//создать виртуальный растр по envp_merc и zoom
	//создать объект GDALWarpOptions 
	//вызвать ChunkAndWarpImage
	//вызвать RasterIO для виртуального растра
	//удалить виртуальный растр, удалить все объекты
	//создать RasterBuffer

	if (data_list_.size()==0) return FALSE;
	GDALDataset	*p_src_ds = (GDALDataset*)GDALOpen((*data_list_.begin()).first.c_str(),GA_ReadOnly );
	if (p_src_ds==NULL)
	{
		cout<<"ERROR: can't open raster file: "<<(*data_list_.begin()).first<<endl;
		return FALSE;
	}
	GDALDataType	dt		= GDALGetRasterDataType(GDALGetRasterBand(p_src_ds,1));
	int				bands	= p_src_ds->GetRasterCount();
	//int				bands	= 3;
  BOOL			nodata_val_from_file_defined;
	int				nodata_val_from_file = (int) p_src_ds->GetRasterBand(1)->GetNoDataValue(&nodata_val_from_file_defined);
	
	double		res			=  MercatorTileGrid::CalcResolutionByZoom(zoom);
	int				buf_width	= int(((envp_merc.MaxX - envp_merc.MinX)/res)+0.5);
	int				buf_height	= int(((envp_merc.MaxY - envp_merc.MinY)/res)+0.5);
	
  srand(srand_seed);
  string			tiff_in_mem = (temp_file_path == "" ) ? ("/vsimem/tiffinmem" + ConvertIntToString(rand()))
                                                    : RemoveEndingSlash(temp_file_path) + "/" + ConvertIntToString(rand()) + ".gdal.temp";
	GDALDataset*	p_vrt_ds = (GDALDataset*)GDALCreate(
								GDALGetDriverByName("GTiff"),
								tiff_in_mem.c_str(),
								buf_width,
								buf_height,
								bands,
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

	if (nodata_val_from_file_defined) p_vrt_ds->GetRasterBand(1)->SetNoDataValue(nodata_val_from_file);
	
	for (list<pair<string,pair<OGREnvelope*,VectorBorder*>>>::iterator iter = data_list_.begin(); iter!=data_list_.end();iter++)
	{
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
			cout<<"ERROR: can't open raster file: "<<(*iter).first<<endl;
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
				cout<<"ERROR: can't read spatial reference from input file: "<<(*data_list_.begin()).first<<endl;
				return FALSE;
			}
		}
		input_ogr_sr.exportToWkt(&p_src_wkt);
		CPLAssert( p_src_wkt != NULL && strlen(p_src_wkt) > 0 );


		GDALWarpOptions *p_warp_options = GDALCreateWarpOptions();

    //p_warp_options->papszWarpOptions

    //char **papszWarpOptions; 
    //papszWarpOptions = CSLSetNameValue(NULL,"INIT_DEST","NO_DATA");
    p_warp_options->papszWarpOptions = NULL;
    //p_warp_options->papszWarpOptions = CSLSetNameValue(NULL,"INIT_DEST","NO_DATA");
    p_warp_options->papszWarpOptions = CSLSetNameValue(p_warp_options->papszWarpOptions,"NUM_THREADS", "ALL_CPUS");
    
		p_warp_options->hSrcDS = p_src_ds;
		p_warp_options->hDstDS = p_vrt_ds;
		p_warp_options->dfWarpMemoryLimit = 250000000; 
    //p_warp_options->dfWarpMemoryLimit = 150000000; 
		double			error_threshold = 0.125;

		p_warp_options->nBandCount = 0;
    /*
    p_warp_options->panSrcBands = new int[3];
    p_warp_options->panDstBands = new int[3];
    p_warp_options->panSrcBands[0] = 1;
    p_warp_options->panSrcBands[1] = 2;
    p_warp_options->panSrcBands[2] = 3;
    p_warp_options->panDstBands[0] = 1;
    p_warp_options->panDstBands[1] = 2;
    p_warp_options->panDstBands[2] = 3;
    */
    //p_warp_options->nSrcAlphaBand = 0;
    //p_warp_options->nDstAlphaBand = 0;
    
		
		
		//Init cutline for source file
		if ((*iter).second.second)
		{
			VectorBorder	*p_vb = (*iter).second.second;
			//OGRGeometry		*p_ogr_geom = p_vb->GetOGRGeometryTransformed(&input_ogr_sr);
			double	gdal_transform[6];
			if (CE_None == input_rf.get_gdal_ds_ref()->GetGeoTransform(gdal_transform))
			{
				if (!((gdal_transform[0] == 0.) &&(gdal_transform[1]==1.)))
					p_warp_options->hCutline = p_vb->GetOGRPolygonTransformedToPixelLine(&input_ogr_sr,gdal_transform);
			}
		}
		//p_warp_options->hCutline = ((OGRMultiPolygon*)(*iter).second.second)->getGeometryRef(0)->clone();
		//((OGRPolygon*)p_warp_options->hCutline)->closeRings();

		

		// p_warp_options->pfnProgress = GDALTermProgress;   
		p_warp_options->pfnProgress = gmxPrintNoProgress;  

		// Establish reprojection transformer. 

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
    if ((resampling_alg == "near") || (resampling_alg == "nearest")) p_warp_options->eResampleAlg = GRA_NearestNeighbour;
    else if (resampling_alg == "bilinear") p_warp_options->eResampleAlg = GRA_Bilinear;
    else if (resampling_alg == "lanczos") p_warp_options->eResampleAlg = GRA_Lanczos;
    else p_warp_options->eResampleAlg = GRA_Cubic; 
    

		// Initialize and execute the warp operation. 
		GDALWarpOperation gdal_warp_operation;
		gdal_warp_operation.Initialize( p_warp_options );
		
   
    BOOL  warp_error = FALSE;
    //if (CE_None != gdal_warp_operation.ChunkAndWarpImage( 0,0,buf_width,buf_height))

    if (CE_None != gdal_warp_operation.ChunkAndWarpMulti( 0,0,buf_width,buf_height))
		{
			cout<<"ERROR: warping raster block of image: "<<(*iter).first<<endl;
      warp_error = TRUE;
		}
    //p_warp_options->panDstBands = NULL;
    //p_warp_options->panSrcBands = NULL;


		GDALDestroyApproxTransformer(p_warp_options->pTransformerArg );
		GDALDestroyWarpOptions( p_warp_options );
		OGRFree(p_src_wkt);
		input_rf.Close();
		GDALClose( p_src_ds );
    if (warp_error) return FALSE;
	}

  ///*

	p_buffer->CreateBuffer(bands,buf_width,buf_height,NULL,dt,p_nodata_value,FALSE,
						p_vrt_ds->GetRasterBand(1)->GetColorTable());
	int nodata_value_from_file = 0;
	if	(p_nodata_value) p_buffer->InitByNoDataValue(p_nodata_value[0]);
	else if (p_def_color) p_buffer->InitByRGBColor(p_def_color); 
	else if (nodata_val_from_file_defined) p_buffer->InitByNoDataValue(nodata_val_from_file);
	p_vrt_ds->RasterIO(	GF_Read,0,0,buf_width,buf_height,p_buffer->get_pixel_data_ref(),
						buf_width,buf_height,p_buffer->get_data_type(),
						p_buffer->get_num_bands(),NULL,0,0,0);
	//*/

	OGRFree(p_dst_wkt);
	GDALClose(p_vrt_ds);
	VSIUnlink(tiff_in_mem.c_str());
	return TRUE;
}


}