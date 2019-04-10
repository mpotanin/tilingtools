#pragma once
#include "stdafx.h"
#include "vectorborder.h"
#include "rasterbuffer.h"
#include "tilename.h"
#include "tilecontainer.h"
#include "tilingparameters.h"

using namespace std;

int GMXPrintProgressStub(double, const char*, void*);


namespace gmx
{

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

  bool			      Init(string raster_file, string set_srs = ""); 
  bool			      Close();
  RasterFileCutline*  GetRasterFileCutline(ITileMatrixSet *p_tile_mset, 
                                           string vector_file = "", 
                                           double clip_offset = 0);
  bool			GetPixelSize (int &width, int &height);
  bool      GetSRS(OGRSpatialReference  &srs, ITileMatrixSet *p_tile_mset = 0);

	GDALDataset*	get_gdal_ds_ref();

  bool			CalcBandStatistics	(int band_num, double &min, double &max, double &mean, double &std, double *p_nodata_val =0);
	double		get_nodata_value		(bool &nodata_defined);

	static			bool ReadSpatialRefFromMapinfoTabFile (string tab_file, OGRSpatialReference &srs);
  static      bool SetBackgroundToGDALDataset (GDALDataset *p_ds, unsigned char background[3]); 

protected:
  bool			GetDefaultSpatialRef(OGRSpatialReference	&srs, OGRSpatialReference *p_tiling_srs);


protected:
  string set_srs_;
 	char	buf[256];
	string	raster_file_;
	GDALDataset	*p_gdal_ds_;
	double		nodata_value_;
	bool	nodata_value_defined_;
	int		num_bands_;
	GDALDataType gdal_data_type_;
};



class BundleTiler
{
public:
	BundleTiler(void);
	~BundleTiler(void);
	void Close();

public:
  int	Init	(list<pair<string,string>> raster_vector, 
            ITileMatrixSet* p_tile_mset, 
            string user_input_srs = "",
            double clip_offset = 0);

  int CalcNumberOfTiles (int zoom);
	int	CalcAppropriateZoom();
  OGREnvelope	CalcEnvelope();

  bool RunBaseZoomTiling	(	TilingParameters		*p_tiling_params, 
								            ITileContainer			*p_tile_container);
  
  int RunChunk (gmx::TilingParameters* p_tiling_params,
                gmx::ITileContainer* p_tile_container,
                int zoom,
                OGREnvelope chunk_envp,
                int tiles_expected,
                int* p_tiles_generated,
                bool is_scalinig_needed,
                double* p_scale_values,
                double* p_offset_values
              );
  static int CallRunChunk(BundleTiler* p_bundle,
                          gmx::TilingParameters* p_tiling_params,
                          gmx::ITileContainer* p_tile_container,
                          int zoom,
                          OGREnvelope chunk_envp,
                          int tiles_expected,
                          int* p_tiles_generated,
                          bool is_scalinig_needed,
                          double* p_scale_values,
                          double* p_offset_values
                          );
    
protected:
  //ToDo bool      GetRasterProfile(bool &nodata_defined); GDALDataType, nodata_val, colortable
  double  GetNodataValue(bool &nodata_defined);
  GDALDataType  GetRasterFileType();




  bool			WarpChunkToBuffer (	int zoom,	
                                OGREnvelope	chunk_envp, 
                                RasterBuffer* p_dst_buffer,
                                int output_bands_num = 0,
                                map<string,int*>* p_band_mapping = 0,
                                GDALResampleAlg resample_alg = GRA_Cubic,
                                int* p_ndval = NULL,
                                unsigned char* p_background_color = NULL);

  bool RunTilingFromBuffer (TilingParameters	*p_tiling_params, 
						  RasterBuffer	*p_buffer,
              OGREnvelope buffer_envelope,
						  int zoom,
						  int tiles_expected, 
						  int *p_tiles_generated,
						  ITileContainer *p_tile_container);

  bool          CalclScalingTo8BitParams   (  double* &scales,
                                                 double*  &offsets,
                                                 int*  p_nodata_val=0,
                                                 int      output_bands_num = 0,
                                                 map<string,int*>*  p_band_mapping=0);
  list<string>	GetFileList();
	bool			    Intersects(OGREnvelope envp);
  ITileMatrixSet* tile_matrix_set(){return p_tile_mset_;};

protected:
	bool			AddItemToBundle (string raster_file, string	vector_file);
  bool      AdjustCutlinesForOverlapping180Degree();

  bool      WaitForTilingThreads(list<future<int>> *p_tiling_threads, int nMaxThreads);
  bool      TerminateTilingThreads(list<future<int>> &tiling_results);
   
protected:
  bool m_bUseWarpClipHack;
  double clip_offset_;
  string set_srs_;
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
