#include "rasterbuffer.h"
#include "filesystemfuncs.h"
#include "stringfuncs.h"
using namespace gmx;


namespace gmx
{



RasterBuffer::RasterBuffer(void)
{
	p_pixel_data_ = NULL;	
	p_color_table_ = NULL;
	alpha_band_defined_	= false;
}

/*
RasterBuffer::RasterBuffer(RasterBuffer &oSrcBuffer)
{
	CreateBuffer(&oSrcBuffer);
}
*/

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
				delete[]((unsigned char*)p_pixel_data_);
				break;
			case GDT_UInt16:
				delete[]((uint16_t*)p_pixel_data_);
				break;
			case GDT_Int16:
				delete[]((int16_t*)p_pixel_data_);
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
				this->p_pixel_data_ = new unsigned char[num_bands_*x_size_*y_size_];
				data_size_ = 1;
				break;
		case GDT_UInt16:
				this->p_pixel_data_ = new uint16_t[num_bands_*x_size_*y_size_];
				data_size_ = 2;
				break;
		case GDT_Int16:
				this->p_pixel_data_ = new int16_t[num_bands_*x_size_*y_size_];
				data_size_ = 2;
				break;
		case GDT_Float32:
				this->p_pixel_data_ = new float[num_bands_*x_size_*y_size_];
				data_size_ = 4;
				break;			
		default:
			return false;
	}

	if (p_pixel_data_src !=NULL) memcpy(this->p_pixel_data_,p_pixel_data_src,data_size_*num_bands_*x_size_*y_size_);
	else InitByValue(0);

	return true;
}


bool RasterBuffer::CreateBuffer		(RasterBuffer *poSrcBuffer)
{
	if (!CreateBuffer(	poSrcBuffer->get_num_bands(),
						poSrcBuffer->get_x_size(),
						poSrcBuffer->get_y_size(),
						poSrcBuffer->get_pixel_data_ref(),
						poSrcBuffer->get_data_type(),
						poSrcBuffer->alpha_band_defined_,
						poSrcBuffer->p_color_table_
						)) return false;
	return true;	
}



bool RasterBuffer::InitByRGBColor	 (unsigned char rgb[3])
{
	if (data_type_ != GDT_Byte) return false;
	if (p_pixel_data_ == NULL) return false;

	unsigned char *p_pixel_data_byte = (unsigned char*)p_pixel_data_;
	int64_t n = x_size_*y_size_;
	
	if (num_bands_ < 3)
	{
		for (int64_t i = 0;i<n;i++)
			p_pixel_data_byte[i] = rgb[0];
	}
	else
	{
		for (int64_t i = 0;i<n;i++)
		{
			p_pixel_data_byte[i]		= rgb[0];
			p_pixel_data_byte[i+n]		= rgb[1];
			p_pixel_data_byte[i+n+n]	= rgb[2];
		}
	}
	return true;
}

			

bool	RasterBuffer::CreateBufferFromTiffData	(void *p_data_src, int size)
{

	//ToDo - if nodata defined must create alphaband
  VSIFileFromMemBuffer("/vsimem/tiffinmem",(GByte*)p_data_src,size,0);
	GDALDataset *p_ds = (GDALDataset*) GDALOpen("/vsimem/tiffinmem",GA_ReadOnly);

	CreateBuffer(p_ds->GetRasterCount(),p_ds->GetRasterXSize(),p_ds->GetRasterYSize(),NULL,p_ds->GetRasterBand(1)->GetRasterDataType());
	p_ds->RasterIO(GF_Read,0,0,x_size_,y_size_,p_pixel_data_,x_size_,y_size_,data_type_,num_bands_,NULL,0,0,0); 
	GDALClose(p_ds);
	VSIUnlink("/vsimem/tiffinmem");
	return true;
}

bool	RasterBuffer::CreateBufferFromInMemoryData(void* p_data_src, int size, TileType oRasterFormat)
{
	ClearBuffer();
	switch (oRasterFormat)
	{
		case JPEG_TILE:
			return CreateBufferFromJpegData(p_data_src, size);
		case PSEUDO_PNG_TILE:
			return CreateBufferFromPseudoPngData(p_data_src, size);
		case PNG_TILE:
			return CreateBufferFromPngData(p_data_src, size);
		case JP2_TILE:
			return CreateFromJP2Data(p_data_src, size);
		default:
			return CreateBufferFromTiffData(p_data_src, size);
	}
	return true;
}


bool	RasterBuffer::CreateBufferFromJpegData (void *p_data_src, int size)
{
	gdImagePtr im;
	if (!  (im =	gdImageCreateFromJpegPtr(size, p_data_src))) return false;
	
	CreateBuffer(3,im->sx,im->sy,NULL,GDT_Byte);

	unsigned char	*p_pixel_data_byte	= (unsigned char*)p_pixel_data_;

	
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
	return true;

}


