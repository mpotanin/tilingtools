#pragma once

#include "stdafx.h"
#include "TileName.h"
#include "TileContainer.h"
#include "RasterFile.h"


class GMXTilingParameters
{
public:
//обязательные параметры
	list<string>          file_list_;
	gmx::MercatorProjType	merc_type_;				//тип Меркатора
	gmx::TileType				  tile_type_;				//формат тайлов
	

//дополнительные параметры
	int							base_zoom_;				//максимальный зум (базовый зум)
	int							min_zoom_;				//минимальный зум
	string					vector_file_;			//векторная граница
	string					container_file_;		//название файла-контейнера тайлов
	bool						use_container_;			//писать тайлы в контейнер
	BYTE						*p_background_color_;		//RGB-цвет для заливки фона в тайлах
	BYTE						*p_transparent_color_;		//RGB-цвет для маски прозрачности
	int             nodata_tolerance_;     //радиус цвета для маски прозрачности
	bool						auto_stretch_to_8bit_;		//автоматически пересчитывать значения к 8 бит		
  string          gdal_resampling_;	      //название фильтра для ресемплинга			

  bool            calculate_histogram_;    //рассчитывать гистограмму

	int							jpeg_quality_;  //уровень сжатия (jpeg, jpeg2000)
	double					shift_x_;				//сдвиг по x
	double					shift_y_;				//сдвиг по y
	gmx::TileName		*p_tile_name_;			//имена тайлов
	unsigned int		max_cache_size_;		//максимальный размер тайлового кэша в оперативной памяти
  int             max_work_threads_;
  int             max_warp_threads_;
  unsigned int    max_gmx_volume_size_; //максимальный размер файла в gmx-котейнере 
  int             bands_num_;
  int             *p_bands_;
  int             **pp_band_mapping_;

  string          temp_file_path_for_warping_;
	//static const int			DEFAULT_JPEG_QUALITY = 85;


	GMXTilingParameters(list<string> file_list, gmx::MercatorProjType merc_type, gmx::TileType tile_type)
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
    auto_stretch_to_8bit_ = FALSE;

    bands_num_ = 0;
    p_bands_ = NULL;
    pp_band_mapping_ = 0;

    max_work_threads_= 0;
    max_warp_threads_= 0;
    
    temp_file_path_for_warping_ = "";
    calculate_histogram_=false;
	}

		
	~GMXTilingParameters ()
	{
    if (p_background_color_)  delete(p_background_color_);
		if (p_transparent_color_) delete(p_transparent_color_);
    if (p_bands_)               delete(p_bands_);
    if (p_tile_name_)         delete(p_tile_name_);
	}
};


BOOL GMXMakeTiling (	GMXTilingParameters		*p_tiling_params);

BOOL GMXMakeBaseZoomTiling (GMXTilingParameters				*p_tiling_params, 
							gmx::RasterFileBundle		*p_bundle, 
							gmx::ITileContainer				*p_tile_container,
              gmx::MetaHistogram            *p_histogram = NULL);

BOOL GMXMakePyramidFromBaseZoom (	gmx::VectorBorder	&vb, 
								int					base_zoom, 
								int					min_zoom, 
								GMXTilingParameters	*p_tiling_params, 
								int					&tiles_expected, 
								int					&tiles_generated, 
								BOOL				only_calculate, 
								gmx::ITileContainer	*p_itile_pyramid
								);

struct GMXAsyncChunkTilingParams
{
	GMXTilingParameters				*p_tiling_params_;
	gmx::RasterFileBundle	*p_bundle_; 
	OGREnvelope               chunk_envp_;
  int								        z_;
	int								        tiles_expected_; 
	int								        *p_tiles_generated_;
  BOOL                      *p_was_error_;
	gmx::ITileContainer				*p_tile_container_;
  BOOL                      stretch_to_8bit_;
  double		                *p_stretch_min_values_;
  double                    *p_stretch_max_values_;
  int                       srand_seed_;
  string                    temp_file_path_;
  gmx::MetaHistogram            *p_histogram_;

  //BOOL (*pfCleanAfterTiling)(gmx::RasterBuffer*p_buffer);
};

DWORD WINAPI GMXAsyncWarpChunkAndMakeTiling (LPVOID lpParam);

BOOL GMXPrintTilingProgress (int tiles_expected, int tiles_generated);

BOOL GMXMakeTilingFromBuffer	(	GMXTilingParameters				*p_tiling_params,
								gmx::RasterBuffer				*p_buffer, 
								gmx::RasterFileBundle		*p_bundle, 
								int								ul_x, 
								int								ul_y,
								int								z,
								int								tiles_expected, 
								int								*p_tiles_generated,
								gmx::ITileContainer				*p_tile_container
								);

BOOL GMXMakePyramidTileRecursively (	gmx::VectorBorder				&vb,
								int								zoom,
								int								x,
								int								y,
								int								base_zoom,
								int								min_zoom,
								GMXTilingParameters				*p_tiling_params,
								gmx::RasterBuffer				&buffer, 
								int								&tiles_expected,
								int								&tiles_generated,
								BOOL							only_calculate,
								gmx::ITileContainer				*p_itile_pyramid,
                BOOL              *p_was_error);


BOOL GMXZoomOutTileBuffer		(	gmx::RasterBuffer			src_quarter_tile_buffers[4], 
								BOOL							src_quarter_tile_buffers_def[4], 
								gmx::RasterBuffer				&zoomed_out_tile_buffer,
                BYTE   *p_background = NULL); 