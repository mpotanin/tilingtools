#include "stdafx.h"
#include "TilingFuncs.h"

using namespace gmx;

int	GMX_MAX_BUFFER_WIDTH	= 16;

int			GMX_MAX_WORK_THREADS	= 2;
int			GMX_MAX_WARP_THREADS	= 1;
int			GMX_CURR_WORK_THREADS	= 0;
HANDLE  GMX_WARP_SEMAPHORE = NULL;


int GMXTilingParameters::CalcOutputBandsNum (gmx::RasterFileBundle *p_bundle)
{
  if (!p_bundle) return 0;
  else if (tile_type_==JPEG_TILE || tile_type_==PNG_TILE) return 3;
  else if (p_band_mapping_) return p_band_mapping_->GetBandsNum();
  else
  {
    RasterFile rf(*p_bundle->GetFileList().begin(),1);
    if (!rf.get_gdal_ds_ref()) 
    {
      cout<<"Error: can't init. gdaldataset from file: "<<(*p_bundle->GetFileList().begin())<<endl;
      return 0;
    }
    return rf.get_gdal_ds_ref()->GetRasterCount();
  }
  return 0;
}

GDALDataType GMXTilingParameters::GetOutputDataType (gmx::RasterFileBundle *p_bundle)
{
  if (!p_bundle) return GDT_Byte;
  else if (tile_type_==JPEG_TILE || tile_type_==PNG_TILE) return GDT_Byte;
  else
  {
    RasterFile rf(*p_bundle->GetFileList().begin(),1);
    if (!rf.get_gdal_ds_ref()) 
    {
      cout<<"Error: can't init. gdaldataset from file: "<<(*p_bundle->GetFileList().begin())<<endl;
      return GDT_Byte;
    }
    return rf.get_gdal_ds_ref()->GetRasterBand(1)->GetRasterDataType();
  }
  return GDT_Byte;
}


GMXTilingParameters::GMXTilingParameters(list<string> file_list, gmx::MercatorProjType merc_type, gmx::TileType tile_type)
{

	file_list_ = file_list;
	merc_type_	= merc_type;
	tile_type_	= tile_type;
		
	use_container_ = TRUE;
	jpeg_quality_ = 0;
	shift_x_ = 0;
	shift_y_ = 0;
	p_tile_name_ = NULL;
	p_background_color_	= NULL;
	p_transparent_color_ = NULL;
	nodata_tolerance_ = 0;
	base_zoom_	= 0;
	min_zoom_	= 0;
	max_cache_size_	= 0;
	max_gmx_volume_size_ =0;
  auto_stretching_ = FALSE;

  p_band_mapping_ = 0;

  max_work_threads_= 0;
  max_warp_threads_= 0;
    
  temp_file_path_for_warping_ = "";
  calculate_histogram_=false;
}

		
GMXTilingParameters::~GMXTilingParameters ()
{
  if (p_background_color_)  delete(p_background_color_);
	if (p_transparent_color_) delete(p_transparent_color_);
  if (p_tile_name_)         delete(p_tile_name_);
}




bool GMXPrintTilingProgress (int tiles_expected, int tiles_generated)
{
	//cout<<tiles_generated<<" "<<endl;
	if (tiles_generated - (int)ceil((tiles_expected/10.0)*(tiles_generated*10/tiles_expected))  ==0)
	{
		cout<<(tiles_generated*10/tiles_expected)*10<<" ";
		fflush(stdout);
	}
	return TRUE;
}


