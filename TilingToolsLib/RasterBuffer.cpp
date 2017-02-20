#include "StdAfx.h"
#include "RasterBuffer.h"
#include "FileSystemFuncs.h"
using namespace gmx;


namespace gmx
{



RasterBuffer::RasterBuffer(void)
{
	p_pixel_data_ = NULL;	
	p_color_table_ = NULL;
	alpha_band_defined_	= FALSE;
}


RasterBuffer::~RasterBuffer(void)
{
	ClearBuffer();
}



void RasterBuffer::ClearBuffer()
{
	if (p_pixel_data_!=NULL)
	{
		switch (data_type_)
		{
			case GDT_Byte:
				delete[]((BYTE*)p_pixel_data_);
				break;
			case GDT_UInt16:
				delete[]((unsigned __int16*)p_pixel_data_);
				break;
			case GDT_Int16:
				delete[]((__int16*)p_pixel_data_);
				break;
			case GDT_Float32:
				delete[]((float*)p_pixel_data_);
				break;
		}
		p_pixel_data_ = NULL;
	}
	
	if (p_color_table_!=NULL)
	{
		GDALDestroyColorTable(p_color_table_);
		p_color_table_ = NULL;
	}	
}



bool RasterBuffer::CreateBuffer	(int			num_bands,
								 int			x_size,
								 int			y_size,
								 void			*p_pixel_data_src,
								 GDALDataType	data_type,
								 bool			alpha_band_defined,
								 GDALColorTable *p_color_table					
								 )
{
	ClearBuffer();

	num_bands_	= num_bands;
	x_size_		= x_size;
	y_size_		= y_size;
	data_type_		= data_type;
	alpha_band_defined_	= alpha_band_defined;
	p_color_table_ = (p_color_table) ? p_color_table->Clone() : NULL;

	switch (data_type_)
	{
		case GDT_Byte:
				this->p_pixel_data_ = new BYTE[num_bands_*x_size_*y_size_];
				data_size_ = 1;
				break;
		case GDT_UInt16:
				this->p_pixel_data_ = new unsigned __int16[num_bands_*x_size_*y_size_];
				data_size_ = 2;
				break;
		case GDT_Int16:
				this->p_pixel_data_ = new __int16[num_bands_*x_size_*y_size_];
				data_size_ = 2;
				break;
		case GDT_Float32:
				this->p_pixel_data_ = new float[num_bands_*x_size_*y_size_];
				data_size_ = 4;
				break;			
		default:
			return FALSE;
	}

	if (p_pixel_data_src !=NULL) memcpy(this->p_pixel_data_,p_pixel_data_src,data_size_*num_bands_*x_size_*y_size_);
	else InitByValue(0);

	return TRUE;
}


bool RasterBuffer::CreateBuffer		(RasterBuffer *pSrcBuffer)
{
	if (!CreateBuffer(	pSrcBuffer->get_num_bands(),
						pSrcBuffer->get_x_size(),
						pSrcBuffer->get_y_size(),
						pSrcBuffer->get_pixel_data_ref(),
						pSrcBuffer->get_data_type(),
						pSrcBuffer->alpha_band_defined_,
						pSrcBuffer->p_color_table_
						)) return FALSE;
	return TRUE;	
}



bool RasterBuffer::InitByRGBColor	 (BYTE rgb[3])
{
	if (data_type_ != GDT_Byte) return FALSE;
	if (p_pixel_data_ == NULL) return FALSE;

	BYTE *p_pixel_data_byte = (BYTE*)p_pixel_data_;
	__int64 n = x_size_*y_size_;
	
	if (num_bands_ < 3)
	{
		for (__int64 i = 0;i<n;i++)
			p_pixel_data_byte[i] = rgb[0];
	}
	else
	{
		for (__int64 i = 0;i<n;i++)
		{
			p_pixel_data_byte[i]		= rgb[0];
			p_pixel_data_byte[i+n]		= rgb[1];
			p_pixel_data_byte[i+n+n]	= rgb[2];
		}
	}
	return TRUE;
}

			

bool	RasterBuffer::CreateBufferFromTiffData	(void *p_data_src, int size)
{

	//ToDo - if nodata defined must create alphaband
  VSIFileFromMemBuffer("/vsimem/tiffinmem",(BYTE*)p_data_src,size,0);
	GDALDataset *p_ds = (GDALDataset*) GDALOpen("/vsimem/tiffinmem",GA_ReadOnly);

	CreateBuffer(p_ds->GetRasterCount(),p_ds->GetRasterXSize(),p_ds->GetRasterYSize(),NULL,p_ds->GetRasterBand(1)->GetRasterDataType());
	p_ds->RasterIO(GF_Read,0,0,x_size_,y_size_,p_pixel_data_,x_size_,y_size_,data_type_,num_bands_,NULL,0,0,0); 
	GDALClose(p_ds);
	VSIUnlink("/vsimem/tiffinmem");
	return TRUE;
}


bool	RasterBuffer::CreateBufferFromJpegData (void *p_data_src, int size)
{

	ClearBuffer();
	gdImagePtr im;
	if (!  (im =	gdImageCreateFromJpegPtr(size, p_data_src))) return FALSE;
	
	CreateBuffer(3,im->sx,im->sy,NULL,GDT_Byte);
	//gdImageDestroy(im);
	//return TRUE;
	BYTE	*p_pixel_data_byte	= (BYTE*)p_pixel_data_;

	
	int n = im->sx * im->sy;
	int color, k;
	for (int j=0;j<im->sy;j++)
	{
		for (int i=0;i<im->sx;i++)
		{
			color = im->tpixels[j][i];
			k = j*x_size_+i;		
			p_pixel_data_byte[k]		= (color>>16) & 0xff;
			p_pixel_data_byte[n+ k]	= (color>>8) & 0xff;
			p_pixel_data_byte[n+ n+ k] = color & 0xff;
		}
	}

	gdImageDestroy(im);
	return TRUE;

}


bool	RasterBuffer::CreateBufferFromPseudoPngData	(void *p_data_src, int size)
{
  gdImagePtr im;
	if (!  (im =	gdImageCreateFromPngPtr(size, p_data_src))) return FALSE;
  if (im->tpixels==NULL)
  {
   	gdImageDestroy(im);
    return FALSE;
  }
  else
  {
    if (im->tpixels[0][0]<<24)
    {
      CreateBuffer(1,im->sx,im->sy,NULL,GDT_Int16);
      __int16* p_pixel_data_int16 = (__int16*)p_pixel_data_;
      for (int j=0;j<im->sy;j++)
	    {
		    for (int i=0;i<im->sx;i++)
		    {
          p_pixel_data_int16[j*x_size_+i]= (im->tpixels[j][i]<<8)>>16;
			  }
      }
    }
    else
    {
      CreateBuffer(1,im->sx,im->sy,NULL,GDT_UInt16);
      unsigned __int16* p_pixel_data_uint16 = (unsigned __int16*)p_pixel_data_;
      for (int j=0;j<im->sy;j++)
	    {
		    for (int i=0;i<im->sx;i++)
		    {
          p_pixel_data_uint16[j*x_size_+i]= (im->tpixels[j][i]<<8)>>16;
			  }
      }
    }
  }

  gdImageDestroy(im);
  return TRUE;
}

bool	RasterBuffer::CreateBufferFromPngData (void *p_data_src, int size)
{
	gdImagePtr im;
	if (!  (im =	gdImageCreateFromPngPtr(size, p_data_src))) return FALSE;
	
  if (im->tpixels!=NULL)
  {
    if (im->alphaBlendingFlag)
    {
      CreateBuffer(4,im->sx,im->sy,NULL,GDT_Byte,TRUE);
      alpha_band_defined_ = true;
    }
    else CreateBuffer(3,im->sx,im->sy,NULL,GDT_Byte);
    
    BYTE	*p_pixel_data_byte	= (BYTE*)p_pixel_data_;
    int n = (num_bands_==1) ? 0 : im->sx * im->sy;
  	int color, k;
    for (int j=0;j<im->sy;j++)
	  {
		  for (int i=0;i<im->sx;i++)
		  {
        color = im->tpixels[j][i];
        k = j*x_size_+i;
			  if (im->alphaBlendingFlag)
			  {
				  p_pixel_data_byte[n + n + n + k] = ((color>>24) > 0) ? 0 : 255;
				  color = color & 0xffffff;
			  }
			  p_pixel_data_byte[k]		= (color>>16) & 0xff;
			  p_pixel_data_byte[n+ k]	= (color>>8) & 0xff;
			  p_pixel_data_byte[n+ n+ k] = color & 0xff;
      }
    }
  }
  else
  {
    GDALColorTable *p_table = new GDALColorTable();
    for (int i=0;i<im->colorsTotal;i++)
    {
      GDALColorEntry *p_color_entry = new GDALColorEntry();
      p_color_entry->c1 = im->red[i];
      p_color_entry->c2 = im->green[i];
      p_color_entry->c3 = im->blue[i];
      p_table->SetColorEntry(i,p_color_entry);
      delete(p_color_entry);
    }
    CreateBuffer(1,im->sx,im->sy,NULL,GDT_Byte,false,p_table);
    delete(p_table);
    
    BYTE	*p_pixel_data_byte	= (BYTE*)p_pixel_data_;
    for (int j=0;j<im->sy;j++)
	  {
		  for (int i=0;i<im->sx;i++)
		  {
        p_pixel_data_byte[j*x_size_+i] = im->pixels[j][i];
      }
    }
  }
 
	gdImageDestroy(im);
	return TRUE;

}


bool RasterBuffer::SaveBufferToFile (string file_name, int quality)
{
	void *p_data_dst = NULL;
	int size = 0;
	bool result = SaveBufferToFileAndData(file_name,p_data_dst,size,quality);
	delete[]((BYTE*)p_data_dst);
	return result;
}


bool	RasterBuffer::SaveBufferToFileAndData	(string file_name, void* &p_data_dst, int &size, int quality)
{
	int n_jpg = file_name.find(".jpg");
	int n_png = file_name.find(".png");
	int n_tif = file_name.find(".tif");
  int n_jp2 = file_name.find(".jp2");
	
	
	if (n_jpg>0)
	{
		if (!SaveToJpegData(p_data_dst,size,quality)) return FALSE;
	}
	else if (n_png>0)
	{
		if (!SaveToPngData(p_data_dst,size)) return FALSE;
	}
  else if (n_jp2>0)
	{
 		if (!SaveToJP2Data(p_data_dst,size,quality)) return FALSE;
	}
	else if (n_tif>0)
	{
		if (!SaveToTiffData(p_data_dst,size)) return FALSE;
	}

	else return FALSE;

	bool result = TRUE;
	if (!GMXFileSys::SaveDataToFile(file_name,p_data_dst,size)) result = FALSE;
	return TRUE;
}


bool RasterBuffer::ConvertFromPanToRGB ()
{
	if (x_size_==0 || y_size_ == 0 || data_type_ != GDT_Byte) return FALSE;
	if (num_bands_!=1)		return FALSE;
	if (this->p_pixel_data_==NULL)	return FALSE;
  int num_bands_new = IsAlphaBand() ? 4 : 3;
	
	BYTE *p_pixel_data_new = new BYTE[num_bands_new*x_size_*y_size_];
	int n = x_size_*y_size_;
  int m;
	for (int j=0;j<y_size_;j++)
	{
		for (int i=0;i<x_size_;i++)
		{
      m = j*x_size_+i;
			p_pixel_data_new[m + n + n] =
				(p_pixel_data_new[m +n] = (p_pixel_data_new[m] = ((BYTE*)p_pixel_data_)[m]));
      if (IsAlphaBand())
        p_pixel_data_new[m + n + n + n] = ((BYTE*)p_pixel_data_)[m + n];
		}
	}
	
	num_bands_ = num_bands_new;
	delete[]((BYTE*)p_pixel_data_);
	p_pixel_data_ = p_pixel_data_new;
	return TRUE;
}



bool	RasterBuffer::ConvertFromIndexToRGB ()
{
	if (p_color_table_!=NULL)
	{
		int n = x_size_*y_size_;
		int num_bands_new = IsAlphaBand() ? 4 : 3;
    
    BYTE *p_pixel_data_new = new BYTE[num_bands_new*n];
		int m;
		const GDALColorEntry *p_color_entry;
		for (int i=0;i<x_size_;i++)
		{
			for (int j=0;j<y_size_;j++)
			{
				m = j*x_size_+i;
				p_color_entry = p_color_table_->GetColorEntry(((BYTE*)p_pixel_data_)[m]);
				p_pixel_data_new[m] = p_color_entry->c1;
				p_pixel_data_new[m+n] = p_color_entry->c2;
				p_pixel_data_new[m+n+n] = p_color_entry->c3;
        if (IsAlphaBand())
         	p_pixel_data_new[m+n+n+n]=((BYTE*)p_pixel_data_)[n+m];
 			}
		}
		delete[]((BYTE*)p_pixel_data_);
		p_pixel_data_ = p_pixel_data_new;
		num_bands_ = num_bands_new;
		GDALDestroyColorTable(p_color_table_);
		p_color_table_ = NULL;
	}
	return TRUE;
}


bool RasterBuffer::SaveToJpegData (void* &p_data_dst, int &size, int quality)
{
	
	if ((x_size_ ==0)||(y_size_ == 0)||(data_type_!=GDT_Byte)) return FALSE;
  quality = quality == 0 ? 85 : quality;
	gdImagePtr im	= gdImageCreateTrueColor(x_size_,y_size_);
	
	int n = (num_bands_ < 3) ? 0 : x_size_*y_size_;
	int color = 0;
	BYTE	*p_pixel_data_byte = (BYTE*)p_pixel_data_;

	const GDALColorEntry *p_color_entry;
	for (int j=0;j<y_size_;j++)
	{
		for (int i=0;i<x_size_;i++)
		{
			if (!p_color_table_)
				im->tpixels[j][i] = gdTrueColor(p_pixel_data_byte[j*x_size_+i],p_pixel_data_byte[j*x_size_+i+n],p_pixel_data_byte[j*x_size_+i+n+n]);
			else
			{
				p_color_entry = p_color_table_->GetColorEntry(p_pixel_data_byte[j*x_size_+i]);
				im->tpixels[j][i] = gdTrueColor(p_color_entry->c1,p_color_entry->c2,p_color_entry->c3);
			}
		}
	}
	
	if (!(p_data_dst = (BYTE*)gdImageJpegPtr(im,&size,quality)))
	{
		gdImageDestroy(im);
		return FALSE;
	}

	gdImageDestroy(im);
	return TRUE;
}

//sample error debug callback expecting no client object

static void error_callback(const char *msg, void *client_data) {
	(void)client_data;
	fprintf(stdout, "[ERROR] %s", msg);
}

//sample warning debug callback expecting no client object

static void warning_callback(const char *msg, void *client_data) {
	(void)client_data;
	fprintf(stdout, "[WARNING] %s", msg);
}


//sample debug callback expecting no client object

static void info_callback(const char *msg, void *client_data) {
	//debug
  /*
  (void)client_data;
	fprintf(stdout, "[INFO] %s", msg);
  */
  //end-debug
}

bool RasterBuffer::CreateFromJP2Data (void *pabData, int nSize)
{
  //ToDo - if nodata defined must create alphaband
  VSIFileFromMemBuffer("/vsimem/jp2inmem", (BYTE*)pabData, nSize, 0);
  GDALDataset *poJP2DS = (GDALDataset*)GDALOpen("/vsimem/jp2inmem", GA_ReadOnly);
  CreateBuffer(poJP2DS->GetRasterCount(), poJP2DS->GetRasterXSize(), poJP2DS->GetRasterYSize(), NULL, poJP2DS->GetRasterBand(1)->GetRasterDataType());

  poJP2DS->RasterIO(GF_Read, 0, 0, x_size_, y_size_, p_pixel_data_, x_size_, y_size_, data_type_, num_bands_, NULL, 0, 0, 0);
  GDALClose(poJP2DS);
  VSIUnlink("/vsimem/jp2inmem");

 	return TRUE;
}

bool RasterBuffer::SaveToJP2Data(void* &pabData, int &nSize, int nRate)
{
  string	strTiffInMem = ("/vsimem/tiffinmem" + GMXString::ConvertIntToString(rand()));
  GDALDataset* poTiffDS = (GDALDataset*)GDALCreate(
      GDALGetDriverByName("GTiff"),
      strTiffInMem.c_str(),
      x_size_,
      y_size_,
      num_bands_,
      data_type_,
      NULL
      );
  poTiffDS->RasterIO(GF_Write, 0, 0, x_size_, y_size_, p_pixel_data_, x_size_, y_size_, data_type_, num_bands_, NULL, 0, 0, 0);
  GDALFlushCache(poTiffDS);
  
  string strJP2DriverName = JP2000DriverFactory::GetDriverName();
  string strJP2InMem = ("/vsimem/jp2inmem" + GMXString::ConvertIntToString(rand()));
  GDALDatasetH poJP2DS = GDALCreateCopy(GDALGetDriverByName(strJP2DriverName.c_str()),
                                       strJP2InMem.c_str(),
                                       poTiffDS, 0, 0, 0, 0);
  GDALFlushCache(poJP2DS);
  GDALClose(poJP2DS);
  GDALClose(poTiffDS);
  vsi_l_offset length;
  BYTE * pabDataBuf = VSIGetMemFileBuffer(strJP2InMem.c_str(), &length, FALSE);
  nSize = length;
  memcpy((pabData = new BYTE[nSize]), pabDataBuf, nSize);
  VSIUnlink(strJP2InMem.c_str());
  VSIUnlink(strTiffInMem.c_str());

	return TRUE;
}


bool RasterBuffer::SaveToPseudoPngData	(void* &p_data_dst, int &size)
{
  if (x_size_==0 || y_size_ == 0) return FALSE;

  gdImagePtr im		= gdImageCreateTrueColor(x_size_,y_size_);
  int n = y_size_*x_size_;
  
  if (this->data_type_== GDT_Byte)
  {
    BYTE *p_pixel_data_byte	= (BYTE*)p_pixel_data_;
    for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++)
				im->tpixels[j][i] = int(p_pixel_data_byte[j*x_size_+i])<<8;
		}
  }
  else if (this->data_type_==GDT_UInt16)
  {
    unsigned __int16 *p_pixel_data_uint16	= (unsigned __int16*)p_pixel_data_;
    for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++)
				im->tpixels[j][i] = int(p_pixel_data_uint16[j*x_size_+i])<<8;
		}
  }
  else if (this->data_type_==GDT_Int16)
  {
    __int16	*p_pixel_data_int16	= (__int16*)p_pixel_data_;
    for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++)
				im->tpixels[j][i] = (int(p_pixel_data_int16[j*x_size_+i])<<8) + 1; // непонятно будет ли работать для отрицательных чисел??? //todo
		}
  }

  if (!(p_data_dst = (BYTE*)gdImagePngPtr(im,&size)))
	{
		gdImageDestroy(im);
		return FALSE;
	}

	gdImageDestroy(im);
	return TRUE;
}


