#include "stdafx.h"
#include "TilingFuncs.h"

using namespace gmx;

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


bool GMXMakeTiling		(TilingParameters		*p_tiling_params)
{
	LONG t_ = time(0);
	srand(t_%10000);
	BundleTiler		raster_bundle;
  MercatorTileGrid merc_grid( p_tiling_params->merc_type_);

  if (!raster_bundle.Init(p_tiling_params->file_list_,&merc_grid,p_tiling_params->vector_file_))
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

  unsigned int max_gmx_volume_size   = p_tiling_params->max_gmx_volume_size_ > 0 ? p_tiling_params->max_gmx_volume_size_ 
                                                                                  : GMXTileContainer::DEFAULT_MAX_VOLUME_SIZE;
  
  ITileContainer	*p_itile_pyramid =	NULL;
 
    
  // initializing p_itile_pyramid - depends on specified 
  // output format of tile pyramid: .gmxtiles, .mbtiles, tile folder etc.
  if (  p_tiling_params->use_container_ && 
        ( GetExtension(p_tiling_params->container_file_) == "tiles" || 
          GetExtension(p_tiling_params->container_file_) == "gmxtiles")
      )
	{
    p_itile_pyramid = new GMXTileContainer();
    
    if ( !((GMXTileContainer*)p_itile_pyramid)->OpenForWriting(p_tiling_params->container_file_,
														                                  p_tiling_params->tile_type_,
														                                  p_tiling_params->merc_type_,
														                                  raster_bundle.CalcEnvelope(),
														                                  base_zoom,
														                                  TRUE,
                                                              NULL,
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
		raster_bundle.CalcEnvelope());
  }
	else 
    p_itile_pyramid = new TileFolder(p_tiling_params->p_tile_name_,TRUE);
  
  bool no_run_tiling_error = true;
  cout<<"Base zoom "<<base_zoom<<": ";
	if ((no_run_tiling_error=raster_bundle.RunBaseZoomTiling( p_tiling_params,
                              p_itile_pyramid))) 
  {
    cout<<" done."<<endl;
    
    int min_zoom = (p_tiling_params->min_zoom_ <=0) ? 1 : p_tiling_params->min_zoom_;
	  if (min_zoom < base_zoom)
    {
	    cout<<"Pyramid tiles: ";//<<endl;
	    cout<<"calculating number of tiles: ";
      int tiles_generated = 0;
	    int tiles_expected = 0;
      GMXMakePyramidFromBaseZoom(raster_bundle.CalcEnvelope(),
								    base_zoom,
								    min_zoom,
								    p_tiling_params,
                    &merc_grid,
								    tiles_expected,
								    tiles_generated,
								    TRUE,
								    p_itile_pyramid);
	    cout<<tiles_expected<<endl;
	    if (tiles_expected > 0) 
	    {
		    cout<<"0% ";
		    if ((no_run_tiling_error *= GMXMakePyramidFromBaseZoom(	raster_bundle.CalcEnvelope(),
									    base_zoom,
									    min_zoom,
									    p_tiling_params,
									    &merc_grid,
                      tiles_expected,
									    tiles_generated,
									    FALSE,
									    p_itile_pyramid))) cout<<" done."<<endl;
	    }
    }
  }

  if (no_run_tiling_error)
    no_run_tiling_error*=p_itile_pyramid->ExtractAndStoreMetadata(p_tiling_params);

  if (!p_itile_pyramid->Close())
     cout<<"Error: can't save tile container to disk"<<endl;
  delete(p_itile_pyramid);

	return no_run_tiling_error;
}



bool GMXMakePyramidFromBaseZoom (	OGREnvelope tiles_envp, 
								int					base_zoom, 
								int					min_zoom, 
								TilingParameters		*p_tiling_params, 
                gmx::ITileGrid  *p_tile_grid,
								int					&tiles_expected, 
								int					&tiles_generated, 
								bool					only_calculate, 
								ITileContainer			*p_itile_pyramid
								)
{

	RasterBuffer oBuffer;
  bool was_error = FALSE;
	bool b;

  int min_x,min_y,max_x,max_y;
  p_tile_grid->CalcTileRange(tiles_envp,min_zoom,min_x,min_y,max_x,max_y);
  for (int x=min_x;x<=max_x;x++)
  {
    for (int y=min_y;y<=max_y;y++)
      GMXMakePyramidTileRecursively(tiles_envp,min_zoom,x,y,base_zoom,p_tiling_params,p_tile_grid,oBuffer,tiles_expected,
							  tiles_generated,only_calculate,p_itile_pyramid,&was_error);
  }

  if (was_error)
  {
    cout<<"Error: GMXMakePyramidTileRecursively"<<endl;
    return FALSE;
  }
  else return TRUE;
}


bool GMXMakePyramidTileRecursively (OGREnvelope tiles_envp, 
					  int				zoom, 
					  int				nX, 
					  int				nY, 
					  int				base_zoom, 
					  TilingParameters	*p_tiling_params, 
			      gmx::ITileGrid  *p_tile_grid,
            RasterBuffer		&tile_buffer,  
					  int				&tiles_expected, 
					  int				&tiles_generated, 
					  bool				only_calculate, 
					  ITileContainer		*p_itile_pyramid, 
            bool      *p_was_error
            )
{

  if (zoom>base_zoom) return false;
  else if (zoom==base_zoom)
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
          tile_buffer.CreateFromJP2Data(p_data,size);
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
        if (tiles_envp.Intersects(p_tile_grid->CalcEnvelopeByTile(zoom+1,2*nX+j,2*nY+i)))
        {
					src_quarter_tile_buffers_def[i*2+j] = GMXMakePyramidTileRecursively(tiles_envp,
															zoom+1,
															2*nX+j,
															2*nY+i,
															base_zoom,
															p_tiling_params,
															p_tile_grid,
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
		if ((!src_quarter_tile_buffers_def[0])&&
			(!src_quarter_tile_buffers_def[1])&&
			(!src_quarter_tile_buffers_def[2])&&
			(!src_quarter_tile_buffers_def[3]) )	return FALSE;
	
		if (only_calculate)
		{
     	tiles_expected++;
			return TRUE;
		}

    if (!GMXZoomOutFourIntoOne(quarter_tile_buffer,
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





bool GMXZoomOutFourIntoOne ( RasterBuffer src_quarter_tile_buffers[4], 
                            bool src_quarter_tile_buffers_def[4], 
                            RasterBuffer &zoomed_out_tile_buffer, 
                            GDALResampleAlg resampling_method,
                            BYTE *p_background) 
{
	int i;
  int tile_size;
	for (i = 0; i<4;i++)
    if (src_quarter_tile_buffers_def[i]) break;
	if (i==4) return FALSE;
  else tile_size = src_quarter_tile_buffers[i].get_x_size(); 
	
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
			void *p_zoomed_data = src_quarter_tile_buffers[n].ZoomOut(resampling_method);
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