bool GMXMakeTiling		(GMXTilingParameters		*p_tiling_params)
{
	LONG t_ = time(0);
	srand(t_%10000);
	RasterFileBundle		raster_bundle;
	int tile_size = MercatorTileGrid::TILE_SIZE;

  
  if (!raster_bundle.Init(p_tiling_params->file_list_,p_tiling_params->merc_type_,p_tiling_params->vector_file_,p_tiling_params->shift_x_,p_tiling_params->shift_y_))
	{
		cout<<"Error: can't init raster bundle object"<<endl;
		return FALSE;
	}

	int base_zoom = (p_tiling_params->base_zoom_ == 0) ? raster_bundle.CalcBestMercZoom() : p_tiling_params->base_zoom_;
	if (base_zoom<=0)
	{
		cout<<"Error: can't calculate base zoom for tiling"<<endl;
		return FALSE;
	}

  extern __int64 GMX_TILE_CACHE_MAX_SIZE;
	
  GMX_MAX_WARP_THREADS	= p_tiling_params->max_warp_threads_ > 0 ? p_tiling_params->max_warp_threads_ : GMX_MAX_WARP_THREADS;
  GMX_MAX_WORK_THREADS	= p_tiling_params->max_work_threads_ > 0 ? p_tiling_params->max_work_threads_ : GMX_MAX_WORK_THREADS;
  GMX_TILE_CACHE_MAX_SIZE   = p_tiling_params->max_cache_size_ > 0 ? p_tiling_params->max_cache_size_  : GMX_TILE_CACHE_MAX_SIZE;
  
  unsigned int max_gmx_volume_size   = p_tiling_params->max_gmx_volume_size_ > 0 ? p_tiling_params->max_gmx_volume_size_ 
                                                                                  : GMXTileContainer::DEFAULT_MAX_VOLUME_SIZE;
  
  ITileContainer	*p_itile_pyramid =	NULL;
  Metadata metadata;
  MetaHistogram histogram;
  MetaNodataValue nodata_value;
  MetaHistogramStatistics hist_stat;

  bool nodata_defined = false;
  double  nodata_val;
  if (p_tiling_params->p_transparent_color_)
  {
    nodata_defined = true;
    nodata_val = p_tiling_params->p_transparent_color_[0];
  }
  else nodata_val = raster_bundle.GetNodataValue(nodata_defined);
  if (nodata_defined) nodata_value.nodv_ = nodata_val;
  
  
  // initializing p_itile_pyramid - depends on specified 
  // output format of tile pyramid: .gmxtiles, .mbtiles, tile folder etc.
  if (  p_tiling_params->use_container_ && 
        ( GetExtension(p_tiling_params->container_file_) == "tiles" || 
          GetExtension(p_tiling_params->container_file_) == "gmxtiles")
      )
	{
    p_itile_pyramid = new GMXTileContainer();
     
    

    if (nodata_defined) metadata.AddTagRef(&nodata_value);
    if (p_tiling_params->calculate_histogram_)
    {
      histogram.Init( p_tiling_params->CalcOutputBandsNum(&raster_bundle),
                      p_tiling_params->GetOutputDataType(&raster_bundle));
      metadata.AddTagRef(&histogram);
      hist_stat.Init(p_tiling_params->CalcOutputBandsNum(&raster_bundle));
      metadata.AddTagRef(&hist_stat);
    }
    if ( !((GMXTileContainer*)p_itile_pyramid)->OpenForWriting(p_tiling_params->container_file_,
														                                  p_tiling_params->tile_type_,
														                                  p_tiling_params->merc_type_,
														                                  raster_bundle.CalcMercEnvelope(),
														                                  base_zoom,
														                                  TRUE,
                                                              &metadata,
                                                              max_gmx_volume_size))
    {
      cout<<"Error: can't open for writing gmx-container file: "<<p_tiling_params->container_file_<<endl;
      return FALSE;
    }
  }
  else if (  p_tiling_params->use_container_ && GetExtension(p_tiling_params->container_file_) == "mbtiles")
  {
    p_itile_pyramid = new MBTileContainer(	p_tiling_params->container_file_,
		p_tiling_params->tile_type_,
		p_tiling_params->merc_type_,
		raster_bundle.CalcMercEnvelope());
  }
	else 
    p_itile_pyramid = new TileFolder(p_tiling_params->p_tile_name_,TRUE);
  
  cout<<"Base zoom "<<base_zoom<<": ";
	if (!GMXMakeBaseZoomTiling( p_tiling_params,
                              &raster_bundle,
                              p_itile_pyramid,
                              p_tiling_params->calculate_histogram_ ? &histogram : NULL)) 
  {
    p_itile_pyramid->Close();
	  delete(p_itile_pyramid);
    return FALSE;
  }
	cout<<" done."<<endl;
  

  if (histogram.IsInitiated())
    histogram.CalcStatistics(&hist_stat,(nodata_defined) ? &nodata_val : 0); 
  
  int min_zoom = (p_tiling_params->min_zoom_ <=0) ? 1 : p_tiling_params->min_zoom_;
	
  if (min_zoom < base_zoom)
  {
	  cout<<"Pyramid tiles: ";//<<endl;
	  cout<<"calculating number of tiles: ";
    VectorBorder	bundle_envp(raster_bundle.CalcMercEnvelope(),p_tiling_params->merc_type_);
	  int tiles_generated = 0;
	  int tiles_expected = 0;
	  GMXMakePyramidFromBaseZoom(	bundle_envp,
								  base_zoom,
								  min_zoom,
								  p_tiling_params,
								  tiles_expected,
								  tiles_generated,
								  TRUE,
								  p_itile_pyramid);
	  cout<<tiles_expected<<endl;
	  if (tiles_expected > 0) 
	  {
		  cout<<"0% ";
		  if (!GMXMakePyramidFromBaseZoom(	bundle_envp,
									  base_zoom,
									  min_zoom,
									  p_tiling_params,
									  tiles_expected,
									  tiles_generated,
									  FALSE,
									  p_itile_pyramid))
      {
        cout<<"Error: GMXMakePyramidFromBaseZoom"<<endl;
        p_itile_pyramid->Close();
	      delete(p_itile_pyramid);
        return FALSE;
      }
		  cout<<" done."<<endl;
	  }
  }
 
	if (!p_itile_pyramid->Close())
  {
    cout<<"Error: can't save tile container to disk"<<endl;
    return FALSE;
  }
	delete(p_itile_pyramid);

	return TRUE;
}