bool RasterBuffer::SaveToPng24Data (void* &p_data_dst, int &size)
{
	if ((x_size_ ==0)||(y_size_ == 0)||(data_type_!=GDT_Byte)) return FALSE;
	
	gdImagePtr im		= gdImageCreateTrueColor(x_size_,y_size_);

 
 	BYTE	*p_pixel_data_byte	= (BYTE*)p_pixel_data_;
	int n = y_size_*x_size_;
	int transparency = 0;


	if ((num_bands_ == 4) || (num_bands_ == 2))  //alpha_band_defined_ == true
	{
		im->alphaBlendingFlag	= 1;
		im->saveAlphaFlag		= 1;
	  if ((this->p_color_table_) && (num_bands_ == 2))
    {
      const GDALColorEntry *p_color_entry = NULL; 
      for (int j=0;j<y_size_;j++)
		  {
			  for (int i=0;i<x_size_;i++){
          p_color_entry = p_color_table_->GetColorEntry(p_pixel_data_byte[j*x_size_+i]);
          im->tpixels[j][i] = gdTrueColorAlpha(p_color_entry->c1,
                                               p_color_entry->c2,
                                               p_color_entry->c3,
									                             (p_pixel_data_byte[j*x_size_+ i + n] >0) ? 0 : 127);

        }
		  }
    }
    else
    {
      int m = (num_bands_ == 4) ? y_size_*x_size_ : 0;
		  for (int j=0;j<y_size_;j++)
		  {
			  for (int i=0;i<x_size_;i++){
          im->tpixels[j][i] = gdTrueColorAlpha(p_pixel_data_byte[j*x_size_+i],
									                             p_pixel_data_byte[j*x_size_ + i + m],
									                             p_pixel_data_byte[j*x_size_+ i + m + m],
									                             (p_pixel_data_byte[j*x_size_+ i + m + m + n] >0) ? 0 : 127);
        }
		  }
    }
	}
	else //num_bands_==3 && alpha_band_defined_ == false
	{
		im->alphaBlendingFlag	= 0;
		im->saveAlphaFlag		= 0;
		for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++)
				im->tpixels[j][i] = gdTrueColor(p_pixel_data_byte[j*x_size_+i],
                                        p_pixel_data_byte[j*x_size_+i+n],
                                        p_pixel_data_byte[j*x_size_+i+n+n]);
		}
	}
  
	if (!(p_data_dst = (BYTE*)gdImagePngPtr(im,&size)))
	{
		gdImageDestroy(im);
		return FALSE;
	}

	gdImageDestroy(im);
	return TRUE;
}
	