bool	RasterBuffer::CreateBufferFromPseudoPngData	(void *p_data_src, int size)
{
  gdImagePtr im;
	if (!  (im =	gdImageCreateFromPngPtr(size, p_data_src))) return false;
  if (im->tpixels==NULL)
  {
   	gdImageDestroy(im);
    return false;
  }
  else
  {
    if (im->tpixels[0][0]<<24)
    {
      CreateBuffer(1,im->sx,im->sy,NULL,GDT_Int16);
      int16_t* p_pixel_data_int16 = (int16_t*)p_pixel_data_;
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
      uint16_t* p_pixel_data_uint16 = (uint16_t*)p_pixel_data_;
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
  return true;
}

bool	RasterBuffer::CreateBufferFromPngData (void *p_data_src, int size)
{
	gdImagePtr im;
	if (!  (im =	gdImageCreateFromPngPtr(size, p_data_src))) return false;
	
  if (im->tpixels!=NULL)
  {
    if (im->alphaBlendingFlag)
    {
      CreateBuffer(4,im->sx,im->sy,NULL,GDT_Byte,true);
      alpha_band_defined_ = true;
    }
    else CreateBuffer(3,im->sx,im->sy,NULL,GDT_Byte);
    
    unsigned char	*p_pixel_data_byte	= (unsigned char*)p_pixel_data_;
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
    
    unsigned char	*p_pixel_data_byte	= (unsigned char*)p_pixel_data_;
    for (int j=0;j<im->sy;j++)
	  {
		  for (int i=0;i<im->sx;i++)
		  {
        p_pixel_data_byte[j*x_size_+i] = im->pixels[j][i];
      }
    }
  }
 
	gdImageDestroy(im);
	return true;

}


bool RasterBuffer::SaveBufferToFile (string file_name, int quality)
{
	void *p_data_dst = NULL;
	int size = 0;
	bool result = SaveBufferToFileAndData(file_name,p_data_dst,size,quality);
	delete[]((unsigned char*)p_data_dst);
	return result;
}


bool	RasterBuffer::SaveBufferToFileAndData	(string file_name, void* &p_data_dst, int &size, int quality)
{
	if (GMXFileSys::GetExtension(file_name)=="jpg")
	{
		if (!SaveToJpegData(p_data_dst,size,quality)) return false;
	}
  else if (GMXFileSys::GetExtension(file_name) == "png")
	{
		if (!SaveToPngData(p_data_dst,size)) return false;
	}
  else if (GMXFileSys::GetExtension(file_name) == "jp2")
	{
 		if (!SaveToJP2Data(p_data_dst,size,quality)) return false;
	}
  else if (GMXFileSys::GetExtension(file_name) == "tif")
	{
		if (!SaveToTiffData(p_data_dst,size)) return false;
	}

	else return false;


	if (!GMXFileSys::SaveDataToFile(file_name,p_data_dst,size)) return false;
	return true;
}


bool RasterBuffer::SaveToJpegData (void* &p_data_dst, int &size, int quality)
{
	
	if ((x_size_ ==0)||(y_size_ == 0)||(data_type_!=GDT_Byte)) return false;
  quality = quality == 0 ? 85 : quality;
	gdImagePtr im	= gdImageCreateTrueColor(x_size_,y_size_);
	
	int n = (num_bands_ < 3) ? 0 : x_size_*y_size_;
	int color = 0;
	unsigned char	*p_pixel_data_byte = (unsigned char*)p_pixel_data_;

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
	
	if (!(p_data_dst = (unsigned char*)gdImageJpegPtr(im,&size,quality)))
	{
		gdImageDestroy(im);
		return false;
	}

	gdImageDestroy(im);
	return true;
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

}




bool RasterBuffer::SaveToPseudoPngData	(void* &p_data_dst, int &size)
{
  if (x_size_==0 || y_size_ == 0) return false;

  gdImagePtr im		= gdImageCreateTrueColor(x_size_,y_size_);
  int n = y_size_*x_size_;
  
  if (this->data_type_== GDT_Byte)
  {
    unsigned char *p_pixel_data_byte	= (unsigned char*)p_pixel_data_;
    for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++)
				im->tpixels[j][i] = int(p_pixel_data_byte[j*x_size_+i])<<8;
		}
  }
  else if (this->data_type_==GDT_UInt16)
  {
    uint16_t *p_pixel_data_uint16	= (uint16_t*)p_pixel_data_;
    for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++)
				im->tpixels[j][i] = int(p_pixel_data_uint16[j*x_size_+i])<<8;
		}
  }
  else if (this->data_type_==GDT_Int16)
  {
    int16_t	*p_pixel_data_int16	= (int16_t*)p_pixel_data_;
    for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++)
				im->tpixels[j][i] = (int(p_pixel_data_int16[j*x_size_+i])<<8) + 1; // непонятно будет ли работать для отрицательных чисел??? //todo
		}
  }

  if (!(p_data_dst = (unsigned char*)gdImagePngPtr(im,&size)))
	{
		gdImageDestroy(im);
		return false;
	}

	gdImageDestroy(im);
	return true;
}