bool GMXMakeTilingFromBuffer (GMXTilingParameters			*p_tiling_params, 
						   RasterBuffer					*p_buffer, 
						   RasterFileBundle			*p_bundle, 
						   int							ul_x, 
						   int							ul_y,
						   int							z,
						   int							tiles_expected, 
						   int							*p_tiles_generated,
						   ITileContainer					*p_tile_container)
{  
	int tile_size = MercatorTileGrid::TILE_SIZE;
  
	for (int x = ul_x; x < ul_x+p_buffer->get_x_size()/tile_size; x += 1)
	{
		for (int y = ul_y; y < ul_y+p_buffer->get_y_size()/tile_size; y += 1)
		{	
			OGREnvelope tile_envelope = MercatorTileGrid::CalcEnvelopeByTile(z,x,y);
			if (!p_bundle->Intersects(tile_envelope)) continue;
			
			
			RasterBuffer tile_buffer;
			void *p_tile_pixel_data = p_buffer->GetPixelDataBlock((x-ul_x)*tile_size, (y-ul_y)*tile_size,tile_size,tile_size);
			tile_buffer.CreateBuffer(	p_buffer->get_num_bands(),
											tile_size,
											tile_size,
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
        ///*
				if (!p_tile_container->AddTile(z,x,y,(BYTE*)p_data,size))
        {
          if (p_data) delete[]((BYTE*)p_data);
          cout<<"Error: AddTile: writing tile to container"<<endl;
          return FALSE;
        }
        //*/
				delete[]((BYTE*)p_data);
				(*p_tiles_generated)++;
			}
			
			GMXPrintTilingProgress(tiles_expected,(*p_tiles_generated));
		}
	}
	
	return TRUE;
}