bool RasterBuffer::SaveToPngData (void* &p_data_dst, int &size)
{

	if ((x_size_ ==0)||(y_size_ == 0)||(data_type_!=GDT_Byte)) return FALSE;
  if (num_bands_>1) return SaveToPng24Data(p_data_dst,size);
  
  gdImagePtr im	= gdImageCreate(x_size_,y_size_);

  if (p_color_table_==NULL)
  {
    im->colorsTotal = 256;
    for (int i=0;i<256;i++)
    {
	    im->red[i]		= i;
	    im->green[i]	= i;
	    im->blue[i]		= i;
	    im->open[i]		= 0;
    }
  }
  else
  {
    im->colorsTotal = p_color_table_->GetColorEntryCount();
    for (int i=0;(i<im->colorsTotal)&&(i<gdMaxColors);i++)
    {
	    const GDALColorEntry *p_color_entry = p_color_table_->GetColorEntry(i);
		
	    im->red[i]		= p_color_entry->c1;
	    im->green[i]	= p_color_entry->c2;
	    im->blue[i]		= p_color_entry->c3;
	    im->open[i]		= 0;
    }
  }

	for (int j=0;j<y_size_;j++)
	{
		for (int i=0;i<x_size_;i++)
		{			
			im->pixels[j][i] = ((BYTE*)p_pixel_data_)[j*x_size_+i];
		}
	}

	if (!(p_data_dst = (BYTE*)gdImagePngPtr(im,&size)))
	{
		gdImageDestroy(im);
		return FALSE;
	}

	gdImageDestroy(im);
	return TRUE;
}


