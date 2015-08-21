#pragma once

#include "stdafx.h"
#include "TileName.h"
#include "TileContainer.h"
#include "RasterFile.h"


class GMXTilingParameters
{
public:
  GMXTilingParameters(list<string> file_list, gmx::MercatorProjType merc_type, gmx::TileType tile_type);		
  ~GMXTilingParameters ();
  int           CalcOutputBandsNum (gmx::RasterFileBundle *p_bundle);
  GDALDataType  GetOutputDataType (gmx::RasterFileBundle *p_bundle);


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
	bool						auto_stretching_;		//автоматически пересчитывать значения к 8 бит		
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
  gmx::BandMapping     *p_band_mapping_;

  string          temp_file_path_for_warping_;
};


bool GMXMakeTiling (	GMXTilingParameters		*p_tiling_params);

bool GMXMakeBaseZoomTiling (GMXTilingParameters				*p_tiling_params, 
							gmx::RasterFileBundle		*p_bundle, 
							gmx::ITileContainer				*p_tile_container,
              gmx::MetaHistogram            *p_histogram = NULL);

bool GMXMakePyramidFromBaseZoom (	gmx::VectorBorder	&vb, 
								int					base_zoom, 
								int					min_zoom, 
								GMXTilingParameters	*p_tiling_params, 
								int					&tiles_expected, 
								int					&tiles_generated, 
								bool				only_calculate, 
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
  bool                      *p_was_error_;
	gmx::ITileContainer				*p_tile_container_;
  bool                      need_stretching_;
  double		                *p_stretch_min_values_;
  double                    *p_stretch_max_values_;
  int                       srand_seed_;
  string                    temp_file_path_;
  gmx::MetaHistogram            *p_histogram_;

  //bool (*pfCleanAfterTiling)(gmx::RasterBuffer*p_buffer);
};

DWORD WINAPI GMXAsyncWarpChunkAndMakeTiling (LPVOID lpParam);

bool GMXPrintTilingProgress (int tiles_expected, int tiles_generated);

bool GMXMakeTilingFromBuffer	(	GMXTilingParameters				*p_tiling_params,
								gmx::RasterBuffer				*p_buffer, 
								gmx::RasterFileBundle		*p_bundle, 
								int								ul_x, 
								int								ul_y,
								int								z,
								int								tiles_expected, 
								int								*p_tiles_generated,
								gmx::ITileContainer				*p_tile_container
								);

bool GMXMakePyramidTileRecursively (	gmx::VectorBorder				&vb,
								int								zoom,
								int								x,
								int								y,
								int								base_zoom,
								int								min_zoom,
								GMXTilingParameters				*p_tiling_params,
								gmx::RasterBuffer				&buffer, 
								int								&tiles_expected,
								int								&tiles_generated,
								bool							only_calculate,
								gmx::ITileContainer				*p_itile_pyramid,
                bool              *p_was_error);


bool GMXZoomOutTileBuffer		(	gmx::RasterBuffer			src_quarter_tile_buffers[4], 
								bool							src_quarter_tile_buffers_def[4], 
								gmx::RasterBuffer				&zoomed_out_tile_buffer,
                string      resampling_method="",
                BYTE   *p_background = NULL); 