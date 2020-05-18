#include "tilingfuncs.h"

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
	long t_ = time(0);
	srand(t_%10000);
	BundleTiler		raster_bundle;

	if (p_tiling_params->tile_px_size_ != 0)
		MercatorTileMatrixSet::TILE_PX_SIZE_ = p_tiling_params->tile_px_size_;
	MercatorTileMatrixSet merc_grid( p_tiling_params->merc_type_);

	if (!raster_bundle.Init(p_tiling_params->p_bundle_input_->GetFiles(),
                          &merc_grid,
                          p_tiling_params->user_input_srs_,
                          p_tiling_params->clip_offset_))
	{
		cout<<"ERROR: can't init raster bundle object"<<endl;
		return FALSE;
	}


	int base_zoom = (p_tiling_params->base_zoom_ == 0) ? raster_bundle.CalcAppropriateZoom() 
														: p_tiling_params->base_zoom_;
	if (base_zoom<=0)
	{
		cout<<"ERROR: can't calculate base zoom for tiling"<<endl;
		return FALSE;
	}

	TileContainerOptions tc_params;
	tc_params.path_ = p_tiling_params->output_path_;
	tc_params.p_tile_name_ = p_tiling_params->p_tile_name_;
	tc_params.max_zoom_=base_zoom;
	tc_params.tiling_srs_envp_=raster_bundle.CalcEnvelope();
	tc_params.extra_options_=p_tiling_params->options_;
	tc_params.tile_type_=p_tiling_params->tile_type_;
	tc_params.p_matrix_set_=&merc_grid;
	ITileContainer* p_itile_pyramid = 0;
	if (!(p_itile_pyramid = TileContainerFactory::OpenForWriting(p_tiling_params->container_type_, &tc_params)))
	{
		cout<<"ERROR: can't open for writing tile container"<<endl;
		return false;
	}
 
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

			bool ndv_from_input_defined = false;
			int ndv_val = raster_bundle.GetNodataValue(ndv_from_input_defined);


			GMXMakePyramidFromBaseZoom(raster_bundle.CalcEnvelope(),
								    base_zoom,
								    min_zoom,
								   	&merc_grid,
								    tiles_expected,
								    tiles_generated,
								    TRUE,
								    p_itile_pyramid,
									p_tiling_params->gdal_resampling_==GRA_NearestNeighbour,
									ndv_from_input_defined ? &ndv_val : 0,
									p_tiling_params->quality_,
									p_tiling_params->p_background_color_);
			cout<<tiles_expected<<endl;
			if (tiles_expected > 0) 
			{
				cout<<"0% ";
				if ((no_run_tiling_error *= GMXMakePyramidFromBaseZoom(	raster_bundle.CalcEnvelope(),
											base_zoom,
											min_zoom,
											&merc_grid,
											tiles_expected,
											tiles_generated,
											FALSE,
											p_itile_pyramid,
											p_tiling_params->gdal_resampling_ == GRA_NearestNeighbour,
											ndv_from_input_defined ? &ndv_val : 0,
											p_tiling_params->quality_,
											p_tiling_params->p_background_color_))) 
					cout<<" done."<<endl;
			}
		}
	}

	if (no_run_tiling_error)
		no_run_tiling_error*=p_itile_pyramid->ExtractAndStoreMetadata(p_tiling_params);

	if (!p_itile_pyramid->Close())
		cout<<"ERROR: can't save tile container to disk"<<endl;
	delete(p_itile_pyramid);

	return no_run_tiling_error;
}

//ToDO:
//  - exclude p_tiling_params
//  - add background
//  - add nodata val defined from input rasters
//  - add resampling algorithm