DWORD WINAPI GMXAsyncWarpChunkAndMakeTiling (LPVOID lpParam)
{
  GMX_CURR_WORK_THREADS++;

  GMXAsyncChunkTilingParams   *p_chunk_tiling_params = (GMXAsyncChunkTilingParams*)lpParam;
  
  GMXTilingParameters		*p_tiling_params = p_chunk_tiling_params->p_tiling_params_;
  RasterFileBundle		*p_bundle = p_chunk_tiling_params->p_bundle_;
  ITileContainer			  *p_tile_container = p_chunk_tiling_params->p_tile_container_;
  int                   zoom = p_chunk_tiling_params->z_;
  OGREnvelope           chunk_envp = p_chunk_tiling_params->chunk_envp_;
   
  RasterBuffer *p_merc_buffer = new RasterBuffer();
  WaitForSingleObject(GMX_WARP_SEMAPHORE,INFINITE);

  //ToDo...

  int bands_num=0;
  int **pp_band_mapping=0;
  if (p_tiling_params->p_band_mapping_)
    p_tiling_params->p_band_mapping_->GetBandMappingData(bands_num,pp_band_mapping);
    
  bool warp_result = p_bundle->WarpToMercBuffer(zoom,
                                                chunk_envp,
                                                p_merc_buffer,
                                                bands_num,
                                                pp_band_mapping,
                                                p_tiling_params->gdal_resampling_,
                                                p_tiling_params->p_transparent_color_,
                                                p_tiling_params->p_background_color_);
  ReleaseSemaphore(GMX_WARP_SEMAPHORE,1,NULL);

  
  if (!warp_result)	
  {
	  cout<<"Error: BaseZoomTiling: warping to merc fail"<<endl;
    GMX_CURR_WORK_THREADS--;
	  return FALSE;
	}

  if (p_chunk_tiling_params->need_stretching_)	
  {
    if (! p_merc_buffer->StretchDataTo8Bit (
                          p_chunk_tiling_params->p_stretch_min_values_,
                          p_chunk_tiling_params->p_stretch_max_values_))
    {
      cout<<"Error: can't stretch raster values to 8 bit"<<endl;
      GMX_CURR_WORK_THREADS--;
			return FALSE;
		}
	}

  gmx::MetaHistogram        *p_histogram = p_chunk_tiling_params->p_histogram_;
  if (p_histogram) 
  {
    if (!p_histogram->IsInitiated())
      p_histogram->Init(p_merc_buffer->get_num_bands(),p_merc_buffer->get_data_type());
    p_merc_buffer->AddPixelDataToMetaHistogram(p_histogram);
  }
    
  int min_x,min_y,max_x,max_y;
  MercatorTileGrid::CalcTileRange(chunk_envp,zoom,min_x,min_y,max_x,max_y);

  int                   tiles_expected = p_chunk_tiling_params->tiles_expected_;
  int                   *p_tiles_generated = p_chunk_tiling_params->p_tiles_generated_;

  if (!GMXMakeTilingFromBuffer(p_tiling_params,
										p_merc_buffer,
										p_bundle,
										min_x,
										min_y,
										zoom,
										tiles_expected,
										p_tiles_generated,
										p_tile_container))
	{
			cout<<"Error: BaseZoomTiling: GMXMakeTilingFromBuffer fail"<<endl;
			GMX_CURR_WORK_THREADS--;
      return FALSE;
	}
 
	delete(p_merc_buffer);
  GMX_CURR_WORK_THREADS--;
  return TRUE;
}