bool	RasterBuffer::SaveToTiffData	(void* &p_data_dst, int &size)
{
  
  srand(999);
  string			tiff_in_mem = ("/vsimem/tiffinmem" + GMXString::ConvertIntToString(rand()));

	GDALDataset* p_ds = (GDALDataset*)GDALCreate(
		GDALGetDriverByName("GTiff"),
    tiff_in_mem.c_str(),
		x_size_,
		y_size_,
		num_bands_,
		data_type_,
		NULL
		);
	p_ds->RasterIO(GF_Write,0,0,x_size_,y_size_,p_pixel_data_,x_size_,y_size_,data_type_,num_bands_,NULL,0,0,0);
	GDALFlushCache(p_ds);
	GDALClose(p_ds);
	vsi_l_offset length; 
	
	BYTE * p_data_dstBuf = VSIGetMemFileBuffer(tiff_in_mem.c_str(),&length, FALSE);
	size = length;
	//GDALClose(p_ds);
	memcpy((p_data_dst = new BYTE[size]),p_data_dstBuf,size);
  VSIUnlink(tiff_in_mem.c_str());
	
	return TRUE;
}


bool RasterBuffer::InitByValue(int value)
{
	if (p_pixel_data_==NULL) return FALSE;

	switch (data_type_)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return InitByValue(t,value);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return InitByValue(t,value);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return InitByValue(t,value);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return InitByValue(t,value);
		}
		default:
			return NULL;
	}
	return TRUE;	
}

