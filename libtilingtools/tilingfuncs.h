#pragma once

#include "stdafx.h"
#include "tilename.h"
#include "tilecontainer.h"
#include "rasterfile.h"
#include "tilingparameters.h"



bool TTXMakeTiling (ttx::TilingParameters *p_tiling_params);


bool TTXMakePyramidFromBaseZoom (OGREnvelope tiles_envp, 
								int	base_zoom, 
								int	min_zoom, 
								ttx::ITileMatrixSet *p_tile_mset,
            					int	&tiles_expected, 
								int	&tiles_generated, 
								bool only_calculate, 
								ttx::ITileContainer	*p_itile_pyramid,
								bool nearest_resampling = false,
								//int* p_ndv = 0,
								int quality = 0);

bool TTXMakePyramidTileRecursively (OGREnvelope tiles_envp,
								int	zoom,
								int	x,
								int	y,
								int	base_zoom,
								ttx::ITileMatrixSet *p_tile_mset,
								ttx::RasterBuffer& buffer, 
								int& tiles_expected,
								int& tiles_generated,
								bool only_calculate,
								ttx::ITileContainer	*p_itile_pyramid,
								bool* p_was_error,
								bool use_nearest_resampling = false,
								int quality = 0);


bool TTXZoomOutFourIntoOne	(	ttx::RasterBuffer src_quarter_tile_buffers[4], 
								bool src_quarter_tile_buffers_def[4], 
								ttx::RasterBuffer& zoomed_out_tile_buffer,
								bool use_nearest_resampling = false); 