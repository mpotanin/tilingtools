#pragma once
#ifndef GMX_RASTERFILE_H
#define GMX_RASTERFILE_H

#include "stdafx.h"
#include "vectorborder.h"
#include "rasterbuffer.h"
#include "tileName.h"
#include "tilecontainer.h"
#include "tilingparameters.h"
#include "threadfuncs.h"

using namespace std;

namespace gmx
{

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
  RasterFileCutline*  GetRasterFileCutline(ITileMatrixSet *p_tile_mset, string vector_file=""); 
  bool			GetPixelSize (int &width, int &height);
  bool      GetSRS(OGRSpatialReference  &srs, ITileMatrixSet *p_tile_mset = 0);

	GDALDataset*	get_gdal_ds_ref();

  bool			CalcBandStatistics	(int band_num, double &min, double &max, double &mean, double &std, double *p_nodata_val =0);
	double		get_nodata_value		(bool &nodata_defined);

	static			bool ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference &srs);
  static      bool SetBackgroundToGDALDataset (GDALDataset *p_ds, BYTE background[3]); 

protected:
  bool			GetDefaultSpatialRef(OGRSpatialReference	&srs, OGRSpatialReference *p_tiling_srs);


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

struct GMXAsyncChunkTilingParams
{
	gmx::TilingParameters			*p_tiling_params_;
	void            	        *p_bundle_; 
	OGREnvelope               chunk_envp_;
  int								        z_;
	int								        tiles_expected_; 
	int								        *p_tiles_generated_;
  bool                      *p_was_error_;
	gmx::ITileContainer				*p_tile_container_;
  bool                      need_stretching_;
  double		                *p_stretch_min_values_;
  double                    *p_stretch_max_values_;
  int                       tiffinmem_ind_;
  string                    temp_file_path_;

  //bool (*pfCleanAfterTiling)(gmx::RasterBuffer*p_buffer);
};


class BundleTiler
{
public:
	BundleTiler(void);
	~BundleTiler(void);
	void Close();

public:
  int	Init	(map<string,string> raster_vector, ITileMatrixSet* p_tile_mset);

  int CalcNumberOfTiles (int zoom);
	int	CalcAppropriateZoom();
  OGREnvelope	CalcEnvelope();

  bool RunBaseZoomTiling	(	TilingParameters		*p_tiling_params, 
								            ITileContainer			*p_tile_container);
  
  bool ProcessChunk (GMXAsyncChunkTilingParams* p_chunk_params);
  static DWORD WINAPI CallProcessChunk (void *p_params);
  static DWORD WINAPI BundleTiler::CallWarpMulti (void *p_params);
    
protected:
  //ToDo bool      GetRasterProfile(bool &nodata_defined); GDALDataType, nodata_val, colortable
  double  GetNodataValue(bool &nodata_defined);
  GDALDataType  GetRasterFileType();

  bool			WarpChunkToBuffer (	int zoom,	
                                OGREnvelope	chunk_envp, 
                                RasterBuffer *p_dst_buffer,
                                int         output_bands_num = 0,
                                map<string,int*>*   p_band_mapping = 0,
                                GDALResampleAlg resample_alg = GRA_Cubic,
                                BYTE *p_nodata = NULL,
                                BYTE *p_background_color = NULL,
                                int  tiffinmem_ind=0);

  bool RunTilingFromBuffer (TilingParameters	*p_tiling_params, 
						  RasterBuffer	*p_buffer,
              OGREnvelope buffer_envelope,
						  int zoom,
						  int tiles_expected, 
						  int *p_tiles_generated,
						  ITileContainer *p_tile_container);

  bool          CalclLinearStretchTo8BitParams (  double* &p_min_values,
                                                 double*  &p_max_values,
                                                 double*  p_nodata_val=0,
                                                 int      output_bands_num = 0,
                                                 map<string,int*>*  p_band_mapping=0);
  list<string>	GetFileList();
	bool			    Intersects(OGREnvelope envp);
  ITileMatrixSet* tile_matrix_set(){return p_tile_mset_;};

protected:
	bool			AddItemToBundle (string raster_file, string	vector_file);
  bool      CheckStatusAndCloseThreads(list<pair<HANDLE,void*>>* p_thread_list);
  bool      TerminateThreads(list<pair<HANDLE,void*>>* p_thread_list);
  bool      AdjustCutlinesForOverlapping180Degree();

protected:
	list<pair<string,RasterFileCutline*>>	item_list_;
  ITileMatrixSet*  p_tile_mset_;
};

}



bool GMXPrintTilingProgress (int tiles_expected, int tiles_generated);


struct GMXAsyncWarpMultiParams
{
  //gdal_warp_operation.ChunkAndWarpMulti( 0,0,buf_width,buf_height) :
  GDALWarpOperation *p_warp_operation_;
  int               buf_width_;
  int               buf_height_;
  bool              warp_error_;
};

#endif