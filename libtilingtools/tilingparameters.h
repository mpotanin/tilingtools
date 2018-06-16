#pragma once

#include "stdafx.h"
#include "tilename.h"


namespace gmx
{

class BundleConsoleInput
{
public:
  BundleConsoleInput()
  {
    bands_num_=0;
  };

  bool  InitByConsoleParams ( list<string> listInputParam, 
                              list<string> listBorderParam, 
                              list<string> listBandParam);

  int   GetBandsNum   () {return bands_num_;};
 
  map<string,int*> GetBandMapping();
  map<string,string> GetFiles();
  list<string> GetRasterFiles();

  ~BundleConsoleInput();

protected:
  void ClearAll();


protected:
  int bands_num_;
  map<string,pair<string,int*>> m_mapInputData;
};


typedef enum { 
  NDEF_CONTAINER_TYPE=-1,
	GMXTILES=0,
	MBTILES=1,
	TILEFOLDER=2
} TileContainerType;


class TilingParameters
{
public:
  TilingParameters()
  {
    jpeg_quality_ = 0;
    p_tile_name_ = NULL;
    p_background_color_	= NULL;
    p_nd_rgbcolors_ = NULL;
    nd_num_ = 0;
    nodata_tolerance_ = 0;
    base_zoom_	= 0;
    min_zoom_	= 0;
    auto_stretching_ = FALSE;

    p_bundle_input_ = 0;

    max_work_threads_= 0;
    tile_chunk_size_= 0;
    
    calculate_histogram_=false;
    clip_offset_ = 0;
  };		
  ~TilingParameters ()
  {
    if (p_background_color_)  delete(p_background_color_);
    if (p_nd_rgbcolors_)
    {
      for (int i=0;i<nd_num_;i++)
        delete(p_nd_rgbcolors_[i]);
      delete(p_nd_rgbcolors_);
    }
    if (p_tile_name_)         delete(p_tile_name_);
  };


public:
  double clip_offset_;
	string output_path_;		//название файла-контейнера тайлов
  TileContainerType container_type_;
  
  gmx::MercatorProjType	merc_type_;				//тип Меркатора
	gmx::TileType tile_type_;				//формат тайлов
	int	base_zoom_;				//максимальный зум (базовый зум)
	int	min_zoom_;				//минимальный зум
	string vector_file_;			//векторная граница
	unsigned char *p_background_color_;		//RGB-цвет для заливки фона в тайлах

  
  unsigned char** p_nd_rgbcolors_; //массив rgb-цветов "нет данных"
  int nd_num_; //количество значений "нет данных"

  int nodata_tolerance_;     //радиус цвета для маски прозрачности
	bool auto_stretching_;		//автоматически пересчитывать значения к 8 бит		
  GDALResampleAlg gdal_resampling_;	      //название фильтра для ресемплинга			

  bool            calculate_histogram_;    //рассчитывать гистограмму

	int							jpeg_quality_;  //уровень сжатия (jpeg, jpeg2000)
	gmx::TileName		*p_tile_name_;			//имена тайлов
  int             max_work_threads_;
  int             tile_chunk_size_;
  gmx::BundleConsoleInput     *p_bundle_input_;
  map<string,string>    options_;
  string user_input_srs_;

};


}