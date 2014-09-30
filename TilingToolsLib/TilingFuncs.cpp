#include "stdafx.h"
#include "TilingFuncs.h"

using namespace gmx;

int	GMX_MAX_BUFFER_WIDTH	= 16;

int			GMX_MAX_WORK_THREADS	= 2;
int			GMX_MAX_WARP_THREADS	= 1;
int			GMX_CURR_WORK_THREADS	= 0;
HANDLE  GMX_WARP_SEMAPHORE = NULL;





BOOL GMXPrintTilingProgress (int tiles_expected, int tiles_generated)
{
	//cout<<tiles_generated<<" "<<endl;
	if (tiles_generated - (int)ceil((tiles_expected/10.0)*(tiles_generated*10/tiles_expected))  ==0)
	{
		cout<<(tiles_generated*10/tiles_expected)*10<<" ";
		fflush(stdout);
	}
	return TRUE;
}


BOOL GMXMakeTiling		(GMXTilingParameters		*p_tiling_params)
{
	LONG t_ = time(0);
	srand(t_%10000);
	RasterFileBundle		raster_bundle;
	int tile_size = MercatorTileGrid::TILE_SIZE;
	//p_tiling_params->jpeg_quality_ = JPEG_BASE_ZOOM_QUALITY;
	
	if (!raster_bundle.Init(p_tiling_params->input_path_,p_tiling_params->merc_type_,p_tiling_params->vector_file_,p_tiling_params->shift_x_,p_tiling_params->shift_y_))
	{
		cout<<"Error: read input data by path: "<<p_tiling_params->input_path_<<endl;
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

  BOOL nodata_defined = false;
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
     
    RasterFile rf(*raster_bundle.GetFileList().begin(),1);
    if (!rf.get_gdal_ds_ref()) 
    {
      cout<<"Error: can't init. gdaldataset from file: "<<(*raster_bundle.GetFileList().begin())<<endl;
      return FALSE;
    }

    GDALDataType input_type = rf.get_gdal_ds_ref()->GetRasterBand(1)->GetRasterDataType();
    int input_num_bands = rf.get_gdal_ds_ref()->GetRasterCount();

    GDALDataType output_type = (p_tiling_params->tile_type_ == JPEG_TILE || 
                                p_tiling_params->tile_type_ == PNG_TILE) ?  GDT_Byte : input_type;

    int output_num_bands;
    if (p_tiling_params->tile_type_ == JPEG_TILE) 
      output_num_bands = min(3,input_num_bands);
    else if (p_tiling_params->tile_type_ == PNG_TILE) 
      output_num_bands = min(3,input_num_bands);
    else 
      output_num_bands =  (p_tiling_params->bands_num_ != 0) ? 
                          min(input_num_bands,p_tiling_params->bands_num_) :
                          input_num_bands;

    if (nodata_defined) metadata.AddTagRef(&nodata_value);
    histogram.Init(output_num_bands,output_type);
    metadata.AddTagRef(&histogram);
    hist_stat.Init(output_num_bands);
    metadata.AddTagRef(&hist_stat);
      
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

	if (!GMXMakeBaseZoomTiling(p_tiling_params,&raster_bundle,p_itile_pyramid,&histogram)) 
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


BOOL GMXMakeTilingFromBuffer (GMXTilingParameters			*p_tiling_params, 
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
  BOOL warp_result = p_bundle->WarpToMercBuffer(zoom,
                                                chunk_envp,
                                                p_merc_buffer,
                                                p_tiling_params->bands_num_,
                                                p_tiling_params->p_bands_,
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

  if (p_chunk_tiling_params->stretch_to_8bit_)	
  {
    BOOL stretch_result = false;
    if (p_tiling_params->bands_num_ == 0)
    {
      stretch_result = p_merc_buffer->StretchDataTo8Bit (
                            p_chunk_tiling_params->p_stretch_min_values_,
                            p_chunk_tiling_params->p_stretch_max_values_);
    }
    else
    {
      double *p_stretch_min_values = new double[p_tiling_params->bands_num_];
      double *p_stretch_max_values = new double[p_tiling_params->bands_num_];
      for (int i=0; i<p_tiling_params->bands_num_;i++)
      {
        p_stretch_min_values[i] = p_chunk_tiling_params->
                        p_stretch_min_values_[p_tiling_params->p_bands_[i]-1];
        p_stretch_max_values[i] = p_chunk_tiling_params->
                        p_stretch_max_values_[p_tiling_params->p_bands_[i]-1];
      }
      stretch_result = p_merc_buffer->StretchDataTo8Bit ( p_stretch_min_values,
                                                          p_stretch_max_values);
      delete[]p_stretch_min_values;
      delete[]p_stretch_max_values;
    }
		if (!stretch_result)
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


BOOL GMXMakeBaseZoomTiling	(	GMXTilingParameters		*p_tiling_params, 
								RasterFileBundle		*p_bundle, 
								ITileContainer			*p_tile_container,
                MetaHistogram           *p_histogram)
{

  //ToDo
  srand(0);
	int			tiles_generated = 0;
	//OGREnvelope bundleMercEnvelope = p_bundle->GetMercatorEnvelope();
	int			zoom = (p_tiling_params->base_zoom_ == 0) ? p_bundle->CalcBestMercZoom() : p_tiling_params->base_zoom_;
	double		res = MercatorTileGrid::CalcResolutionByZoom(zoom);

  cout<<"calculating number of tiles: ";
	int tiles_expected	= p_bundle->CalcNumberOfTiles(zoom);	
	cout<<tiles_expected<<endl;
  if (tiles_expected == 0) return FALSE;
  
	bool		stretch_to_8bit = false;
	double		*p_stretch_min_values = NULL, *p_stretch_max_values = NULL;

  GMX_WARP_SEMAPHORE = CreateSemaphore(NULL,GMX_MAX_WARP_THREADS,GMX_MAX_WARP_THREADS,NULL);

	if (p_tiling_params->auto_stretch_to_8bit_)
	{
		RasterFile rf((*p_bundle->GetFileList().begin()),TRUE);
		double *p_min,*p_max,*p_mean,*p_std_dev;
		int bands;
		if ( (p_tiling_params->tile_type_ == JPEG_TILE || p_tiling_params->tile_type_ == PNG_TILE) && 
			 (rf.get_gdal_ds_ref()->GetRasterBand(1)->GetRasterDataType() != GDT_Byte))
		{
			stretch_to_8bit = true;
			cout<<"WARNING: input raster doesn't match 8 bit/band. Auto stretching to 8 bit will be performed"<<endl;

			if (!rf.CalcStatistics(bands,p_min,p_max,p_mean,p_std_dev))
			{
				cout<<"Error: compute statistics failed for "<<(*p_bundle->GetFileList().begin())<<endl;
				return false;
			}
			p_stretch_min_values = new double[bands];
			p_stretch_max_values = new double[bands];
			for (int i=0;i<bands;i++)
			{
				p_stretch_min_values[i] = p_mean[i] - 2*p_std_dev[i];
				p_stretch_max_values[i] = p_mean[i] + 2*p_std_dev[i];
			}
			delete[]p_min;delete[]p_max;delete[]p_mean;delete[]p_std_dev;
		}
	}

	cout<<"0% ";
	fflush(stdout);

	//if (p_bundle->getFileList().size()>1) GMX_MAX_WORK_THREADS = 1;

	int minx,maxx,miny,maxy;
	MercatorTileGrid::CalcTileRange(p_bundle->CalcMercEnvelope(),zoom,minx,miny,maxx,maxy);

	HANDLE			thread_handle = NULL;
	unsigned long	thread_id;

  //int num_warp = 0;
  
  BOOL tiling_error = FALSE;
  int srand_seed = 0;


  /*
  int max_warp_threads;
  if (p_tiling_params->temp_file_path_for_warping_ != "")
    max_warp_threads = (p_tiling_params->max_warp_threads_ == 0) ? 2 : p_tiling_params->max_warp_threads_;
  else 
    max_warp_threads = 1;
  */

  //int max_work_threads = (p_tiling_params->max_work_threads_ == 0) ? max_warp_threads + 1 : (int)max(p_tiling_params->max_work_threads_,max_warp_threads + 1);

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
	    while (GMX_CURR_WORK_THREADS >= GMX_MAX_WORK_THREADS)        Sleep(250);
      
      GMXAsyncChunkTilingParams	*p_chunk_tiling_params = new  GMXAsyncChunkTilingParams();
      
      p_chunk_tiling_params->p_tiling_params_ = p_tiling_params;
      p_chunk_tiling_params->chunk_envp_ = chunk_envp;
      p_chunk_tiling_params->p_bundle_ = p_bundle;
      p_chunk_tiling_params->p_tile_container_ = p_tile_container;
      p_chunk_tiling_params->p_tiles_generated_ = &tiles_generated;
      p_chunk_tiling_params->tiles_expected_ = tiles_expected;
      p_chunk_tiling_params->stretch_to_8bit_ = stretch_to_8bit;
      p_chunk_tiling_params->p_stretch_min_values_ = p_stretch_min_values;
      p_chunk_tiling_params->p_stretch_max_values_ = p_stretch_max_values;
      p_chunk_tiling_params->z_ = zoom;
      p_chunk_tiling_params->p_histogram_ = p_histogram;
    	CreateThread(NULL,0,GMXAsyncWarpChunkAndMakeTiling,p_chunk_tiling_params,0,&thread_id);      Sleep(100);    }
	}

  while (GMX_CURR_WORK_THREADS > 0)
    Sleep(250);
	
  if (GMX_WARP_SEMAPHORE)
    CloseHandle(GMX_WARP_SEMAPHORE);

	return TRUE;
}


BOOL GMXMakePyramidFromBaseZoom (	VectorBorder		&vb, 
								int					base_zoom, 
								int					min_zoom, 
								GMXTilingParameters		*p_tiling_params, 
								int					&tiles_expected, 
								int					&tiles_generated, 
								BOOL					only_calculate, 
								ITileContainer			*p_itile_pyramid
								)
{

	RasterBuffer oBuffer;
  BOOL was_error = FALSE;
	BOOL b;

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


BOOL GMXMakePyramidTileRecursively (VectorBorder	&vb, 
					  int				zoom, 
					  int				nX, 
					  int				nY, 
					  int				base_zoom, 
					  int				min_zoom, 
					  GMXTilingParameters	*p_tiling_params, 
					  RasterBuffer		&tile_buffer,  
					  int				&tiles_expected, 
					  int				&tiles_generated, 
					  BOOL				only_calculate, 
					  ITileContainer		*p_itile_pyramid, 
            BOOL      *p_was_error
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
		BOOL			src_quarter_tile_buffers_def[4];

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

    if (!GMXZoomOutTileBuffer(quarter_tile_buffer,src_quarter_tile_buffers_def, tile_buffer,p_tiling_params->p_background_color_)) return FALSE;
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





BOOL GMXZoomOutTileBuffer ( RasterBuffer src_quarter_tile_buffers[4], BOOL src_quarter_tile_buffers_def[4], 
                            RasterBuffer &zoomed_out_tile_buffer, BYTE *p_background) 
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
									src_quarter_tile_buffers[i].IsAlphaBand());
  if (p_background && src_quarter_tile_buffers[i].get_data_type() == GDT_Byte)
    zoomed_out_tile_buffer.InitByRGBColor(p_background);

  for (int n=0;n<4;n++)
	{
		if (src_quarter_tile_buffers_def[n])
		{
			void *p_zoomed_data = src_quarter_tile_buffers[n].GetDataZoomedOut();
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
BOOL GMXCleanAfterTilingFromBufer (gmx::RasterBuffer				*p_buffer)
{
	delete(p_buffer);
	return TRUE;
}


DWORD WINAPI GMXCallTilingFromBuffer( LPVOID lpParam )
{
	GMXTilingFromBufferParams	*p_from_buffer_params = (GMXTilingFromBufferParams*)lpParam;
	//ToDo
	//Обработка ошибки
	BOOL succeed = GMXMakeTilingFromBuffer(p_from_buffer_params->p_tiling_params_,
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
                BOOL                *p_was_error)
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