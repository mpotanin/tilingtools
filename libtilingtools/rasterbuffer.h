#pragma once
#include "stdafx.h"
#include "vectorborder.h"


namespace gmx
{

typedef enum {
	NDEF_TILE_TYPE = -1,
	JPEG_TILE = 0,
	PNG_TILE = 1,
	TIFF_TILE = 4,
	JP2_TILE = 16,
	PSEUDO_PNG_TILE = 32
} TileType;


class RasterBuffer
{
public:
	RasterBuffer(void);
	~RasterBuffer(void);

	void	ClearBuffer();

	bool			CreateBuffer	(int			num_bands,
									 int			x_size,
									 int			y_size,
									 void			*p_data_src				= NULL,
									 GDALDataType	data_type				= GDT_Byte,
									 bool			is_alpha_band			= FALSE,
									 GDALColorTable *p_color_table			= NULL
									 );

	bool			CreateBuffer				(RasterBuffer *p_src_buffer);

	bool			CreateBufferFromInMemoryData(void* p_data_src, int size, TileType oRasterFormat);

	bool			SerializeToInMemoryData(void* &p_data_src, int &size, TileType oRasterFormat, int nQuality = 0);

	
	bool			SaveBufferToFile		(string filename, int quality = 0);
	bool			SaveBufferToFileAndData	(string filename, void* &p_data_dst, int &size, int quality = 0);

	bool			InitByRGBColor	 (unsigned char rgb[3]);
	bool			InitByValue(int value = 0);	

	void*			GetPixelDataBlock	(	int left, int top, int w, int h);
	bool			SetPixelDataBlock	(int left, int top, int w, int h, void *p_pixel_data_block, int band_min = -1, int band_max = -1);
	void*			ZoomOut	(GDALResampleAlg resampling_method);	

  //ToDo
  bool			CreateAlphaBandByNodataValues(unsigned char	**p_nd_rgbcolors, int nd_num, int tolerance = 0);


	//bool      CreateAlphaBandByPixelLinePolygon(VectorOperations *p_vb);

  bool			IsAlphaBand();
	//BOOL			createAlphaBandByValue(int	value);
	bool			ScaleDataTo8Bit(double* p_scales, double* p_offsets);
    

public:
	void*			get_pixel_data_ref();
	int				get_num_bands();
	int				get_x_size();
	int				get_y_size();
	GDALDataType	get_data_type();
	GDALColorTable*	get_color_table_ref ();
	bool			set_color_table (GDALColorTable *p_color_table);

protected:
	bool			SaveToPngData(void* &p_data_dst, int &size);
	bool			SaveToPng24Data(void* &p_data_dst, int &size);
	bool			SaveToPseudoPngData(void* &p_data_dst, int &size);
	bool			SaveToJpegData(void* &p_data_dst, int &size, int quality = 0);
	bool			SaveToTiffData(void* &p_data_dst, int &size);
	bool			SaveToJP2Data(void* &pabData, int &nSize, int nRate = 0);

	bool			CreateBufferFromJpegData(void *p_data_src, int size);
	bool			CreateBufferFromPngData(void *p_data_src, int size);
	bool			CreateBufferFromPseudoPngData(void *p_data_src, int size);
	bool			CreateFromJP2Data(void *pabData, int nSize);
	bool			CreateBufferFromTiffData(void *p_data_src, int size);

 	template <typename T>	bool		InitByValue		(T type, int value);
	template <typename T>	void*		GetPixelDataBlock	(	T type, int left, int top, int w, int h);
	template <typename T>	bool		SetPixelDataBlock	(	T type, int left, int top, int w, int h, 
																                     void *p_block_data, int band_min = -1, int band_max = -1);
	template <typename T>	void*		ZoomOut(T type, GDALResampleAlg resampling_method);
  template <typename T> bool    ScaleDataTo8Bit(T type, double* p_scales, double* p_offsets);

	//void*                         GetPixelDataOrder2();
	//template <typename T>void*    GetPixelDataOrder2(T type);

protected:
	bool			alpha_band_defined_;
	void			*p_pixel_data_;
	GDALDataType	data_type_;
	int				data_size_;
	 
	
	int				num_bands_;
	int				x_size_;
	int				y_size_;

	GDALColorTable	*p_color_table_;
};


class JP2000DriverFactory
{
public:
  JP2000DriverFactory(){};
  ~JP2000DriverFactory(){};

public:
  static string GetDriverName();
  static string strDriverName;
};

}
