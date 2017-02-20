#pragma once
#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H
#include "stdafx.h"
#include "histogram.h"
#include "VectorBorder.h"


namespace gmx
{


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
	bool			CreateBufferFromJpegData	(void *p_data_src, int size);
	bool			CreateBufferFromPngData		(void *p_data_src, int size);
 	bool			CreateBufferFromPseudoPngData	(void *p_data_src, int size);
  bool			CreateFromJP2Data			(void *pabData, int nSize);
	bool			CreateBufferFromTiffData	(void *p_data_src, int size);

	bool			SaveToPngData	(void* &p_data_dst, int &size);
  bool			SaveToPng24Data	(void* &p_data_dst, int &size);
	bool			SaveToPseudoPngData	(void* &p_data_dst, int &size);
  bool			SaveToJpegData	(void* &p_data_dst, int &size, int quality = 0);
	bool			SaveToTiffData	(void* &p_data_dst, int &size);
	bool			SaveToJP2Data	(void* &pabData, int &nSize, int nRate = 0);

	//bool			IsAnyNoDataPixel			();

	bool			SaveBufferToFile		(string filename, int quality = 0);
	bool			SaveBufferToFileAndData	(string filename, void* &p_data_dst, int &size, int quality = 0);

	//bool			ResizeAndConvertToRGB	(int nNewWidth, int nNewHeight);
	//bool			MergeUsingBlack (RasterBuffer oBackGround, RasterBuffer &oMerged);

	//bool			makeZero(LONG nLeft, LONG nTop, LONG nWidth, LONG nHeight, LONG nNoDataValue = 0);
	bool			InitByRGBColor	 (BYTE rgb[3]);
	bool			InitByValue(int value = 0);	

	void*			GetPixelDataBlock	(	int left, int top, int w, int h);
	bool			SetPixelDataBlock	(int left, int top, int w, int h, void *p_pixel_data_block, int bands = 0);
	void*			ZoomOut	(GDALResampleAlg resampling_method);	
	bool			ConvertFromIndexToRGB ();
	bool			ConvertFromPanToRGB();
	bool			CreateAlphaBandByRGBColor(BYTE	*p_rgb, int tolerance = 0);

	bool      CreateAlphaBandByPixelLinePolygon(VectorOperations *p_vb);

  bool			IsAlphaBand();
	//BOOL			createAlphaBandByValue(int	value);
	bool			StretchDataTo8Bit(double *min_values, double *max_values);
    

public:
	void*			get_pixel_data_ref();
	int				get_num_bands();
	int				get_x_size();
	int				get_y_size();
	GDALDataType	get_data_type();
	GDALColorTable*	get_color_table_ref ();
	bool			set_color_table (GDALColorTable *p_color_table);

protected:
	//void									initAlphaBand();
  void*                         GetPixelDataOrder2();
  template <typename T>void*    GetPixelDataOrder2(T type);
 //template <typename T>	bool		IsAnyNoDataPixel(T type);
	template <typename T>	bool		InitByValue		(T type, int value);
	template <typename T>	void*		GetPixelDataBlock	(	T type, int left, int top, int w, int h);
	template <typename T>	bool		SetPixelDataBlock	(	T type, int left, int top, int w, int h, 
																                     void *p_block_data, int bands = 0);
	template <typename T>	void*		ZoomOut(T type, GDALResampleAlg resampling_method);
  template <typename T> bool    StretchDataTo8Bit(T type, double *min_values, double *max_values);


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

#endif