void* RasterBuffer::GetPixelDataOrder2()
{
  if (this->get_pixel_data_ref()==NULL) return NULL;
  switch (data_type_)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return GetPixelDataOrder2(t);
		}
		case GDT_UInt16:
		{
			__int16 t = 257;
			return GetPixelDataOrder2(t);
		}
		case GDT_Int16:
		{
			__int16 t = 257;
			return GetPixelDataOrder2(t);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return GetPixelDataOrder2(t);;
		}
		default:
			return NULL;
	}
  return NULL;
}

template <typename T>
void* RasterBuffer::GetPixelDataOrder2(T type)
{
  T *p_pixel_data_t = (T*)p_pixel_data_;
  T *p_data_order2 = new T[x_size_*y_size_*num_bands_];
  
  int n =0;
  int area = x_size_*y_size_;
  int area2;
  for (int j=0;j<y_size_;j++)
  {
    for (int i=0;i<x_size_;i++)
    {
      area2 = j*x_size_;
      for (int b=0;b<num_bands_;b++)
      {
        p_data_order2[n] = p_pixel_data_t[area*b + area2 + i];
        n++;
      }
    }
  }

	return p_data_order2;
}

template <typename T>
bool RasterBuffer::InitByValue(T type, int value)
{
	if (p_pixel_data_==NULL) return FALSE;

	T *p_pixel_data_t= (T*)p_pixel_data_;
	if (!alpha_band_defined_)
	{
		unsigned __int64 n = num_bands_*x_size_*y_size_;
		for (unsigned __int64 i=0;i<n;i++)
			p_pixel_data_t[i]=value;
	}
	else
	{
		unsigned __int64 n = (num_bands_-1)*x_size_*y_size_;
		for (unsigned __int64 i=0;i<n;i++)
			p_pixel_data_t[i]=value;
		n = num_bands_*x_size_*y_size_;
		for (unsigned __int64 i=(num_bands_-1)*x_size_*y_size_;i<n;i++)
			p_pixel_data_t[i]=0;

	}
	return TRUE;	
}



