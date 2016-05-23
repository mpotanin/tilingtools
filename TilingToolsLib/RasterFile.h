#pragma once
#ifndef GMX_RASTERFILE_H
#define GMX_RASTERFILE_H

#include "stdafx.h"
#include "VectorBorder.h"
#include "RasterBuffer.h"
#include "TileName.h"
#include "TileContainer.h"
#include "TilingParameters.h"

using namespace std;

namespace gmx
{

extern int	MAX_BUFFER_WIDTH;
extern int	MAX_WORK_THREADS;
extern int	MAX_WARP_THREADS;
extern int	CURR_WORK_THREADS;
extern HANDLE WARP_SEMAPHORE;


class RasterFileCutline
{
public:
  RasterFileCutline()
  {
    tiling_srs_cutline_=0;
    p_pixel_line_cutline_=0;
  };
  ~RasterFileCutline()
  {
    if (tiling_srs_cutline_)
    {
      delete(tiling_srs_cutline_);
      tiling_srs_cutline_=0;
    }
    if (p_pixel_line_cutline_)
    {
      delete(p_pixel_line_cutline_);
      p_pixel_line_cutline_=0;
    }
  };
  bool  Intersects(OGREnvelope envelope)
  {
    if (!tiling_srs_envp_.Intersects(envelope)) return false;
    if (tiling_srs_cutline_) 
      return tiling_srs_cutline_->Intersects(VectorOperations::CreateOGRPolygonByOGREnvelope(envelope));
    else return true;
  };
public:
  OGRMultiPolygon*  tiling_srs_cutline_;
  OGRMultiPolygon*  p_pixel_line_cutline_;
  OGREnvelope tiling_srs_envp_;

};

class RasterFile
{

public:
	
  RasterFile();
	~RasterFile(void);

  bool			      Init(string raster_file); 
  bool			      Close();
  RasterFileCutline*  GetRasterFileCutline(ITileGrid *p_tile_grid, string vector_file=""); 
  bool			GetPixelSize (int &width, int &height);
  bool      GetSRS (OGRSpatialReference  &srs); 
  bool			GetDefaultSpatialRef (OGRSpatialReference	&srs, OGRSpatialReference *p_tiling_srs);

	GDALDataset*	get_gdal_ds_ref();

	

public: 
	bool			CalcBandStatistics	(int band_num, double &min, double &max, double &mean, double &std, double *p_nodata_val =0);
	double		get_nodata_value		(bool &nodata_defined);


public:
	//void			readMetaData ();
	static			bool ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference &srs);
  static      bool SetBackgroundToGDALDataset (GDALDataset *p_ds, BYTE background[3]); 

protected:
	//void			delete_all();
	bool		GetEnvelope (OGREnvelope &envelope); //ToDo - delete

protected:
 	_TCHAR	buf[256];
	string	raster_file_;
	GDALDataset	*p_gdal_ds_;
	double		nodata_value_;
	bool	nodata_value_defined_;
	int		num_bands_;
	GDALDataType gdal_data_type_;
};


int _stdcall gmxPrintNoProgress ( double, const char*,void*);

class BundleTiler
{
public:
	BundleTiler(void);
	~BundleTiler(void);
	void Close();

public:
	
  //ToDo
  //bool      GetRasterProfile(bool &nodata_defined); GDALDataType, nodata_val, colortable

  double  GetNodataValue(bool &nodata_defined);
	int			Init	(list<string> file_list, ITileGrid* p_tile_grid, string vector_file="");

	OGREnvelope		CalcEnvelope();
	int CalcNumberOfTiles (int zoom);
	int	CalcBestMercZoom();

  bool RunBaseZoomTiling	(	TilingParameters		*p_tiling_params, 
								            ITileContainer			*p_tile_container);
  
  bool RunTilingFromBuffer (TilingParameters	*p_tiling_params, 
						  RasterBuffer	*p_buffer,
              OGREnvelope buffer_envelope,
						  int zoom,
						  int tiles_expected, 
						  int *p_tiles_generated,
						  ITileContainer *p_tile_container);

	bool			WarpChunkToBuffer (	int zoom,	
                                OGREnvelope	chunk_envp, 
                                RasterBuffer *p_dst_buffer,
                                int         output_bands_num = 0,
                                int         **pp_band_mapping = NULL,
                                GDALResampleAlg resample_alg = GRA_Cubic,
                                BYTE *p_nodata = NULL,
                                BYTE *p_background_color = NULL,
                                bool  warp_multithread = true);
  
	list<string>	GetFileList();
	bool			    Intersects(OGREnvelope envp);
  bool          CalclValuesForStretchingTo8Bit (  double *&p_min_values,
                                                 double *&p_max_values,
                                                 double *p_nodata_val = 0,
                                                 BandMapping    *p_band_mapping=0);
  GDALDataType  GetRasterFileType();

  ITileGrid* tile_grid(){return p_tile_grid_;};

protected:
	bool			AddItemToBundle (string raster_file, string	vector_file);
  bool      CalcAsyncWarpMulti (GDALWarpOperation* p_warp_operation, int width, int height);
  bool      CheckStatusAndCloseThreads(list<pair<HANDLE,void*>>* p_thread_list);
  bool      TerminateThreads(list<pair<HANDLE,void*>>* p_thread_list);

protected:
	list<pair<string,RasterFileCutline*>>	item_list_;
  ITileGrid*  p_tile_grid_;
};

}


struct GMXAsyncChunkTilingParams
{
	gmx::TilingParameters				*p_tiling_params_;
	gmx::BundleTiler	        *p_bundle_; 
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

  //bool (*pfCleanAfterTiling)(gmx::RasterBuffer*p_buffer);
};

DWORD WINAPI GMXAsyncWarpChunkAndMakeTiling (LPVOID lpParam);

bool GMXPrintTilingProgress (int tiles_expected, int tiles_generated);

DWORD WINAPI GMXAsyncWarpMulti (LPVOID lpParam);
struct GMXAsyncWarpMultiParams
{
  //gdal_warp_operation.ChunkAndWarpMulti( 0,0,buf_width,buf_height) :
  GDALWarpOperation *p_warp_operation_;
  int               buf_width_;
  int               buf_height_;
  bool              warp_error_;
};

#endif