bool RasterBuffer::SaveToPng24Data (void* &p_data_dst, int &size)
{
	if ((x_size_ ==0)||(y_size_ == 0)||(data_type_!=GDT_Byte)) return false;
	
	gdImagePtr im		= gdImageCreateTrueColor(x_size_,y_size_);

 
 	unsigned char	*p_pixel_data_byte	= (unsigned char*)p_pixel_data_;
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
									                             (p_pixel_data_byte[j*x_size_+ i + m + m + n] >1) ? 0 : 127);
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
  
	if (!(p_data_dst = (unsigned char*)gdImagePngPtr(im,&size)))
	{
		gdImageDestroy(im);
		return false;
	}

	gdImageDestroy(im);
	return true;
}

bool RasterBuffer::SerializeToInMemoryData(void* &p_data_src, int &size, TileType oRasterFormat, int nQuality)
{
	switch (oRasterFormat)
	{
		case JPEG_TILE:
			return SaveToJpegData(p_data_src, size, nQuality);
		case PNG_TILE:
			return SaveToPngData(p_data_src, size);
		case PSEUDO_PNG_TILE:
			return SaveToPseudoPngData(p_data_src, size);
		case JP2_TILE:
			return SaveToJP2Data(p_data_src, size,nQuality);
		default:
			return SaveToTiffData(p_data_src, size);
	}
	return true;
}

	
bool RasterBuffer::SaveToPngData (void* &p_data_dst, int &size)
{

	if ((x_size_ ==0)||(y_size_ == 0)||(data_type_!=GDT_Byte)) return false;
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
			im->pixels[j][i] = ((unsigned char*)p_pixel_data_)[j*x_size_+i];
		}
	}

	if (!(p_data_dst = (unsigned char*)gdImagePngPtr(im,&size)))
	{
		gdImageDestroy(im);
		return false;
	}

	gdImageDestroy(im);
	return true;
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
	if (p_color_table_ && num_bands_ <3)
		p_ds->GetRasterBand(1)->SetColorTable(p_color_table_);

	GDALFlushCache(p_ds);
	GDALClose(p_ds);
	vsi_l_offset length; 
	
	unsigned char * p_data_dstBuf = VSIGetMemFileBuffer(tiff_in_mem.c_str(),&length, false);
	size = length;
	//GDALClose(p_ds);
	memcpy((p_data_dst = new unsigned char[size]),p_data_dstBuf,size);
  VSIUnlink(tiff_in_mem.c_str());
	
	return true;
}