bool	RasterBuffer::StretchDataTo8Bit(double *minValues, double *maxValues)
{
  if (p_pixel_data_ == NULL || x_size_ == 0 || y_size_==0) return NULL;

	switch (data_type_)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return StretchDataTo8Bit(t,minValues,maxValues);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return StretchDataTo8Bit(t,minValues,maxValues);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return StretchDataTo8Bit(t,minValues,maxValues);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return StretchDataTo8Bit(t,minValues,maxValues);
		}
		default:
			return FALSE;
	}
	return FALSE;
}

template <typename T>
bool	RasterBuffer::StretchDataTo8Bit(T type, double *p_min_values, double *p_max_values)
{
  BYTE  *p_pixel_stretched_data = new BYTE[x_size_*y_size_*this->num_bands_];
  T     *p_pixel_data_t = (T*)p_pixel_data_;
  int n = x_size_*y_size_;
  double d;
  int m;

  int _num_bands = IsAlphaBand() ? num_bands_-1 : num_bands_;
  for (int i=0;i<y_size_;i++)
  {
    for (int j=0;j<x_size_;j++)
    {
      for (int b=0;b<_num_bands;b++)
      {
        m = i*x_size_+j+b*n;
        d = max(min(p_pixel_data_t[m],p_max_values[b]),p_min_values[b]);
        p_pixel_stretched_data[m] = (int)(0.5+255*((d-p_min_values[b])/(p_max_values[b]-p_min_values[b])));
      }
    }
  }
  
  if (IsAlphaBand())
  {
    n = x_size_*y_size_*(num_bands_-1);
    for (int i=0;i<y_size_;i++)
    {
      for (int j=0;j<x_size_;j++)
      {
          m = i*x_size_+j+n;
          p_pixel_stretched_data[m] = (int)(p_pixel_data_t[m]);
      }
    }
  }

  delete[]p_pixel_data_t;
  p_pixel_data_= p_pixel_stretched_data;
  data_type_=GDT_Byte;

  return TRUE;
}


void*	RasterBuffer::GetPixelDataBlock (int left, int top, int w, int h)
{
	if (p_pixel_data_ == NULL || x_size_ == 0 || y_size_==0) return NULL;
	
	switch (data_type_)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return GetPixelDataBlock(t,left,top,w,h);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return GetPixelDataBlock(t,left,top,w,h);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return GetPixelDataBlock(t,left,top,w,h);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return GetPixelDataBlock(t,left,top,w,h);
		}
		default:
			return NULL;
	}
	return NULL;
}


template <typename T>
void*	RasterBuffer::GetPixelDataBlock (T type, int left, int top, int w, int h)
{
	if (num_bands_==0) return NULL;
	int					n = w*h;
	unsigned __int64	m = x_size_*y_size_;
	T				*p_pixel_block_t;
	p_pixel_block_t		= new T[num_bands_*n];

	
	T *p_pixel_data_t= (T*)p_pixel_data_;
	for (int k=0;k<num_bands_;k++)
	{
		for (int j=left;j<left+w;j++)
		{
			for (int i=top;i<top+h;i++)
			{
				p_pixel_block_t[n*k+(i-top)*w+j-left] = p_pixel_data_t[m*k+i*x_size_+j];
			}
		}
	}			

	return p_pixel_block_t;
}



void*		RasterBuffer::ZoomOut	(GDALResampleAlg resampling_method)
{
	void *p_pixel_data_zoomedout;
	switch (data_type_)
	{
		case GDT_Byte:
		{
			BYTE t = 1;
			return ZoomOut(t,resampling_method);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return ZoomOut(t,resampling_method);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return ZoomOut(t,resampling_method);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return ZoomOut(t,resampling_method);
		}
		default:
			return NULL;
	}
}