bool GMXMakePyramidFromBaseZoom (OGREnvelope tiles_envp, 
								int	base_zoom, 
								int min_zoom, 
								gmx::ITileMatrixSet* p_tile_mset,
								int	&tiles_expected, 
								int	&tiles_generated, 
								bool only_calculate, 
								ITileContainer* p_itile_pyramid,
								bool nearest_resampling,
								int* p_ndv,
								int quality,
								unsigned char* p_background_color)
{

	RasterBuffer oBuffer;
	bool was_error = FALSE;
	bool b;

	int min_x,min_y,max_x,max_y;
	p_tile_mset->CalcTileRange(tiles_envp,min_zoom,min_x,min_y,max_x,max_y);
	for (int x=min_x;x<=max_x;x++)
	{
		for (int y=min_y;y<=max_y;y++)
			GMXMakePyramidTileRecursively(tiles_envp,min_zoom,x,y,base_zoom,
											p_tile_mset,oBuffer,tiles_expected,tiles_generated,
											only_calculate,p_itile_pyramid,&was_error,
											nearest_resampling,p_ndv,quality,p_background_color);
	}

	if (was_error)
	{
		cout<<"ERROR: GMXMakePyramidTileRecursively"<<endl;
		return FALSE;
	}
	else return TRUE;
}


bool GMXMakePyramidTileRecursively (OGREnvelope tiles_envp, 
									int	zoom, 
									int	nX, 
									int	nY, 
									int	base_zoom, 
									gmx::ITileMatrixSet* p_tile_mset,
									RasterBuffer &tile_buffer,  
									int	&tiles_expected, 
									int	&tiles_generated, 
									bool only_calculate, 
									ITileContainer* p_itile_pyramid, 
									bool* p_was_error,
									bool use_nearest_resampling,
									int* p_ndv,
									int quality,
									unsigned char* p_background_color
									)
{

	if (zoom>base_zoom) return false;
	else if (zoom==base_zoom)
	{	
		if (!p_itile_pyramid->TileExists(zoom,nX,nY)) return FALSE;
		if (only_calculate) return TRUE;

		unsigned int size	= 0;
		unsigned char* p_data	= NULL;
	
		p_itile_pyramid->GetTile(zoom,nX,nY,p_data,size);
		if (size ==0) return FALSE;

		if (!tile_buffer.CreateBufferFromInMemoryData(p_data, size, p_itile_pyramid->GetTileType()))
		{
			cout << "ERROR: reading tile data" << endl;
			return FALSE;
		}

		if (p_ndv) tile_buffer.CreateAlphaBandByNDV(p_ndv[0]);
		
		//p_tiling_params->

   		delete[]((unsigned char*)p_data);
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
				if (tiles_envp.Intersects(p_tile_mset->CalcEnvelopeByTile(zoom+1,2*nX+j,2*nY+i)))
				{
					src_quarter_tile_buffers_def[i*2+j] = GMXMakePyramidTileRecursively(tiles_envp,
															zoom+1,
															2*nX+j,
															2*nY+i,
															base_zoom,
															p_tile_mset,
															quarter_tile_buffer[i*2+j],
															tiles_expected,
															tiles_generated,
															only_calculate,
															p_itile_pyramid,
															p_was_error,
															use_nearest_resampling,
															p_ndv,
															quality,
															p_background_color);
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
                              use_nearest_resampling,
                              p_background_color)) return FALSE;
		void *p_data=NULL;
		int size = 0;
		tile_buffer.SerializeToInMemoryData(p_data, size, p_itile_pyramid->GetTileType(), 
											quality);
		
		(*p_was_error) =  (!p_itile_pyramid->AddTile(zoom,nX,nY,(unsigned char*)p_data,size));
		delete[]((unsigned char*)p_data);
		if (*p_was_error) return FALSE;
		tiles_generated++;
		GMXPrintTilingProgress(tiles_expected,tiles_generated);
	}

	return TRUE;
}





bool GMXZoomOutFourIntoOne ( RasterBuffer src_quarter_tile_buffers[4], 
                            bool src_quarter_tile_buffers_def[4], 
                            RasterBuffer& zoomed_out_tile_buffer, 
							bool use_nearest_resampling, 
                            unsigned char* p_background) 
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
			void *p_zoomed_data = src_quarter_tile_buffers[n].ZoomOut(use_nearest_resampling);
			zoomed_out_tile_buffer.SetPixelDataBlock(	(n%2)*tile_size/2,
									(n/2)*tile_size/2,
									tile_size/2,
									tile_size/2,
									p_zoomed_data,
                  0,
                  src_quarter_tile_buffers[n].get_num_bands()-1);
			delete[]((unsigned char*)p_zoomed_data);
		}
	}

	return TRUE;
}

