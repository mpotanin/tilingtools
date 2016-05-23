#pragma once

#include "stdafx.h"
#include "TileName.h"
#include "TileContainer.h"
#include "RasterFile.h"
#include "TilingParameters.h"



bool GMXMakeTiling (	gmx::TilingParameters		*p_tiling_params);


bool GMXMakePyramidFromBaseZoom (	OGREnvelope tiles_envp, 
								int					base_zoom, 
								int					min_zoom, 
								gmx::TilingParameters	*p_tiling_params,
                gmx::ITileGrid  *p_tile_grid,
            		int					&tiles_expected, 
								int					&tiles_generated, 
								bool				only_calculate, 
								gmx::ITileContainer	*p_itile_pyramid
								);

bool GMXMakePyramidTileRecursively (	OGREnvelope tiles_envp,
								int								zoom,
								int								x,
								int								y,
								int								base_zoom,
								gmx::TilingParameters				*p_tiling_params,
						    gmx::ITileGrid  *p_tile_grid,
                gmx::RasterBuffer				&buffer, 
								int								&tiles_expected,
								int								&tiles_generated,
								bool							only_calculate,
								gmx::ITileContainer				*p_itile_pyramid,
                bool              *p_was_error);


bool GMXZoomOutFourIntoOne		          (	gmx::RasterBuffer			src_quarter_tile_buffers[4], 
								bool							      src_quarter_tile_buffers_def[4], 
								gmx::RasterBuffer				&zoomed_out_tile_buffer,
                GDALResampleAlg         resampling_method,
                BYTE                    *p_background = NULL); 