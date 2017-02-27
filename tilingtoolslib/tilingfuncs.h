#pragma once

#include "stdafx.h"
#include "tilename.h"
#include "tilecontainer.h"
#include "rasterfile.h"
#include "tilingparameters.h"



bool GMXMakeTiling (	gmx::TilingParameters		*p_tiling_params);


bool GMXMakePyramidFromBaseZoom (	OGREnvelope tiles_envp, 
								int					base_zoom, 
								int					min_zoom, 
								gmx::TilingParameters	*p_tiling_params,
                gmx::ITileMatrixSet  *p_tile_mset,
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
						    gmx::ITileMatrixSet  *p_tile_mset,
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
                unsigned char           *p_background = NULL); 