template<typename T>
void* RasterBuffer::ZoomOut	(T type, GDALResampleAlg resampling_method)
{ 
  int n	= x_size_*y_size_;
	int n_4	= x_size_*y_size_/4;
	int w = x_size_/2;
	int h = y_size_/2;
	T	*p_pixel_data_zoomedout	= NULL;

	if(! (p_pixel_data_zoomedout = new T[num_bands_*n_4]) )
	{
		return NULL;
	}

  int m,k,q,l,i,j,b,num_def_pix;
  T	*p_pixel_data_t		= (T*)p_pixel_data_;
  
  int _num_bands = (IsAlphaBand()) ? num_bands_-1 : num_bands_;

  int r[4], pixel_sum[100];
  int min, dist, l_min, _dist;

  for (i=0;i<h;i++)
	{
		for (j=0;j<w;j++)
		{
      m = i*w;
      if (!IsAlphaBand())
      {
        for (l=0;l<4;l++)
          r[l] = (m<<2) + (j<<1) + l%2 + (l>>1)*x_size_;
        num_def_pix = 4;
      }
      else
      {
        num_def_pix=0;
        k = n_4*(num_bands_-1);

        for (l=0;l<4;l++)
        {
          q = (m<<2) + (j<<1) + l%2 + (l>>1)*x_size_;
          if (p_pixel_data_t[q + (k<<2)] != 0)
          {
            r[num_def_pix] = q;
            num_def_pix++;
          }
        }

        if (num_def_pix==0 ||
            (num_def_pix==1 && resampling_method!=GRA_NearestNeighbour)
            )
        {
          p_pixel_data_zoomedout[m+j+k] = 0;
          for (b=0;b<_num_bands;b++)
            p_pixel_data_zoomedout[m+j+n_4*b] = p_pixel_data_t[(m<<2) + (j<<1) +b*n];
          continue;
        }
        else p_pixel_data_zoomedout[m+j+k] = 255;
      }
                
      for (b=0;b<_num_bands;b++)
      {
        pixel_sum[b] = 0;
        for (l=0;l<num_def_pix;l++)
          pixel_sum[b]+=p_pixel_data_t[r[l]+b*n];
      }
                  
      if (resampling_method!=GRA_NearestNeighbour)
      {
        for (b=0;b<_num_bands;b++)
           p_pixel_data_zoomedout[m+j+b*n_4]=  (pixel_sum[b]/num_def_pix) + (((pixel_sum[b]%num_def_pix)<<1)>num_def_pix); 
      }
      else
      {
        min = 1000000;
        l_min=0;
    
        for (l=0;l<num_def_pix;l++)
        {
          dist=0;
          for (b=0;b<_num_bands;b++)
             dist+= ((_dist = pixel_sum[b]-p_pixel_data_t[r[l]+b*n]*num_def_pix) > 0) ? 
                      _dist : -_dist;
          if (dist<min)
          {
            l_min=l;
            min=dist;
          }
        }
        for (b=0;b<_num_bands;b++)
           p_pixel_data_zoomedout[m+j+b*n_4]=p_pixel_data_t[r[l_min]+b*n];
      }
    }
  }

  return p_pixel_data_zoomedout;
}


bool	RasterBuffer::IsAlphaBand()
{
	return alpha_band_defined_;
}


bool  RasterBuffer::CreateAlphaBandByPixelLinePolygon (VectorOperations *p_vb)
{

  if (x_size_==0 || y_size_==0 || num_bands_==0) return FALSE;
  BYTE *vector_mask = new BYTE[x_size_*y_size_];

  //OGRGeometry *p_ogr_geom = p_vb->get_ogr_geometry_ref();
  for (int j=0;j<y_size_;j++)
  {
    int num_points = 0;
    int  *p_x = NULL;
    
    int n = j*x_size_;
    for (int i = 0; i<x_size_; i++)
        vector_mask[n+i]=0;
    
    //ToDo
    if ((!VectorOperations::CalcIntersectionBetweenLineAndPixelLineGeometry(j,p_vb->get_ogr_geometry_ref(),num_points,p_x)) || 
        (num_points == 0) || ((num_points%2)==1))
      continue;
            
    for (int k=0;k<num_points;k+=2)
    {
      for (int i=p_x[k];i<=p_x[k+1];i++)
      {
        if (i<0) continue;
        if (i>=x_size_) break;
        vector_mask[n+i]=255;
      }
    }

    delete[]p_x;
  } 

  RasterBuffer temp_buffer;
  if (!temp_buffer.CreateBuffer(num_bands_+1,x_size_,y_size_,NULL,data_type_,1,p_color_table_)) return FALSE;

  int n = x_size_*y_size_*num_bands_;
  int m = x_size_*y_size_;
  
  void *p_new_pixel_data = new BYTE[(n+m)*data_size_];
  if (!p_new_pixel_data ) return FALSE;
  if (!memcpy(p_new_pixel_data,p_pixel_data_,n))
  {
    delete[]p_new_pixel_data;
    return FALSE;
  }
    
  switch (data_type_)
	{
		case GDT_Byte:
		{
      BYTE *p_new_pixel_data_B = (BYTE*)p_new_pixel_data;
      for (int i=0;i<m;i++)
         p_new_pixel_data_B[i+n]=vector_mask[i];
    }

    case GDT_UInt16:
		{
      unsigned __int16 *p_new_pixel_data_UInt16 = (unsigned __int16*)p_new_pixel_data;
      for (int i=0;i<m;i++)
         p_new_pixel_data_UInt16[i+n]=vector_mask[i];
    }

    case GDT_Int16:
		{
      __int16 *p_new_pixel_data_Int16 = (__int16*)p_new_pixel_data;
      for (int i=0;i<m;i++)
         p_new_pixel_data_Int16[i+n]=vector_mask[i];
    }

    case GDT_Float32:
		{
      float *p_new_pixel_data_F32 = (float*)p_new_pixel_data;
      for (int i=0;i<m;i++)
         p_new_pixel_data_F32[i+n]=vector_mask[i];
    }
    default:
      return FALSE;
  }

  delete[]p_pixel_data_;
  p_pixel_data_=p_new_pixel_data;
  num_bands_++;
  alpha_band_defined_ = TRUE;

  return TRUE;
}



