#pragma once
//#ifndef IMAGETILLING_RasterFile_H
//#define IMAGETILLING_RasterFile_H

#include "stdafx.h"
#include "RasterBuffer.h"
#include "TileName.h"
#include "VectorBorder.h"
using namespace std;


namespace gmx
{


class RasterFile
{

public:
	RasterFile();
	RasterFile(string raster_file, BOOL is_geo_referenced = TRUE);
	~RasterFile(void);

	BOOL			Init(string raster_file, BOOL is_geo_referenced, double shift_x=0.0, double shift_y=0.0 );
	BOOL			Close();

	void			GetPixelSize (int &width, int &height);
	//void			getGeoReference (double &dULx, double &dULy, double &res);
	//double		get_resolution();

	BOOL			GetSpatialRef(OGRSpatialReference	&ogr_sr); 
	BOOL			GetDefaultSpatialRef (OGRSpatialReference	&ogr_sr, MercatorProjType merc_type);

	GDALDataset*	get_gdal_ds_ref();

	

public: 
	//void			setGeoReference		(double dResolution, double dULx, double dULy);
	BOOL			CalcStatistics	(int &bands, double *&min, double *&max, double *&mean, double *&std_dev);
	BOOL			get_nodata_value		(int *p_nodata_value);


public:
	//void			readMetaData ();
	OGREnvelope*	CalcMercEnvelope (MercatorProjType	merc_type);

	static			BOOL ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference *p_ogr_sr);


protected:
	//void			delete_all();
	OGREnvelope		GetEnvelope ();

protected:
	_TCHAR	buf[256];
	string	raster_file_;
	GDALDataset	*p_gdal_ds_;
	BOOL	is_georeferenced_;
	int		width_;
	int		height_;
	double	resolution_;
	double	ul_x_;
	double	ul_y_;
	int		nodata_value_;
	BOOL	nodata_value_defined_;
	int		num_bands_;
	GDALDataType gdal_data_type_;

};





int _stdcall gmxPrintNoProgress ( double, const char*,void*);

class BundleOfRasterFiles
{
public:
	BundleOfRasterFiles(void);
	~BundleOfRasterFiles(void);
	void Close();

public:
	
	int				Init	(string input_path, MercatorProjType merc_type, string vector_file="", 
							double shift_x = 0.0, double shift_y = 0.0);
	//string			BestImage(double min_x, double min_y, double max_x, double max_y, double &max_intersection);

	OGREnvelope		CalcMercEnvelope();
	int				CalcNumberOfTiles (int zoom);
	int				CalcBestMercZoom();
	BOOL			WarpToMercBuffer (	int zoom,	
                                OGREnvelope	merc_envp, 
                                RasterBuffer *p_buffer,
                                string resampling_alg = "", 
                                int *p_nodata_value = NULL, 
                                BYTE *p_def_color = NULL,
                                string  temp_file_path = "",
                                int srand_seed = 0);

	list<string>	GetFileList();
	list<string>	GetFileListByEnvelope(OGREnvelope merc_envp);
	BOOL			Intersects(OGREnvelope merc_envp);



	//BOOL			createBundleBorder (VectorBorder &border);	
protected:

	BOOL			AddItemToBundle (string raster_file, string	vector_file, double shift_x = 0.0, double shift_y = 0.0);


protected:
	list<pair<string,pair<OGREnvelope*,VectorBorder*>>>	data_list_;
	MercatorProjType		merc_type_;
};


}