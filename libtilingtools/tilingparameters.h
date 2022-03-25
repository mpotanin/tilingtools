#pragma once

#include "stdafx.h"
#include "tilename.h"


namespace ttx
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
  list<pair<string,string>> GetFiles();
  list<string> GetRasterFiles();

  ~BundleConsoleInput();

protected:
  void ClearAll();


protected:
  int bands_num_;
  list<pair<string,pair<string,int*>>> m_listInputData;
};


typedef enum { 
  NDEF_CONTAINER_TYPE=-1,
	GMXTILES=0,
	MBTILES=1,
	TILEFOLDER=2,
  GTIFF=3
} TileContainerType;


class TilingParameters
{
public:
	TilingParameters()
	{
		quality_ = 0;
		p_tile_name_ = NULL;
		m_bNDVDefined = false;
		m_fNDV = 0;
		m_fNDVTolerance = 0;
		base_zoom_	= 0;
		min_zoom_	= 0;
 
		p_bundle_input_ = 0;

		max_work_threads_= 0;
		tile_chunk_size_= 0;
    
		calculate_histogram_=false;
		clip_offset_ = 0;
		tile_px_size_ = 0;
		pyramid_resampling_ = "";
	};		
	~TilingParameters ()
	{
		if (p_tile_name_) delete(p_tile_name_);
  };


public:
	double clip_offset_;
	string output_path_;		//название файла-контейнера тайлов
	TileContainerType container_type_;
  
	ttx::MercatorProjType	merc_type_;				//тип Меркатора
	ttx::TileType tile_type_;				//формат тайлов
	int	base_zoom_;				//максимальный зум (базовый зум)
	int	min_zoom_;				//минимальный зум
	string vector_file_;			//векторная граница

	bool m_bNDVDefined;		//nodata value defined
	float m_fNDV;				//nodata value
	float m_fNDVTolerance;     //radius for nodata value

	GDALResampleAlg gdal_resampling_;	      //название фильтра для ресемплинга
	string pyramid_resampling_;	 //тип ресемплинга для пирамидных тайлов

	bool calculate_histogram_;    //рассчитывать гистограмму

	int	quality_;  //уровень сжатия (jpeg, jpeg2000)
	ttx::TileName* p_tile_name_;			//имена тайлов
	int max_work_threads_;
	int tile_chunk_size_;
	ttx::BundleConsoleInput* p_bundle_input_;
	map<string,string> options_;
	string user_input_srs_;

	int tile_px_size_;

};


}