bool	RasterBuffer::CreateAlphaBandByRGBColor(BYTE	*pRGB, int tolerance)
{
	if ((this-p_pixel_data_ == NULL) || (data_type_!=GDT_Byte) || (num_bands_>3)) return FALSE;

	int n = x_size_ *y_size_;
	int num_bands_new = (num_bands_==1 || num_bands_==3) ? num_bands_+1 : num_bands_;
  BYTE	*p_pixel_data_new = new BYTE[num_bands_new * n];
  memcpy(p_pixel_data_new,p_pixel_data_,num_bands_ * n);
	int d = (num_bands_new == 4) ? 1 : 0;

  for (int i=0; i<y_size_; i++)
	{
		for (int j=0; j<x_size_; j++)
			p_pixel_data_new[i*x_size_ + j + n + d*(n+n)] = (	abs(p_pixel_data_new[i*x_size_ + j] - pRGB[0]) + 
														abs(p_pixel_data_new[i*x_size_ + j + d*n] - pRGB[0 +d] ) +
														abs(p_pixel_data_new[i*x_size_ + j + d*(n+n)] - pRGB[0+d+d] ) <= tolerance)
														? 0 : 255;
			/*
			pData_new[i*x_size_ + j + n + d*(n+n)] = (	(pData_new[i*x_size_ + j] == pRGB[0] ) &&
														(pData_new[i*x_size_ + j + d*n] == pRGB[0 +d] ) &&
														(pData_new[i*x_size_ + j + d*(n+n)] == pRGB[0+d+d] ) ) 
														? 100 : 0;
			*/
	}
	delete[]((BYTE*)p_pixel_data_);
	p_pixel_data_ = p_pixel_data_new;
	num_bands_ = num_bands_new;
	alpha_band_defined_ = TRUE;
	return TRUE;
}

//bool	RasterBuffer::createAlphaBandByValue(int	value);


bool	RasterBuffer::SetPixelDataBlock (int left, int top, int w, int h, void *p_block_data, int bands)
{
	if (p_pixel_data_ == NULL || x_size_ == 0 || y_size_==0) return NULL;
	bands = (bands==0) ? num_bands_ : bands;

	switch (data_type_)
	{
		case GDT_Byte:
		{
			BYTE t = 1;
			return SetPixelDataBlock(t,left,top,w,h,p_block_data,bands);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return SetPixelDataBlock(t,left,top,w,h,p_block_data,bands);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return SetPixelDataBlock(t,left,top,w,h,p_block_data,bands);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return SetPixelDataBlock(t,left,top,w,h,p_block_data,bands);
		}
		default:
			return FALSE;
	}
	return FALSE;
}

///*
template <typename T>
bool	RasterBuffer::SetPixelDataBlock (T type, int left, int top, int w, int h, void *p_block_data, int bands)
{
	bands = (bands==0) ? num_bands_ : bands;

	int n = w*h;
	int m = x_size_*y_size_;
	T *p_pixel_data_t = (T*)p_pixel_data_;
	T *p_block_data_t = (T*)p_block_data;


	for (int k=0;k<bands;k++)
	{
		for (int j=left;j<left+w;j++)
			for (int i=top;i<top+h;i++)
				p_pixel_data_t[m*k+i*x_size_+j] = p_block_data_t[n*k+(i-top)*w + j-left];
	}		

	return TRUE;
}
//*/




void* RasterBuffer::get_pixel_data_ref()
{
	return p_pixel_data_;
}


int	RasterBuffer::get_num_bands()
{
	return num_bands_;	
}


int RasterBuffer::get_x_size()
{
	return x_size_;	
}


int RasterBuffer::get_y_size()
{
	return y_size_;	
}


GDALDataType RasterBuffer::get_data_type()
{
	return data_type_;
}


bool	RasterBuffer::set_color_table (GDALColorTable *p_color_table)
{
	p_color_table_ = p_color_table->Clone();
	
	return TRUE;
}


GDALColorTable*	RasterBuffer::get_color_table_ref ()
{
	return p_color_table_;
}


string JP2000DriverFactory::GetDriverName()
{
  if (strDriverName != "") return strDriverName;
  else if (GDALGetDriverByName("JP2KAK")) strDriverName = "JP2KAK";
  else if (GDALGetDriverByName("JP2OpenJPEG")) strDriverName = "JP2OpenJPEG";
  else if (GDALGetDriverByName("JPEG2000")) strDriverName = "JPEG2000";
  return strDriverName;
}

string JP2000DriverFactory::strDriverName = "";

}