#pragma once
//#ifndef IMAGETILLING_RasterFile_H
//#define IMAGETILLING_RasterFile_H

#include "stdafx.h"
#include "VectorBorder.h"
#include "RasterBuffer.h"
#include "TileName.h"
using namespace std;


namespace gmx
{


class RasterFile
{

public:
	RasterFile();
	RasterFile(string raster_file, bool is_geo_referenced = TRUE);
	~RasterFile(void);

	bool			Init(string raster_file, bool is_geo_referenced, double shift_x=0.0, double shift_y=0.0 );
	bool			Close();

	void			GetPixelSize (int &width, int &height);
	//void			getGeoReference (double &dULx, double &dULy, double &res);
	//double		get_resolution();

	bool			GetSpatialRef(OGRSpatialReference	&ogr_sr); 
	bool			GetDefaultSpatialRef (OGRSpatialReference	&ogr_sr, MercatorProjType merc_type);

	GDALDataset*	get_gdal_ds_ref();

	

public: 
	//void			setGeoReference		(double dResolution, double dULx, double dULy);
	bool			CalcBandStatistics	(int band_num, double &min, double &max, double &mean, double &std, double *p_nodata_val =0);
	double		get_nodata_value		(bool &nodata_defined);


public:
	//void			readMetaData ();
	OGREnvelope*	CalcMercEnvelope (MercatorProjType	merc_type);
  
	static			bool ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference *p_ogr_sr);
  static      bool SetBackgroundToGDALDataset (GDALDataset *p_ds, BYTE background[3]); 


protected:
	//void			delete_all();
	OGREnvelope		GetEnvelope ();

protected:
	_TCHAR	buf[256];
	string	raster_file_;
	GDALDataset	*p_gdal_ds_;
	bool	is_georeferenced_;
	int		width_;
	int		height_;
	double	resolution_;
	double	ul_x_;
	double	ul_y_;
	double		nodata_value_;
	bool	nodata_value_defined_;
	int		num_bands_;
	GDALDataType gdal_data_type_;

};


int _stdcall gmxPrintNoProgress ( double, const char*,void*);

class BandMapping
{
public:
  BandMapping()
  {
    bands_num_=0;
  };
  bool  InitByConsoleParams (string file_param, string bands_param);
  bool  InitLandsat8  (string file_param, string bands_param);
  bool  GetBands      (string file_name, int &bands_num, int *&p_bands); 
  int   GetBandsNum   () {return bands_num_;};
  //ToDo
  list<string>  GetFileList();
  bool  GetBandMappingData (int &output_bands_num, int **&pp_band_mapping);
  ~BandMapping();
  static bool    ParseFileParameter (string str_file_param, list<string> &file_list, int &output_bands_num, int **&pp_band_mapping);

protected:
  bool  AddFile       (string file_name, int *p_bands); 


protected:
  int bands_num_;
  map<string,int*> data_map_;

};

class RasterFileBundle
{
public:
	RasterFileBundle(void);
	~RasterFileBundle(void);
	void Close();

public:
	
  double   GetNodataValue(bool &nodata_defined);
	int				Init	(list<string> file_list, MercatorProjType merc_type, string vector_file="", 
							double shift_x = 0.0, double shift_y = 0.0);
	//string			BestImage(double min_x, double min_y, double max_x, double max_y, double &max_intersection);

	OGREnvelope		CalcMercEnvelope();
	int				CalcNumberOfTiles (int zoom);
	int				CalcBestMercZoom();
	bool			WarpToMercBuffer (	int zoom,	
                                OGREnvelope	merc_envp, 
                                RasterBuffer *p_dst_buffer,
                                int         output_bands_num = 0,
                                int         **pp_band_mapping = NULL,
                                string resampling_alg = "",
                                BYTE *p_nodata = NULL,
                                BYTE *p_background_color = NULL);
                                //string  temp_file_path = "",
                                //int srand_seed = 0);

	list<string>	GetFileList();
	list<string>	GetFileListByEnvelope(OGREnvelope merc_envp);
	bool			    Intersects(OGREnvelope merc_envp);
  bool          CalclValuesForStretchingTo8Bit (  double *&p_min_values,
                                                 double *&p_max_values,
                                                 double *p_nodata_val = 0,
                                                 BandMapping    *p_band_mapping=0);
  GDALDataType  GetRasterFileType();



	//BOOL			createBundleBorder (VectorBorder &border);	
protected:

	bool			AddItemToBundle (string raster_file, string	vector_file, double shift_x = 0.0, double shift_y = 0.0);


protected:
	list<pair<string,pair<OGREnvelope*,VectorBorder*>>>	data_list_;
	MercatorProjType		merc_type_;
};



}