bool GMXMakeBaseZoomTiling	(	GMXTilingParameters		*p_tiling_params, 
								RasterFileBundle		*p_bundle, 
								ITileContainer			*p_tile_container,
                MetaHistogram           *p_histogram)
{
  //ToDo
  srand(0);
	int			tiles_generated = 0;
	int			zoom = (p_tiling_params->base_zoom_ == 0) ? p_bundle->CalcBestMercZoom() : p_tiling_params->base_zoom_;
	double		res = MercatorTileGrid::CalcResolutionByZoom(zoom);

  cout<<"calculating number of tiles: ";
	int tiles_expected	= p_bundle->CalcNumberOfTiles(zoom);	
	cout<<tiles_expected<<endl;
  if (tiles_expected == 0) return FALSE;
 

  
	bool		need_stretching = false;
	double		*p_stretch_min_values = NULL, *p_stretch_max_values = NULL;

  GMX_WARP_SEMAPHORE = CreateSemaphore(NULL,GMX_MAX_WARP_THREADS,GMX_MAX_WARP_THREADS,NULL);
    
  if (p_tiling_params->auto_stretching_)
	{
    if (  (p_tiling_params->tile_type_ == JPEG_TILE || p_tiling_params->tile_type_ == PNG_TILE) && 
          (p_bundle->GetRasterFileType()!= GDT_Byte)  
       )
		{
			need_stretching = true;
			cout<<"WARNING: input raster doesn't match 8 bit/band. Auto stretching to 8 bit will be performed"<<endl;
      double nodata_val = (p_tiling_params->p_transparent_color_) ?
                           p_tiling_params->p_transparent_color_[0] : 0;
      if (!p_bundle->CalclValuesForStretchingTo8Bit(p_stretch_min_values,
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
  MercatorTileGrid::CalcTileRange(p_bundle->CalcMercEnvelope(),zoom,minx,miny,maxx,maxy);

	HANDLE			thread_handle = NULL;
	//unsigned long	thread_id;

  //int num_warp = 0;
  
  bool tiling_error = FALSE;
  
  unsigned long thread_id;
  list<pair<HANDLE,GMXAsyncChunkTilingParams*>> thread_params_list; 
  
  for (int curr_min_x = minx; curr_min_x<=maxx; curr_min_x+=GMX_MAX_BUFFER_WIDTH)
	{
		int curr_max_x =	(curr_min_x + GMX_MAX_BUFFER_WIDTH - 1 > maxx) ? 
							maxx : curr_min_x + GMX_MAX_BUFFER_WIDTH - 1;
		
		for (int curr_min_y = miny; curr_min_y<=maxy; curr_min_y+=GMX_MAX_BUFFER_WIDTH)
		{
			int curr_max_y =	(curr_min_y + GMX_MAX_BUFFER_WIDTH - 1 > maxy) ? 
								maxy : curr_min_y + GMX_MAX_BUFFER_WIDTH - 1;
			
			OGREnvelope chunk_envp = MercatorTileGrid::CalcEnvelopeByTileRange(	zoom,
																					curr_min_x,
																					curr_min_y,
																					curr_max_x,
																					curr_max_y);
			if (!p_bundle->Intersects(chunk_envp)) continue;
	    
      while (GMX_CURR_WORK_THREADS >= GMX_MAX_WORK_THREADS)        
        Sleep(100);
      
      for (list<pair<HANDLE,GMXAsyncChunkTilingParams*>>::iterator iter = 
          thread_params_list.begin();iter!=thread_params_list.end();iter++)
      {
        DWORD exit_code;
        if (GetExitCodeThread((*iter).first,&exit_code))
        {
          if (exit_code != STILL_ACTIVE)
          {
            CloseHandle((*iter).first);
            delete((*iter).second);
            thread_params_list.remove(*iter);
            break;
          }
        }
      }

      GMXAsyncChunkTilingParams	*p_chunk_tiling_params = new  GMXAsyncChunkTilingParams();
      unsigned long *p_thread_id = new unsigned long();
      
      p_chunk_tiling_params->p_tiling_params_ = p_tiling_params;
      p_chunk_tiling_params->chunk_envp_ = chunk_envp;
      p_chunk_tiling_params->p_bundle_ = p_bundle;
      p_chunk_tiling_params->p_tile_container_ = p_tile_container;
      p_chunk_tiling_params->p_tiles_generated_ = &tiles_generated;
      p_chunk_tiling_params->tiles_expected_ = tiles_expected;
      p_chunk_tiling_params->need_stretching_ = need_stretching;
      p_chunk_tiling_params->p_stretch_min_values_ = p_stretch_min_values;
      p_chunk_tiling_params->p_stretch_max_values_ = p_stretch_max_values;
      p_chunk_tiling_params->z_ = zoom;
      p_chunk_tiling_params->p_histogram_ = p_histogram;
      HANDLE hThread = CreateThread(NULL,0,GMXAsyncWarpChunkAndMakeTiling,p_chunk_tiling_params,0,&thread_id);      
      if (hThread)
         thread_params_list.push_back(
        pair<HANDLE,GMXAsyncChunkTilingParams*>(hThread,p_chunk_tiling_params));
      Sleep(100);    
    }
	}

  while (GMX_CURR_WORK_THREADS > 0)
    Sleep(250);

  for (list<pair<HANDLE,GMXAsyncChunkTilingParams*>>::iterator iter = 
          thread_params_list.begin();iter!=thread_params_list.end();iter++)
  {
    CloseHandle((*iter).first);
    delete((*iter).second);
  }
  thread_params_list.end();

  	
  if (GMX_WARP_SEMAPHORE)
    CloseHandle(GMX_WARP_SEMAPHORE);

	return TRUE;
}


bool GMXMakePyramidFromBaseZoom (	VectorBorder		&vb, 
								int					base_zoom, 
								int					min_zoom, 
								GMXTilingParameters		*p_tiling_params, 
								int					&tiles_expected, 
								int					&tiles_generated, 
								bool					only_calculate, 
								ITileContainer			*p_itile_pyramid
								)
{

	RasterBuffer oBuffer;
  bool was_error = FALSE;
	bool b;

	GMXMakePyramidTileRecursively(	vb,0,0,0,base_zoom,min_zoom,p_tiling_params,oBuffer,tiles_expected,
							tiles_generated,only_calculate,p_itile_pyramid,&was_error);
  
  if (vb.GetEnvelope().MaxX>-MercatorTileGrid::ULX())
	{
		GMXMakePyramidTileRecursively(	vb,1,2,0,base_zoom,min_zoom,p_tiling_params,oBuffer,tiles_expected,
								tiles_generated,only_calculate,p_itile_pyramid,&was_error);
		GMXMakePyramidTileRecursively(vb,1,2,1,base_zoom,min_zoom,p_tiling_params,oBuffer,tiles_expected,
								tiles_generated,only_calculate,p_itile_pyramid,&was_error);
	}

  if (was_error)
  {
    cout<<"Error: GMXMakePyramidTileRecursively"<<endl;
    return FALSE;
  }
  else return TRUE;
}


bool GMXMakePyramidTileRecursively (VectorBorder	&vb, 
					  int				zoom, 
					  int				nX, 
					  int				nY, 
					  int				base_zoom, 
					  int				min_zoom, 
					  GMXTilingParameters	*p_tiling_params, 
					  RasterBuffer		&tile_buffer,  
					  int				&tiles_expected, 
					  int				&tiles_generated, 
					  bool				only_calculate, 
					  ITileContainer		*p_itile_pyramid, 
            bool      *p_was_error
            )
{

	if (zoom==base_zoom)
	{	
		if (!p_itile_pyramid->TileExists(zoom,nX,nY)) return FALSE;
		if (only_calculate) return TRUE;

		unsigned int size	= 0;
		BYTE		*p_data	= NULL;
	
		p_itile_pyramid->GetTile(zoom,nX,nY,p_data,size);
		if (size ==0) return FALSE;

   	switch (p_tiling_params->tile_type_)
		{
			case JPEG_TILE:
				{
					if(!tile_buffer.CreateBufferFromJpegData(p_data,size))
					{
						cout<<"Error: reading jpeg-data"<<endl;
						return FALSE;
					}
					break;
				}
      case PSEUDO_PNG_TILE:
				{
          tile_buffer.CreateBufferFromPseudoPngData(p_data,size);
					break;
				}
			case PNG_TILE:
				{
					tile_buffer.CreateBufferFromPngData(p_data,size);
					break;
				}
      case JP2_TILE:
        {
          tile_buffer.createFromJP2Data(p_data,size);
          break;
        }
			default:
				tile_buffer.CreateBufferFromTiffData(p_data,size);
		}
		delete[]((BYTE*)p_data);
	
		return TRUE;
	}
	else
	{
		RasterBuffer	quarter_tile_buffer[4];
		bool			src_quarter_tile_buffers_def[4];

    for (int i=0;i<2;i++)
		{
			for (int j=0;j<2;j++)
			{
				if (vb.Intersects(zoom+1,2*nX+j,2*nY+i))
        {
					src_quarter_tile_buffers_def[i*2+j] = GMXMakePyramidTileRecursively(vb,
															zoom+1,
															2*nX+j,
															2*nY+i,
															base_zoom,
															min_zoom,
															p_tiling_params,
															quarter_tile_buffer[i*2+j],
															tiles_expected,
															tiles_generated,
															only_calculate,
															p_itile_pyramid,
                              p_was_error);
          if ((*p_was_error)) return FALSE;
        }
				else src_quarter_tile_buffers_def[i*2+j] = FALSE;
			}
		}
		
		if (zoom < min_zoom)		return FALSE;
		if ((!src_quarter_tile_buffers_def[0])&&
			(!src_quarter_tile_buffers_def[1])&&
			(!src_quarter_tile_buffers_def[2])&&
			(!src_quarter_tile_buffers_def[3]) )	return FALSE;
	
		if (only_calculate)
		{
     	tiles_expected++;
			return TRUE;
		}

		//ToDo
		//Проверить длину очереди на выполнение операций: ZoomOut + SaveData + запись результатов
		//Если < Lmax, то отправить на выполнение
		//Если >=Lmax, то ждать
    
    if (!GMXZoomOutTileBuffer(quarter_tile_buffer,
                              src_quarter_tile_buffers_def, 
                              tile_buffer,
                              p_tiling_params->gdal_resampling_,
                              p_tiling_params->p_background_color_)) return FALSE;
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


		(*p_was_error) =  (!p_itile_pyramid->AddTile(zoom,nX,nY,(BYTE*)p_data,size));
    delete[]((BYTE*)p_data);
		if (*p_was_error) return FALSE;
    tiles_generated++;
		GMXPrintTilingProgress(tiles_expected,tiles_generated);
	}

	return TRUE;
}





bool GMXZoomOutTileBuffer ( RasterBuffer src_quarter_tile_buffers[4], 
                            bool src_quarter_tile_buffers_def[4], 
                            RasterBuffer &zoomed_out_tile_buffer, 
                            string      resampling_method,
                            BYTE *p_background) 
{
	int i;
	for (i = 0; i<4;i++)
		if (src_quarter_tile_buffers_def[i]) break;
	if (i==4) return FALSE;
	
	int tile_size = MercatorTileGrid::TILE_SIZE;
	zoomed_out_tile_buffer.CreateBuffer(src_quarter_tile_buffers[i].get_num_bands(),
									tile_size,
									tile_size,
									NULL,
									src_quarter_tile_buffers[i].get_data_type(),
									src_quarter_tile_buffers[i].IsAlphaBand(),
                  src_quarter_tile_buffers[i].get_color_table_ref());

  
  if (p_background && src_quarter_tile_buffers[i].get_data_type() == GDT_Byte)
    zoomed_out_tile_buffer.InitByRGBColor(p_background);

  for (int n=0;n<4;n++)
	{
		if (src_quarter_tile_buffers_def[n])
		{
			void *p_zoomed_data = src_quarter_tile_buffers[n].GetDataZoomedOut(resampling_method);
			zoomed_out_tile_buffer.SetPixelDataBlock(	(n%2)*tile_size/2,
									(n/2)*tile_size/2,
									tile_size/2,
									tile_size/2,
									p_zoomed_data);
			delete[]p_zoomed_data;
		}
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
bool GMXCleanAfterTilingFromBufer (gmx::RasterBuffer				*p_buffer)
{
	delete(p_buffer);
	return TRUE;
}


DWORD WINAPI GMXCallTilingFromBuffer( LPVOID lpParam )
{
	GMXTilingFromBufferParams	*p_from_buffer_params = (GMXTilingFromBufferParams*)lpParam;
	//ToDo
	//Обработка ошибки
	bool succeed = GMXMakeTilingFromBuffer(p_from_buffer_params->p_tiling_params_,
						p_from_buffer_params->p_buffer_,
						p_from_buffer_params->p_bundle_,
						p_from_buffer_params->ul_x_,
						p_from_buffer_params->ul_y_,
						p_from_buffer_params->z_,
						p_from_buffer_params->tiles_expected_,
						p_from_buffer_params->p_tiles_generated_,
						p_from_buffer_params->p_tile_container_);
	p_from_buffer_params->pfCleanAfterTiling(p_from_buffer_params->p_buffer_);
	GMX_CURR_NUM_OF_THREADS--;
	(*p_from_buffer_params->p_was_error_)=(!succeed);
  return succeed;
}

HANDLE GMXAsyncMakeTilingFromBuffer (	GMXTilingParameters		*p_tiling_params, 
								RasterBuffer				*p_buffer, 
								RasterFileBundle			*p_bundle, 
								int							ul_x, 
								int							ul_y,
								int							z,
								int							tiles_expected, 
								int							*p_tiles_generated,
								ITileContainer				*p_tile_container,
								unsigned long				&thread_id,
                bool                *p_was_error)
{
	//запустить GMXMakeTilingFromBuffer в отдельном потоке
	GMXTilingFromBufferParams	*p_from_buffer_params = new GMXTilingFromBufferParams;
	
	p_from_buffer_params->p_tiling_params_		= p_tiling_params;
	p_from_buffer_params->p_buffer_				= p_buffer;
	p_from_buffer_params->p_bundle_				= p_bundle;
	p_from_buffer_params->tiles_expected_		= tiles_expected;
	p_from_buffer_params->ul_x_					= ul_x;
	p_from_buffer_params->ul_y_					= ul_y;
	p_from_buffer_params->p_tiles_generated_	= p_tiles_generated;
	p_from_buffer_params->p_tile_container_		= p_tile_container;	
	p_from_buffer_params->z_					= z;
	p_from_buffer_params->pfCleanAfterTiling	=GMXCleanAfterTilingFromBufer;
  p_from_buffer_params->p_was_error_ = p_was_error;


	//Увеличить количество потоков
	GMX_CURR_NUM_OF_THREADS++;
	return CreateThread(NULL,0,GMXCallTilingFromBuffer,p_from_buffer_params,0,&thread_id);
}
*/


/*
  while (GMX_CURR_NUM_OF_THREADS>1)
		Sleep(250);
	if (thread_handle!=NULL) 
	{
		CloseHandle(thread_handle);
		thread_handle = NULL;
    if (tiling_error)
    {
      cout<<"Error: BaseZoomTiling: AsyncMakeTilingFromBuffer fail"<<endl;
      return FALSE;
    }
	}

 RasterBuffer *p_merc_buffer = new RasterBuffer();

      
      if (!p_bundle->WarpToMercBuffer(zoom,buffer_envp,p_merc_buffer,p_tiling_params->gdal_resampling_,p_tiling_params->p_nodata_value_,p_tiling_params->p_background_color_))
			{
				cout<<"Error: BaseZoomTiling: warping to merc fail"<<endl;
				return FALSE;
			}

      if ( p_tiling_params->p_transparent_color_ != NULL ) p_merc_buffer->CreateAlphaBandByRGBColor(p_tiling_params->p_transparent_color_);
			
			if (stretch_to_8bit)
			{
				if (!p_merc_buffer->StretchDataTo8Bit(p_stretch_min_values,p_stretch_max_values))
				{
					cout<<"Error: can't stretch raster values to 8 bit"<<endl;
					return FALSE;
				}
				
			}
            
			if (GMX_MAX_WORK_THREADS == 1)
			{
				if (!GMXMakeTilingFromBuffer(p_tiling_params,
										p_merc_buffer,
										p_bundle,
										curr_min_x,
										curr_min_y,
										zoom,
										tiles_expected,
										&tiles_generated,
										p_tile_container))
				{
					cout<<"Error: BaseZoomTiling: GMXMakeTilingFromBuffer fail"<<endl;
					return FALSE;
				}
				delete(p_merc_buffer);
			}
			else 
			{
				while (GMX_CURR_NUM_OF_THREADS==GMX_MAX_WORK_THREADS)
        {
					Sleep(250);
        }
				if (thread_handle!=NULL) 
				{
					CloseHandle(thread_handle);
					thread_handle = NULL;
          if (tiling_error)
          {
            cout<<"Error: BaseZoomTiling: AsyncMakeTilingFromBuffer fail"<<endl;
            return FALSE;
          }
				}
				if (!(thread_handle =GMXAsyncMakeTilingFromBuffer(	p_tiling_params,
																p_merc_buffer,
																p_bundle,
																curr_min_x,
																curr_min_y,
																zoom,
																tiles_expected,
																&tiles_generated,
																p_tile_container,
																thread_id,
                                &tiling_error)))
				{
					cout<<"Error: BaseZoomTiling: GMXMakeTilingFromBuffer fail"<<endl;
					return FALSE;
				}
			}
*/