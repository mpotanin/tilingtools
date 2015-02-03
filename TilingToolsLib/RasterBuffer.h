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

	BOOL			CreateBuffer	(int			num_bands,
									 int			x_size,
									 int			y_size,
									 void			*p_data_src				= NULL,
									 GDALDataType	data_type				= GDT_Byte,
									 BOOL			is_alpha_band			= FALSE,
									 GDALColorTable *p_color_table			= NULL
									 );

	BOOL			CreateBuffer				(RasterBuffer *p_src_buffer);
	BOOL			CreateBufferFromJpegData	(void *p_data_src, int size);
	BOOL			CreateBufferFromPngData		(void *p_data_src, int size);
	BOOL			createFromJP2Data			(void *p_data_src, int size);
	BOOL			CreateBufferFromTiffData	(void *p_data_src, int size);

	BOOL			SaveToPngData	(void* &p_data_dst, int &size);
	BOOL			SaveToPng24Data	(void* &p_data_dst, int &size);
	BOOL			SaveToJpegData	(void* &p_data_dst, int &size, int quality = 0);
	BOOL			SaveToTiffData	(void* &p_data_dst, int &size);
	BOOL			SaveToJP2Data	(void* &p_data_dst, int &size, int compression_rate = 0);

	//BOOL			IsAnyNoDataPixel			();

	BOOL			SaveBufferToFile		(string filename, int quality = 0);
	BOOL			SaveBufferToFileAndData	(string filename, void* &p_data_dst, int &size, int quality = 0);

	//BOOL			ResizeAndConvertToRGB	(int nNewWidth, int nNewHeight);
	//BOOL			MergeUsingBlack (RasterBuffer oBackGround, RasterBuffer &oMerged);

	//BOOL			makeZero(LONG nLeft, LONG nTop, LONG nWidth, LONG nHeight, LONG nNoDataValue = 0);
	BOOL			InitByRGBColor	 (BYTE rgb[3]);
	BOOL			InitByValue(int value = 0);	

	void*			GetPixelDataBlock	(	int left, int top, int w, int h);
	BOOL			SetPixelDataBlock	(int left, int top, int w, int h, void *p_pixel_data_block, int bands = 0);
	void*			GetDataZoomedOut	(string resampling_method="");	
	BOOL			ConvertFromIndexToRGB ();
	BOOL			ConvertFromPanToRGB();
	BOOL			CreateAlphaBandByRGBColor(BYTE	*p_rgb, int tolerance = 0);

	BOOL      CreateAlphaBandByPixelLinePolygon(VectorBorder *p_vb);

  BOOL			IsAlphaBand();
	//BOOL			createAlphaBandByValue(int	value);
	BOOL			StretchDataTo8Bit(double *min_values, double *max_values);
  BOOL      AddPixelDataToMetaHistogram(MetaHistogram *p_hist);
    

public:
	void*			get_pixel_data_ref();
	int				get_num_bands();
	int				get_x_size();
	int				get_y_size();
	GDALDataType	get_data_type();
	GDALColorTable*	get_color_table_ref ();
	BOOL			set_color_table (GDALColorTable *p_color_table);

protected:
	//void									initAlphaBand();
  void*                         GetPixelDataOrder2();
  template <typename T>void*    GetPixelDataOrder2(T type);
 //template <typename T>	BOOL		IsAnyNoDataPixel(T type);
	template <typename T>	BOOL		InitByValue		(T type, int value);
	template <typename T>	void*		GetPixelDataBlock	(	T type, int left, int top, int w, int h);
	template <typename T>	BOOL		SetPixelDataBlock	(	T type, int left, int top, int w, int h, 
																                     void *p_block_data, int bands = 0);
	template <typename T>	void*		GetDataZoomedOut(T type,string resampling_method="");
  template <typename T>	BOOL    AddPixelDataToMetaHistogram(T type, MetaHistogram *p_hist);
  template <typename T> BOOL    StretchDataTo8Bit(T type, double *min_values, double *max_values);


protected:
	BOOL			alpha_band_defined_;
	void			*p_pixel_data_;
	GDALDataType	data_type_;
	int				data_size_;
	 
	
	int				num_bands_;
	int				x_size_;
	int				y_size_;

	GDALColorTable	*p_color_table_;
};


}

#endif