bool RasterBuffer::InitByValue(int value)
{
	if (p_pixel_data_==NULL) return false;

	switch (data_type_)
	{
		case GDT_Byte:
		{
			unsigned char t  = 1;
			return InitByValue(t,value);
		}
		case GDT_UInt16:
		{
			uint16_t t = 257;
			return InitByValue(t,value);
		}
		case GDT_Int16:
		{
			int16_t t = -257;
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
	return true;	
}


template <typename T>
bool RasterBuffer::InitByValue(T type, int value)
{
	if (p_pixel_data_==NULL) return false;

	T *p_pixel_data_t= (T*)p_pixel_data_;
	if (!alpha_band_defined_)
	{
		uint64_t n = num_bands_*x_size_*y_size_;
		for (uint64_t i=0;i<n;i++)
			p_pixel_data_t[i]=value;
	}
	else
	{
		uint64_t n = (num_bands_-1)*x_size_*y_size_;
		for (uint64_t i=0;i<n;i++)
			p_pixel_data_t[i]=value;
		n = num_bands_*x_size_*y_size_;
		for (uint64_t i=(num_bands_-1)*x_size_*y_size_;i<n;i++)
			p_pixel_data_t[i]=0;

	}
	return true;	
}



bool	RasterBuffer::ScaleDataTo8Bit(double* p_scales, double* p_offsets)
{
  if (p_pixel_data_ == NULL || x_size_ == 0 || y_size_==0) return NULL;

	switch (data_type_)
	{
		case GDT_Byte:
		{
			unsigned char t  = 1;
      return ScaleDataTo8Bit(t,p_scales,p_offsets);
		}
		case GDT_UInt16:
		{
			uint16_t t = 257;
      return ScaleDataTo8Bit(t,p_scales,p_offsets);
		}
		case GDT_Int16:
		{
			int16_t t = -257;
      return ScaleDataTo8Bit(t,p_scales,p_offsets);
		}
		case GDT_Float32:
		{
			float t = 1.1;
      return ScaleDataTo8Bit(t,p_scales,p_offsets);
		}
		default:
			return false;
	}
	return false;
}


template <typename T>
bool	RasterBuffer::ScaleDataTo8Bit(T type, double* p_scales, double* p_offsets)
{
  unsigned char  *p_pixel_stretched_data = new unsigned char[x_size_*y_size_*this->num_bands_];
  T     *p_pixel_data_t = (T*)p_pixel_data_;
  int n = x_size_*y_size_;
  int m,d;

  int _num_bands = IsAlphaBand() ? num_bands_ - 1 : num_bands_;

  for (int b = 0; b<_num_bands; b++)
  {
    m = b*n;
    if ( (fabs(p_scales[b]-1.)<1e-5) && (p_offsets[b] == 0.))
    {
      for (int i=m; i<m+n; i++)
        p_pixel_stretched_data[i] = p_pixel_data_t[i];
    }
    else
    {
      for (int i = m; i<m + n; i++)
      {
        d = (int)(0.5 + (((double)p_pixel_data_t[i])*p_scales[b] + p_offsets[b]));
        p_pixel_stretched_data[i] = min(max(d,0),255);
      }
    }
    
  }

  if (IsAlphaBand())
  {
    m = x_size_*y_size_*(num_bands_ - 1);
    for (int i = m; i<m + n; i++)
      p_pixel_stretched_data[i] = p_pixel_data_t[i];
  }

  delete[]p_pixel_data_t;
  p_pixel_data_ = p_pixel_stretched_data;
  data_type_ = GDT_Byte;

  return true;
}


/*//legacy version 
template <typename T>
bool	RasterBuffer::ScaleDataTo8Bit(T type, double *p_min_values, double *p_max_values)
{
  unsigned char  *p_pixel_stretched_data = new unsigned char[x_size_*y_size_*this->num_bands_];
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
        d = max(min((double)p_pixel_data_t[m],p_max_values[b]),p_min_values[b]);
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

  return true;
}
*/

void*	RasterBuffer::GetPixelDataBlock (int left, int top, int w, int h)
{
	if (p_pixel_data_ == 0 || x_size_ == 0 || y_size_==0) return 0;
	
	switch (data_type_)
	{
		case GDT_Byte:
		{
			unsigned char t  = 1;
			return GetPixelDataBlock(t,left,top,w,h);
		}
		case GDT_UInt16:
		{
			uint16_t t = 257;
			return GetPixelDataBlock(t,left,top,w,h);
		}
		case GDT_Int16:
		{
			int16_t t = -257;
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
	uint64_t	m = x_size_*y_size_;
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
			unsigned char t = 1;
			return ZoomOut(t,resampling_method);
		}
		case GDT_UInt16:
		{
			uint16_t t = 257;
			return ZoomOut(t,resampling_method);
		}
		case GDT_Int16:
		{
			int16_t t = -257;
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


/*
bool  RasterBuffer::CreateAlphaBandByPixelLinePolygon (VectorOperations *p_vb)
{

  if (x_size_==0 || y_size_==0 || num_bands_==0) return false;
  unsigned char *vector_mask = new unsigned char[x_size_*y_size_];

  //OGRGeometry *p_ogr_geom = p_vb->get_ogr_geometry_ref();
  for (int j=0;j<y_size_;j++)
  {
    int num_points = 0;
    int* p_x = 0;
    
    int n = j*x_size_;
    for (int i = 0; i<x_size_; i++)
        vector_mask[n+i]=0;
    
    //ToDo
    if ((!VectorOperations::IntersectYLineWithPixelLineGeometry(j,p_vb->get_ogr_geometry_ref(),num_points,p_x)) || 
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
  if (!temp_buffer.CreateBuffer(num_bands_+1,x_size_,y_size_,0,data_type_,1,p_color_table_)) return false;

  int n = x_size_*y_size_*num_bands_;
  int m = x_size_*y_size_;
  
  unsigned char* p_new_pixel_data = new unsigned char[(n + m)*data_size_];
  memcpy(p_new_pixel_data,p_pixel_data_,n);
   
  switch (data_type_)
	{
		case GDT_Byte:
		{
      unsigned char *p_new_pixel_data_B = (unsigned char*)p_new_pixel_data;
      for (int i=0;i<m;i++)
         p_new_pixel_data_B[i+n]=vector_mask[i];
    }

    case GDT_UInt16:
		{
      uint16_t *p_new_pixel_data_UInt16 = (uint16_t*)p_new_pixel_data;
      for (int i=0;i<m;i++)
         p_new_pixel_data_UInt16[i+n]=vector_mask[i];
    }

    case GDT_Int16:
		{
      int16_t *p_new_pixel_data_Int16 = (int16_t*)p_new_pixel_data;
      for (int i=0;i<m;i++)
         p_new_pixel_data_Int16[i+n]=vector_mask[i];
    }

    case GDT_Float32:
		{
      float *p_new_pixel_data_F32 = (float*)p_new_pixel_data;
      for (int i=0;i<m;i++)
         p_new_pixel_data_F32[i+n]=vector_mask[i];
    }
  }

  delete[]((unsigned char*)p_pixel_data_);
  delete[]vector_mask;

  p_pixel_data_=p_new_pixel_data;
  num_bands_++;
  alpha_band_defined_ = true;

  return true;
}
*/

bool RasterBuffer::CreateAlphaBandByNodataValues(unsigned char** p_nd_rgb, int rgb_num, int tolerance)
{
  if ((this->p_pixel_data_ == 0) || (data_type_ != GDT_Byte) || (num_bands_>3)) return false;

  int n = x_size_ *y_size_;
  int num_bands_new = (num_bands_ == 1 || num_bands_ == 3) ? num_bands_ + 1 : num_bands_;
  unsigned char	*p_pixel_data_new = new unsigned char[num_bands_new * n];
  memcpy(p_pixel_data_new, p_pixel_data_, num_bands_ * n);
  int d = (num_bands_new == 4) ? 1 : 0;

  int t;
  for (int i = 0; i<y_size_; i++)
  {
    for (int j = 0; j < x_size_; j++)
    {
      for (int k = 0; k < rgb_num; k++)
      {
        t = i*x_size_ + j;
        if ((abs(p_pixel_data_new[t] - p_nd_rgb[k][0]) +
          abs(p_pixel_data_new[t + d*n] - p_nd_rgb[k][0 + d]) +
          abs(p_pixel_data_new[t + d*(n + n)] - p_nd_rgb[k][0 + d + d])) <= tolerance)
        {
          p_pixel_data_new[t + n + d*(n + n)] = 0;
          break;
        }
        else p_pixel_data_new[t + n + d*(n + n)] = 255;
      }
    }
  }
  delete[]((unsigned char*)p_pixel_data_);
  p_pixel_data_ = p_pixel_data_new;
  num_bands_ = num_bands_new;
  alpha_band_defined_ = true;
  return true;
}




bool	RasterBuffer::SetPixelDataBlock ( int left, 
                                        int top, 
                                        int w, 
                                        int h, 
                                        void *p_block_data, 
                                        int band_min, 
                                        int band_max)
{
	if (p_pixel_data_ == NULL || x_size_ == 0 || y_size_==0) return NULL;
	
	switch (data_type_)
	{
		case GDT_Byte:
		{
			unsigned char t = 1;
			return SetPixelDataBlock(t,left,top,w,h,p_block_data,band_min,band_max);
		}
		case GDT_UInt16:
		{
			uint16_t t = 257;
      return SetPixelDataBlock(t, left, top, w, h, p_block_data, band_min, band_max);
		}
		case GDT_Int16:
		{
			int16_t t = -257;
      return SetPixelDataBlock(t, left, top, w, h, p_block_data, band_min, band_max);
		}
		case GDT_Float32:
		{
			float t = 1.1;
      return SetPixelDataBlock(t, left, top, w, h, p_block_data, band_min, band_max);
		}
		default:
			return false;
	}
	return false;
}

///*
template <typename T>
bool	RasterBuffer::SetPixelDataBlock(T type, 
                                      int left,
                                      int top,
                                      int w,
                                      int h,
                                      void *p_block_data,
                                      int band_min,
                                      int band_max)
{
  band_min = band_min == -1 ? 0 : band_min;
  band_max = band_max == -1 ? num_bands_ : band_max + 1;

	int n = w*h;
	int m = x_size_*y_size_;
	T *p_pixel_data_t = (T*)p_pixel_data_;
	T *p_block_data_t = (T*)p_block_data;


	for (int k=band_min;k<band_max;k++)
	{
		for (int j=left;j<left+w;j++)
			for (int i=top;i<top+h;i++)
				p_pixel_data_t[m*k+i*x_size_+j] = p_block_data_t[n*(k-band_min)+(i-top)*w + j-left];
	}		

	return true;
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
	
	return true;
}


GDALColorTable*	RasterBuffer::get_color_table_ref ()
{
	return p_color_table_;
}



/*
==========================================================
Hoodoo piece of code to supportJPEG2000
==========================================================
*/
string JP2000DriverFactory::GetDriverName()
{
  if (strDriverName != "") return strDriverName;
  else if (GDALGetDriverByName("JP2KAK")) strDriverName = "JP2KAK";
  else if (GDALGetDriverByName("JP2OpenJPEG")) strDriverName = "JP2OpenJPEG";
  else if (GDALGetDriverByName("JPEG2000")) strDriverName = "JPEG2000";
  return strDriverName;
}

string JP2000DriverFactory::strDriverName = "";


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
  unsigned char* pabDataBuf = VSIGetMemFileBuffer(strJP2InMem.c_str(), &length, false);
  nSize = length;
  memcpy((pabData = new unsigned char[nSize]), pabDataBuf, nSize);
  VSIUnlink(strJP2InMem.c_str());
  VSIUnlink(strTiffInMem.c_str());

  return true;
}


bool RasterBuffer::CreateFromJP2Data(void *pabData, int nSize)
{
  //ToDo - if nodata defined must create alphaband
  VSIFileFromMemBuffer("/vsimem/jp2inmem", (GByte*)pabData, nSize, 0);
  GDALDataset *poJP2DS = (GDALDataset*)GDALOpen("/vsimem/jp2inmem", GA_ReadOnly);

  CreateBuffer(poJP2DS->GetRasterCount(), poJP2DS->GetRasterXSize(), poJP2DS->GetRasterYSize(), NULL, poJP2DS->GetRasterBand(1)->GetRasterDataType());
  poJP2DS->RasterIO(GF_Read, 0, 0, x_size_, y_size_, p_pixel_data_, x_size_, y_size_, data_type_, num_bands_, NULL, 0, 0, 0);  
  
  GDALClose(poJP2DS);
  VSIUnlink("/vsimem/jp2inmem");

  return true;
}

#ifdef OPJLIB_DIRECT_SUPPORT
struct OPJStreamData
{
  OPJ_SIZE_T offset;
  OPJ_SIZE_T size;
  OPJ_SIZE_T max_size;
  void *p_data;
};

static OPJ_SIZE_T OPJStreamReadFunc(void* p_buffer, OPJ_SIZE_T n_bytes, void* p_stream_data)
{
  OPJStreamData *p_stream_obj = (OPJStreamData*)p_stream_data;
  if (p_stream_obj->offset >= p_stream_obj->size) return 0;
  int read_size = min(p_stream_obj->size - p_stream_obj->offset, n_bytes);
  if (!memcpy(p_buffer, (BYTE*)p_stream_obj->p_data + p_stream_obj->offset, read_size)) return 0;
  p_stream_obj->offset += read_size;
  return read_size;
}

static OPJ_SIZE_T OPJStreamWriteFunc(void* p_buffer, OPJ_SIZE_T n_bytes, void* p_stream_data)
{
  OPJStreamData *p_stream_obj = (OPJStreamData*)p_stream_data;
  if (p_stream_obj->offset + n_bytes > p_stream_obj->max_size)
  {
    cout << "Error: OPJStreamWriteFunc: max_size exceeded" << endl;
    return 0;
  }
  if (!memcpy((BYTE*)p_stream_obj->p_data + p_stream_obj->offset, p_buffer, n_bytes)) return 0;
  p_stream_obj->offset += n_bytes;
  p_stream_obj->size += n_bytes;
  return n_bytes;
}

static OPJ_OFF_T OPJStreamSkipFunc(OPJ_OFF_T n_skip, void *p_stream_data)
{
  OPJStreamData *p_stream_obj = (OPJStreamData*)p_stream_data;
  if (p_stream_obj->offset + n_skip > p_stream_obj->max_size) return -1;
  p_stream_obj->offset += n_skip;
  return n_skip;
}


static OPJ_BOOL OPJStreamSeekFunc(OPJ_OFF_T n_seek, void *p_stream_data)
{
  OPJStreamData *p_stream_obj = (OPJStreamData*)p_stream_data;
  if (n_seek <= p_stream_obj->size) p_stream_obj->offset = n_seek;
  else return OPJ_FALSE;
  return OPJ_TRUE;
}


static opj_stream_t* OPJStreamCreate(OPJStreamData *p_stream_data, unsigned int buffer_size, bool is_read_only)
{
  opj_stream_t* p_opj_stream = 00;
  if (!p_stream_data) return NULL;
  p_opj_stream = opj_stream_create(buffer_size, is_read_only);
  if (!p_opj_stream) return NULL;

  opj_stream_set_user_data(p_opj_stream, p_stream_data, 0);
  opj_stream_set_user_data_length(p_opj_stream, p_stream_data->size);
  opj_stream_set_read_function(p_opj_stream, (opj_stream_read_fn)OPJStreamReadFunc);
  opj_stream_set_write_function(p_opj_stream, (opj_stream_write_fn)OPJStreamWriteFunc);
  opj_stream_set_skip_function(p_opj_stream, (opj_stream_skip_fn)OPJStreamSkipFunc);
  opj_stream_set_seek_function(p_opj_stream, (opj_stream_seek_fn)OPJStreamSeekFunc);

  return p_opj_stream;
}
bool RasterBuffer::SaveToJP2Data(void* &p_data_dst, int &size, int compression_rate)
{
  if ((compression_rate <= 0) || (compression_rate > 20))  compression_rate = 10;

  int j, numcomps, w, h, index;
  OPJ_COLOR_SPACE color_space;

  opj_image_cmptparm_t cmptparm[20]; // RGBA //
  opj_image_t *image = NULL;
  int imgsize = 0;
  int has_alpha = 0;

  w = this->get_x_size();
  h = this->get_y_size();
  memset(&cmptparm[0], 0, 20 * sizeof(opj_image_cmptparm_t));

  numcomps = num_bands_;
  color_space = OPJ_CLRSPC_GRAY;

  for (j = 0; j < numcomps; j++)
  {
    cmptparm[j].prec = (data_type_ == GDT_Byte) ? 8 : 16;
    cmptparm[j].bpp = (data_type_ == GDT_Byte) ? 8 : 16;
    cmptparm[j].dx = 1;
    cmptparm[j].dy = 1;
    cmptparm[j].w = w;
    cmptparm[j].h = h;
  }

  image = opj_image_create(numcomps, &cmptparm[0], color_space);
  image->x0 = 0;
  image->y0 = 0;
  image->x1 = w;
  image->y1 = h;

  index = 0;
  imgsize = image->comps[0].w * image->comps[0].h;
  if (data_type_ == GDT_Byte)
  {
    BYTE	*pData8 = (BYTE*)p_pixel_data_;
    for (int i = 0; i<h; i++)
    {
      for (j = 0; j < w; j++)
      {
        for (int k = 0; k<numcomps; k++)
          image->comps[k].data[index] = pData8[index + k*imgsize];
        index++;
      }
    }
  }
  else
  {
    unsigned __int16	*pData16 = (unsigned __int16*)p_pixel_data_;
    for (int i = 0; i<h; i++)
    {
      for (j = 0; j < w; j++)
      {
        for (int k = 0; k<numcomps; k++)
          image->comps[k].data[index] = pData16[index + k*imgsize];
        index++;
      }
    }
  }

  opj_cparameters_t parameters;
  opj_stream_t *l_stream = 00;
  opj_codec_t* l_codec = 00;
  char indexfilename[OPJ_PATH_LEN];
  unsigned int i, num_images, imageno;

  OPJ_BOOL bSuccess;
  OPJ_BOOL bUseTiles = OPJ_FALSE;
  OPJ_UINT32 l_nb_tiles = 4;

  opj_set_default_encoder_parameters(&parameters);

  parameters.tcp_mct = 0;

  l_codec = opj_create_compress(OPJ_CODEC_JP2);

  opj_set_info_handler(l_codec, info_callback, 00);

  opj_set_warning_handler(l_codec, warning_callback, 00);
  opj_set_error_handler(l_codec, error_callback, 00);

  parameters.tcp_numlayers = 1;
  parameters.cod_format = 0;
  parameters.cp_disto_alloc = 1;
  parameters.tcp_rates[0] = compression_rate;
  bool r = opj_setup_encoder(l_codec, &parameters, image);

  OPJStreamData opj_stream_data;
  opj_stream_data.max_size = 10000000;
  opj_stream_data.offset = (opj_stream_data.size = 0);
  opj_stream_data.p_data = new BYTE[opj_stream_data.max_size];
  l_stream = OPJStreamCreate(&opj_stream_data, 10000000, FALSE);

  bSuccess = opj_start_compress(l_codec, image, l_stream);
  if (!bSuccess) fprintf(stderr, "failed to encode image: opj_start_compress\n");

  bSuccess = bSuccess && opj_encode(l_codec, l_stream);
  if (!bSuccess) fprintf(stderr, "failed to encode image: opj_encode\n");

  bSuccess = bSuccess && opj_end_compress(l_codec, l_stream);
  if (!bSuccess) fprintf(stderr, "failed to encode image: opj_end_compress\n");

  opj_destroy_codec(l_codec);
  opj_image_destroy(image);
  opj_stream_destroy(l_stream);

  p_data_dst = opj_stream_data.p_data;
  size = opj_stream_data.size;
  if (parameters.cp_comment)   free(parameters.cp_comment);
  if (parameters.cp_matrice)   free(parameters.cp_matrice);

  return TRUE;
}

bool RasterBuffer::CreateFromJP2Data(void *p_data_src, int size)
{
  ClearBuffer();
  if (size>10000000) return FALSE;
  OPJStreamData opj_stream_data;
  opj_stream_data.max_size = (opj_stream_data.size = size);
  opj_stream_data.offset = 0;
  opj_stream_data.p_data = new BYTE[size];
  memcpy(opj_stream_data.p_data, p_data_src, size);
  opj_stream_t *l_stream = OPJStreamCreate(&opj_stream_data, 10000000, TRUE);


  opj_dparameters_t parameters;   // decompression parameters //
  opj_image_t* image = NULL;
  opj_codec_t* l_codec = NULL;				// Handle to a decompressor //
  opj_codestream_index_t* cstr_index = NULL;

  char indexfilename[OPJ_PATH_LEN];	// index file name //

  OPJ_INT32 num_images, imageno;
  //img_fol_t img_fol;
  //dircnt_t *dirptr = NULL;

  // set decoding parameters to default values //
  opj_set_default_decoder_parameters(&parameters);

  // FIXME Initialize indexfilename and img_fol //
  *indexfilename = 0;

  // Initialize img_fol //
  //memset(&img_fol,0,sizeof(img_fol_t));

  // read the input file and put it in memory //
  //fsrc = fopen("e:\\erosb_16bit.jp2", "rb");
  //opj_stream_t l_stream_f = opj_stream_create_default_file_stream(fsrc,1);

  l_codec = opj_create_decompress(OPJ_CODEC_JP2);

  // catch events using our callbacks and give a local context //
  opj_set_info_handler(l_codec, info_callback, 00);
  opj_set_warning_handler(l_codec, warning_callback, 00);
  opj_set_error_handler(l_codec, error_callback, 00);


  // Setup the decoder decoding parameters using user parameters //
  if (!opj_setup_decoder(l_codec, &parameters)){
    fprintf(stderr, "Error -> j2k_dump: failed to setup the decoder\n");
    opj_stream_destroy(l_stream);
    //fclose(fsrc);
    opj_destroy_codec(l_codec);
    return false;
  }


  // Read the main header of the codestream and if necessary the JP2 boxes //
  if (!opj_read_header(l_stream, l_codec, &image)){
    fprintf(stderr, "Error -> opj_decompress: failed to read the header\n");
    opj_stream_destroy(l_stream);
    delete[]opj_stream_data.p_data;
    opj_destroy_codec(l_codec);
    opj_image_destroy(image);
    return false;
  }

  // Optional if you want decode the entire image //
  if (!opj_set_decode_area(l_codec, image, parameters.DA_x0,
    parameters.DA_y0, parameters.DA_x1, parameters.DA_y1)){
    fprintf(stderr, "Error -> opj_decompress: failed to set the decoded area\n");
    opj_stream_destroy(l_stream);
    opj_destroy_codec(l_codec);
    opj_image_destroy(image);
    delete[]opj_stream_data.p_data;
    return false;
  }

  //opj_stream_private
  // Get the decoded image //

  if (!(opj_decode(l_codec, l_stream, image))) {//&& opj_end_decompress(l_codec,	l_stream)
    fprintf(stderr, "Error -> opj_decompress: failed to decode image!\n");
    opj_destroy_codec(l_codec);
    opj_stream_destroy(l_stream);
    opj_image_destroy(image);
    delete[]opj_stream_data.p_data;
    return false;
  }

  opj_destroy_codec(l_codec);
  opj_stream_destroy(l_stream);
  delete[]opj_stream_data.p_data;

  this->CreateBuffer(image->numcomps, image->comps[0].w, image->comps[0].h, 0,
    (image->comps[0].prec == 8) ? GDT_Byte : GDT_UInt16, false, 0);

  int area = x_size_*y_size_;
  int index = 0;
  if (data_type_ == GDT_Byte)
  {
    BYTE *p_pixel_data = (BYTE*)p_pixel_data_;
    for (int i = 0; i<y_size_; i++)
    {
      for (int j = 0; j<x_size_; j++)
      {
        for (int k = 0; k<num_bands_; k++)
        {
          p_pixel_data[k*area + index] = image->comps[k].data[index];
        }
        index++;
      }
    }
  }
  else
  {
    unsigned __int16 *p_pixel_data = (unsigned __int16*)p_pixel_data_;
    for (int i = 0; i<y_size_; i++)
    {
      for (int j = 0; j<x_size_; j++)
      {
        for (int k = 0; k<num_bands_; k++)
        {
          p_pixel_data[k*area + index] = image->comps[k].data[index];
        }
        index++;
      }
    }
  }
  opj_image_destroy(image);

  return TRUE;
}


#endif

}