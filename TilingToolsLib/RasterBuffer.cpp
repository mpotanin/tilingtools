#include "StdAfx.h"
#include "RasterBuffer.h"
#include "FileSystemFuncs.h"
using namespace gmx;

/*
 * Callback function prototype for read function
 */
//typedef OPJ_SIZE_T (* opj_stream_read_fn) (void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data) ;


/*
static OPJ_SIZE_T opj_read_from_file (void * p_buffer, OPJ_SIZE_T p_nb_bytes, FILE * p_file)
{
	OPJ_SIZE_T l_nb_read = fread(p_buffer,1,p_nb_bytes,p_file);
	return l_nb_read ? l_nb_read : (OPJ_SIZE_T)-1;
}
*/


//	opj_stream_set_read_function(l_stream, (opj_stream_read_fn) opj_read_from_file);
//	opj_stream_set_write_function(l_stream, (opj_stream_write_fn) opj_write_from_file);
//	opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_skip_from_file);
//	opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_seek_from_file);

/*
static OPJ_SIZE_T opj_read_from_file (void * p_buffer, OPJ_SIZE_T p_nb_bytes, FILE * p_file)
{
        OPJ_SIZE_T l_nb_read = fread(p_buffer,1,p_nb_bytes,p_file);
        return l_nb_read ? l_nb_read : (OPJ_SIZE_T)-1;
}

static OPJ_UINT64 opj_get_data_length_from_file (FILE * p_file)
{
        OPJ_OFF_T file_length = 0;

        fseek(p_file, 0, SEEK_END);
        file_length = (OPJ_UINT64)ftell(p_file);
        fseek(p_file, 0, SEEK_SET);

        return file_length;
}



static OPJ_OFF_T opj_skip_from_file (OPJ_OFF_T p_nb_bytes, FILE * p_user_data)
{
        if (fseek(p_user_data,p_nb_bytes,SEEK_CUR)) {
                return -1;
        }

        return p_nb_bytes;
}

static OPJ_BOOL opj_seek_from_file (OPJ_OFF_T p_nb_bytes, FILE * p_user_data)
{
        if (fseek(p_user_data,p_nb_bytes,SEEK_SET)) {
                return OPJ_FALSE;
        }

        return OPJ_TRUE;
}


static opj_stream_t* gmx_opj_stream_create_default_file_stream(FILE *p_file, BOOL p_is_read_stream)
{
	opj_stream_t*	l_stream = opj_stream_create(1000000,false);
	opj_stream_set_user_data(l_stream, p_file);
  opj_stream_set_user_data_length(l_stream, opj_get_data_length_from_file(p_file));
  opj_stream_set_read_function(l_stream, (opj_stream_read_fn) opj_read_from_file);
  opj_stream_set_write_function(l_stream, (opj_stream_write_fn) opj_write_from_file);
  opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_skip_from_file);
  opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_seek_from_file);

	return l_stream;
}
*/


namespace gmx
{

struct OPJStreamData
{
  OPJ_SIZE_T offset;
  OPJ_SIZE_T size;
  OPJ_SIZE_T max_size;
  void *p_data;
};

static OPJ_SIZE_T OPJStreamReadFunc (void* p_buffer, OPJ_SIZE_T n_bytes, void* p_stream_data)
{
  OPJStreamData *p_stream_obj = (OPJStreamData*)p_stream_data;
  if (p_stream_obj->offset >= p_stream_obj->size) return 0;
  int read_size =  min(p_stream_obj->size-p_stream_obj->offset,n_bytes);
  if (!memcpy(p_buffer,(BYTE*)p_stream_obj->p_data+p_stream_obj->offset,read_size)) return 0;
  p_stream_obj->offset+=read_size;
  return read_size;
}

static OPJ_SIZE_T OPJStreamWriteFunc (void* p_buffer, OPJ_SIZE_T n_bytes, void* p_stream_data)
{
  OPJStreamData *p_stream_obj = (OPJStreamData*)p_stream_data;
  if (p_stream_obj->offset + n_bytes > p_stream_obj->max_size)
  {
    cout<<"Error: OPJStreamWriteFunc: max_size exceeded"<<endl;
    return 0;
  }
  if (! memcpy((BYTE*)p_stream_obj->p_data+p_stream_obj->offset,p_buffer,n_bytes)) return 0;
  p_stream_obj->offset+=n_bytes;
  p_stream_obj->size+=n_bytes;
  return n_bytes;
}

static OPJ_OFF_T OPJStreamSkipFunc (OPJ_OFF_T n_skip, void *p_stream_data)
{
  OPJStreamData *p_stream_obj = (OPJStreamData*)p_stream_data;
  if (p_stream_obj->offset + n_skip > p_stream_obj->max_size) return -1;
  p_stream_obj->offset+=n_skip;
  return n_skip;
}


static OPJ_BOOL OPJStreamSeekFunc (OPJ_OFF_T n_seek, void *p_stream_data)
{
  OPJStreamData *p_stream_obj = (OPJStreamData*)p_stream_data;
  if (n_seek<=p_stream_obj->size) p_stream_obj->offset=n_seek;
  else return OPJ_FALSE;
  return OPJ_TRUE;
}

/*
opj_stream_t* OPJ_CALLCONV opj_stream_create_file_stream (	FILE * p_file, 
															OPJ_SIZE_T p_size, 
															OPJ_BOOL p_is_read_stream)
{
	opj_stream_t* l_stream = 00;

	if (! p_file) {
		return NULL;
	}

	l_stream = opj_stream_create(p_size,p_is_read_stream);
	if (! l_stream) {
		return NULL;
	}

	opj_stream_set_user_data(l_stream, p_file);
	opj_stream_set_user_data_length(l_stream, opj_get_data_length_from_file(p_file));
	opj_stream_set_read_function(l_stream, (opj_stream_read_fn) opj_read_from_file);
	opj_stream_set_write_function(l_stream, (opj_stream_write_fn) opj_write_from_file);
	opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_skip_from_file);
	opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_seek_from_file);

	return l_stream;
}
*/

static opj_stream_t* OPJStreamCreate (OPJStreamData *p_stream_data, unsigned int buffer_size, BOOL is_read_only)
{
  opj_stream_t* p_opj_stream = 00;
  if (! p_stream_data) return NULL;
	p_opj_stream = opj_stream_create(buffer_size,is_read_only);
	if (! p_opj_stream) return NULL;
	
  opj_stream_set_user_data(p_opj_stream, p_stream_data);
  opj_stream_set_user_data_length(p_opj_stream, p_stream_data->size);
  opj_stream_set_read_function(p_opj_stream, (opj_stream_read_fn)OPJStreamReadFunc);
  opj_stream_set_write_function(p_opj_stream, (opj_stream_write_fn)OPJStreamWriteFunc);
  opj_stream_set_skip_function(p_opj_stream, (opj_stream_skip_fn)OPJStreamSkipFunc);
  opj_stream_set_seek_function(p_opj_stream, (opj_stream_seek_fn)OPJStreamSeekFunc);

  return p_opj_stream;
}



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



BOOL RasterBuffer::CreateBuffer	(int			num_bands,
								 int			x_size,
								 int			y_size,
								 void			*p_pixel_data_src,
								 GDALDataType	data_type,
								 BOOL			alpha_band_defined,
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
				data_size_ = 3;
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


BOOL RasterBuffer::CreateBuffer		(RasterBuffer *pSrcBuffer)
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



BOOL RasterBuffer::InitByRGBColor	 (BYTE rgb[3])
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

			

BOOL	RasterBuffer::CreateBufferFromTiffData	(void *p_data_src, int size)
{

	VSIFileFromMemBuffer("/vsimem/tiffinmem",(BYTE*)p_data_src,size,0);
	GDALDataset *p_ds = (GDALDataset*) GDALOpen("/vsimem/tiffinmem",GA_ReadOnly);

	CreateBuffer(p_ds->GetRasterCount(),p_ds->GetRasterXSize(),p_ds->GetRasterYSize(),NULL,p_ds->GetRasterBand(1)->GetRasterDataType());
	p_ds->RasterIO(GF_Read,0,0,x_size_,y_size_,p_pixel_data_,x_size_,y_size_,data_type_,num_bands_,NULL,0,0,0); 
	GDALClose(p_ds);
	VSIUnlink("/vsimem/tiffinmem");
	return TRUE;
}


BOOL	RasterBuffer::CreateBufferFromJpegData (void *p_data_src, int size)
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


BOOL	RasterBuffer::CreateBufferFromPngData (void *p_data_src, int size)
{
	gdImagePtr im;
	if (!  (im =	gdImageCreateFromPngPtr(size, p_data_src))) return FALSE;
	
	if (im->alphaBlendingFlag) CreateBuffer(4,im->sx,im->sy,NULL,GDT_Byte,TRUE);
	else CreateBuffer(3,im->sx,im->sy,NULL,GDT_Byte);

	BYTE	*p_pixel_data_byte	= (BYTE*)p_pixel_data_;

	int n = im->sx * im->sy;
	int color, k;
	for (int j=0;j<im->sy;j++)
	{
		for (int i=0;i<im->sx;i++)
		{
			color = (im->tpixels!=NULL) ? im->tpixels[j][i] :
					gdTrueColor(im->red[im->pixels[j][i]],im->green[im->pixels[j][i]],im->blue[im->pixels[j][i]]);
			k = j*x_size_+i;
			if (im->alphaBlendingFlag)
			{
				p_pixel_data_byte[n+ n + n + k] = ((color>>24) > 0) ? 0 : 255;
				color = color & 0xffffff;
			}
			p_pixel_data_byte[k]		= (color>>16) & 0xff;
			p_pixel_data_byte[n+ k]	= (color>>8) & 0xff;
			p_pixel_data_byte[n+ n+ k] = color & 0xff;
		}
	}

	gdImageDestroy(im);
	return TRUE;

}


BOOL RasterBuffer::SaveBufferToFile (string file_name, int quality)
{
	void *p_data_dst = NULL;
	int size = 0;
	BOOL result = SaveBufferToFileAndData(file_name,p_data_dst,size,quality);
	delete[]((BYTE*)p_data_dst);
	return result;
}


BOOL	RasterBuffer::SaveBufferToFileAndData	(string file_name, void* &p_data_dst, int &size, int quality)
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

	BOOL result = TRUE;
	if (!SaveDataToFile(file_name,p_data_dst,size)) result = FALSE;
	return TRUE;
}


BOOL RasterBuffer::ConvertFromPanToRGB ()
{
	if (x_size_==0 || y_size_ == 0 || data_type_ != GDT_Byte) return FALSE;
	if (num_bands_!=1)		return FALSE;
	if (this->p_pixel_data_==NULL)	return FALSE;
	
	BYTE *p_pixel_data_new = new BYTE[3*x_size_*y_size_];
	int n = x_size_*y_size_;
	for (int j=0;j<y_size_;j++)
	{
		for (int i=0;i<x_size_;i++)
		{
			p_pixel_data_new[j*x_size_+i + n + n] =
				(p_pixel_data_new[j*x_size_+i +n] = (p_pixel_data_new[j*x_size_+i] = p_pixel_data_new[j*x_size_+i]));
		}
	}
	
	num_bands_ = 3;
	delete[]((BYTE*)p_pixel_data_);
	p_pixel_data_ = p_pixel_data_new;
	return TRUE;
}



BOOL	RasterBuffer::ConvertFromIndexToRGB ()
{
	if (p_color_table_!=NULL)
	{
		int n = x_size_*y_size_;
		BYTE *p_pixel_data_new = new BYTE[3*n];
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
			}
		}
		delete[]((BYTE*)p_pixel_data_);
		p_pixel_data_ = p_pixel_data_new;
		num_bands_ = 3;
		GDALDestroyColorTable(p_color_table_);
		p_color_table_ = NULL;
	}
	return TRUE;
}


BOOL RasterBuffer::SaveToJpegData (void* &p_data_dst, int &size, int quality)
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
	(void)client_data;
	fprintf(stdout, "[INFO] %s", msg);
}

#ifndef NO_KAKADU
BOOL RasterBuffer::createFromJP2Data (void *p_data_src, int size)
{
 
  kdu_simple_buffer_source input(p_data_src, size);
  kdu_codestream codestream; codestream.create(&input);
  codestream.set_fussy(); 

  kdu_dims dims; codestream.get_dims(0,dims);
  int num_components = codestream.get_num_components();
  
  codestream.apply_input_restrictions(0,num_components,0,0,NULL);
  void *p_buffer = codestream.get_bit_depth(0) == 8 ? (void*)new kdu_byte[(int) dims.area()*num_components] :  (void*)new kdu_int16[(int) dims.area()*num_components];

  kdu_stripe_decompressor decompressor;
  decompressor.start(codestream);

  int *stripe_heights = new int[num_components];
  for (int b=0;b<num_components;b++)
    stripe_heights[b]=dims.size.y;

  int bit_depth = codestream.get_bit_depth(0);
  if (bit_depth == 8) decompressor.pull_stripe((kdu_byte*)p_buffer,stripe_heights);
  else decompressor.pull_stripe((kdu_int16*)p_buffer,stripe_heights);
  
  decompressor.finish();
  codestream.destroy();
  input.close(); //ToDo
  
   
  gmx::RasterBuffer rbuf;
  if (bit_depth == 8) 
  {
    CreateBuffer(num_components,dims.size.x,dims.size.y,NULL,GDT_Byte,FALSE);
    kdu_byte *p_pixel_data_byte = (kdu_byte*)p_pixel_data_;
    kdu_byte *p_buffer_byte = (kdu_byte*)p_buffer;

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
          p_pixel_data_byte[area*b + area2 + i] = p_buffer_byte[n];
          n++;
        }
      }
    }
  }
  else
  {
    CreateBuffer(num_components,dims.size.x,dims.size.y,NULL,GDT_UInt16,FALSE);
    kdu_uint16 *p_pixel_data_uint16 = (kdu_uint16*)p_pixel_data_;
    kdu_uint16 *p_buffer_uint16 = (kdu_uint16*)p_buffer;

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
          p_pixel_data_uint16[area*b + area2 + i] = p_buffer_uint16[n];
          n++;
        }
      }
    }

  }
    

  delete[] p_buffer;
  return TRUE;
}
#endif

#ifdef NO_KAKADU
BOOL RasterBuffer::createFromJP2Data (void *p_data_src, int size)
{
  ClearBuffer();
  if (size>10000000) return FALSE;
  OPJStreamData opj_stream_data;
  opj_stream_data.max_size = (opj_stream_data.size=size);
  opj_stream_data.offset = 0;
  opj_stream_data.p_data = new BYTE[size];
  memcpy(opj_stream_data.p_data,p_data_src,size);
  opj_stream_t *l_stream = OPJStreamCreate(&opj_stream_data,10000000,TRUE);


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
	opj_set_info_handler(l_codec, info_callback,00);
	opj_set_warning_handler(l_codec, warning_callback,00);
	opj_set_error_handler(l_codec, error_callback,00);


	// Setup the decoder decoding parameters using user parameters //
	if ( !opj_setup_decoder(l_codec, &parameters) ){
		fprintf(stderr, "ERROR -> j2k_dump: failed to setup the decoder\n");
		opj_stream_destroy(l_stream);
		//fclose(fsrc);
		opj_destroy_codec(l_codec);
		return EXIT_FAILURE;
	}


	// Read the main header of the codestream and if necessary the JP2 boxes //
	if(! opj_read_header(l_stream, l_codec, &image)){
		fprintf(stderr, "ERROR -> opj_decompress: failed to read the header\n");
		opj_stream_destroy(l_stream);
	  delete[]opj_stream_data.p_data;
		opj_destroy_codec(l_codec);
		opj_image_destroy(image);
		return EXIT_FAILURE;
	}

	// Optional if you want decode the entire image //
	if (!opj_set_decode_area(l_codec, image, parameters.DA_x0,
			parameters.DA_y0, parameters.DA_x1, parameters.DA_y1)){
		fprintf(stderr,	"ERROR -> opj_decompress: failed to set the decoded area\n");
		opj_stream_destroy(l_stream);
		opj_destroy_codec(l_codec);
		opj_image_destroy(image);
    delete[]opj_stream_data.p_data;
		return EXIT_FAILURE;
	}

  //opj_stream_private 
	// Get the decoded image //

	if (!(opj_decode(l_codec, l_stream, image) )) {//&& opj_end_decompress(l_codec,	l_stream)
		fprintf(stderr,"ERROR -> opj_decompress: failed to decode image!\n");
		opj_destroy_codec(l_codec);
		opj_stream_destroy(l_stream);
		opj_image_destroy(image);
    delete[]opj_stream_data.p_data;
    return EXIT_FAILURE;
	}

  opj_destroy_codec(l_codec);
	opj_stream_destroy(l_stream);
	delete[]opj_stream_data.p_data;

  this->CreateBuffer(image->numcomps,
                      image->comps[0].w,
                      image->comps[0].h,
                      NULL,
                      (image->comps[0].prec == 8) ? GDT_Byte : GDT_UInt16,
                      FALSE,
                      NULL);

  int area = x_size_*y_size_;
  int index=0;
  if (data_type_==GDT_Byte)
  {
    BYTE *p_pixel_data = (BYTE*)p_pixel_data_;
    for (int i=0;i<y_size_;i++)
    {
      for (int j=0;j<x_size_;j++)
      {
        for (int k=0;k<num_bands_;k++)
        {
          p_pixel_data[k*area+index] = image->comps[k].data[index];
        }
        index++;
      }
    }
  }
  else
  {
    unsigned __int16 *p_pixel_data = (unsigned __int16*)p_pixel_data_;
    for (int i=0;i<y_size_;i++)
    {
      for (int j=0;j<x_size_;j++)
      {
        for (int k=0;k<num_bands_;k++)
        {
          p_pixel_data[k*area+index] = image->comps[k].data[index];
        }
        index++;
      }
    }
  }
  opj_image_destroy(image);

 	return TRUE;
}
#endif

#ifndef NO_KAKADU
BOOL RasterBuffer::SaveToJP2Data	(void* &p_data_dst, int &size, int compression_rate)
{
  int num_components=0, height, width;
  num_components = num_bands_;
  height = y_size_;
  width = x_size_;
 
  void *p_data_order2 = GetPixelDataOrder2();

  siz_params siz;
  siz.set(Scomponents,0,0,num_components);
  siz.set(Sdims,0,0,height); 
  siz.set(Sdims,0,1,width);  
  if (data_type_ == GDT_Byte)
    siz.set(Sprecision,0,0,8);
  else if ((data_type_ == GDT_Int16)||((data_type_ == GDT_UInt16)))
    siz.set(Sprecision,0,0,16);
 
  siz.set(Ssigned,0,0,false);
  kdu_params *siz_ref = &siz; 

  siz_ref->finalize();

  kdu_simple_buffer_target output(1000000);

  kdu_codestream codestream; codestream.create(&siz,&output);
  codestream.access_siz()->parse_string("Clayers=1");
  if ((data_type_ == GDT_Int16)||((data_type_ == GDT_UInt16)))
    codestream.access_siz()->parse_string("Qstep=0.002");
  
  codestream.access_siz()->finalize_all(); 
  
  kdu_stripe_compressor compressor;
  kdu_long *layer_sizes = new kdu_long[1];
  
  if ((compression_rate<=0) || (compression_rate>100)) compression_rate=10;
  else switch ((100-compression_rate)/5)
  {
    case 0:
      compression_rate = 2;
      break;
    case 1:
      compression_rate = 3;
      break;
    case 2:
      compression_rate = 5;
      break;
    case 3:
      compression_rate = 10;
      break;
    case 4:
      compression_rate = 20;
      break;
    case 5:
      compression_rate = 30;
      break;
    case 6:
      compression_rate = 50;
      break;
    default:
      compression_rate = 100;
      break;
  }
  layer_sizes[0] = (((double)(num_bands_*256*256*(2-(data_type_==GDT_Byte))))/compression_rate);
  compressor.start(codestream,1,layer_sizes);

  int * stripe_heights= new int[num_components];
  for (int b=0; b<num_components;b++)
    stripe_heights[b]=height;
  switch (data_type_)
  {
    case GDT_Byte:
      compressor.push_stripe((kdu_byte*)p_data_order2,stripe_heights);
      break;
    case GDT_Int16:
      compressor.push_stripe((kdu_int16*)p_data_order2,stripe_heights);
      break;
    case GDT_UInt16:
      compressor.push_stripe((kdu_int16*)p_data_order2,stripe_heights);
      break;
    /*
    case GDT_Float32:
      compressor.push_stripe((kdu_float32*)p_data_order2,stripe_heights);
      break;
    */
    default:
      break;
  }
  
  
  compressor.finish();
  codestream.destroy();
  size = output.cur_pos_;
  p_data_dst = new BYTE[size];
  memcpy(p_data_dst,output.p_buffer_data_,size);
  output.close();
  delete[]p_data_order2;

  return TRUE;
}
#endif NO_KAKADU

#ifdef NO_KAKADU

BOOL RasterBuffer::SaveToJP2Data	(void* &p_data_dst, int &size, int compression_rate)
{
	//int subsampling_dx;
	//int subsampling_dy;
  if ((compression_rate<=0) || (compression_rate>100)) compression_rate=10;
  else switch ((100-compression_rate)/5)
  {
    case 0:
      compression_rate = 2;
      break;
    case 1:
      compression_rate = 3;
      break;
    case 2:
      compression_rate = 5;
      break;
    case 3:
      compression_rate = 10;
      break;
    case 4:
      compression_rate = 20;
      break;
    case 5:
      compression_rate = 30;
      break;
    case 6:
      compression_rate = 50;
      break;
    default:
      compression_rate = 100;
      break;
  }
    
	int j, numcomps, w, h,index;
  OPJ_COLOR_SPACE color_space;
	opj_image_cmptparm_t cmptparm[4]; // RGBA //
	opj_image_t *image = NULL;
	int imgsize = 0;
	int has_alpha = 0;
	
	w		= this->get_x_size();
	h		= this->get_y_size();
 	memset(&cmptparm[0], 0, 4 * sizeof(opj_image_cmptparm_t));
	
	numcomps = num_bands_;
	color_space = (num_bands_>=3) ? OPJ_CLRSPC_SRGB : OPJ_CLRSPC_GRAY;
	for(j = 0; j < numcomps; j++) 
	{
		cmptparm[j].prec= (data_type_ == GDT_Byte) ? 8: 16;
		cmptparm[j].bpp	= (data_type_ == GDT_Byte) ? 8: 16;
 		cmptparm[j].dx	= 1;	
		cmptparm[j].dy	= 1;
		cmptparm[j].w	= w;
		cmptparm[j].h	= h;
	}

	image = opj_image_create(numcomps, &cmptparm[0], color_space);
	image->x0 = 0;
	image->y0 = 0;
	image->x1 =	w;
	image->y1 =	h;

	index = 0;
	imgsize = image->comps[0].w * image->comps[0].h ;
	if (data_type_ == GDT_Byte)
	{
    BYTE	*pData8 = (BYTE*)p_pixel_data_;
		for(int i=0; i<h; i++) 
		{
			for(j = 0; j < w; j++) 
			{
				for (int k=0;k<num_bands_;k++)
					image->comps[k].data[index] = pData8[index + k*imgsize];
				index++;
			}
		}
	}
	else 
	{
		unsigned __int16	*pData16 = (unsigned __int16*) p_pixel_data_;
		for(int i=0; i<h; i++) 
		{
			for(j = 0; j < w; j++) 
			{
				for (int k=0;k<num_bands_;k++)
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
  parameters.tcp_mct = image->numcomps == 3 ? 1 : 0;
	l_codec = opj_create_compress(OPJ_CODEC_JP2);

	opj_set_info_handler(l_codec, info_callback,00);
  opj_set_warning_handler(l_codec, warning_callback,00);
  opj_set_error_handler(l_codec, error_callback,00);

	parameters.tcp_numlayers = 1;
	parameters.cod_format = 0;
	parameters.cp_disto_alloc = 1;
  parameters.tcp_rates[0]=compression_rate;
  bool r = opj_setup_encoder(l_codec, &parameters, image);

  OPJStreamData opj_stream_data;
  opj_stream_data.max_size = 10000000;
  opj_stream_data.offset = (opj_stream_data.size = 0);
  opj_stream_data.p_data = new BYTE[opj_stream_data.max_size];
  l_stream = OPJStreamCreate(&opj_stream_data,10000000,FALSE);

	bSuccess = opj_start_compress(l_codec,image,l_stream);
	if (!bSuccess) 
    fprintf(stderr, "failed to encode image: opj_start_compress\n");
    
 bSuccess = bSuccess && opj_encode(l_codec, l_stream);
 if (!bSuccess)
   fprintf(stderr, "failed to encode image: opj_encode\n");
    
	bSuccess = bSuccess && opj_end_compress(l_codec, l_stream);
  if(!bSuccess) 
    fprintf(stderr, "failed to encode image: opj_end_compress\n");
  
  opj_destroy_codec(l_codec);
  opj_image_destroy(image);
  opj_stream_destroy(l_stream);
 
  p_data_dst = opj_stream_data.p_data;
  size = opj_stream_data.size;
  if(parameters.cp_comment)   free(parameters.cp_comment);
  if(parameters.cp_matrice)   free(parameters.cp_matrice);
  
	return TRUE;
}
#endif

BOOL RasterBuffer::SaveToPng24Data (void* &p_data_dst, int &size)
{
	if ((x_size_ ==0)||(y_size_ == 0)||(data_type_!=GDT_Byte)) return FALSE;
	
	gdImagePtr im		= gdImageCreateTrueColor(x_size_,y_size_);
	
	BYTE	*p_pixel_data_byte	= (BYTE*)p_pixel_data_;
	int n = (num_bands_ == 3 || num_bands_ == 4) ? 	y_size_*x_size_ : 0;
	int transparency = 0;

	if ((num_bands_ == 4) || (num_bands_ == 2))
	{
		im->alphaBlendingFlag	= 1;
		im->saveAlphaFlag		= 1;
		int n = (num_bands_ == 4) ? 	y_size_*x_size_ : 0;
		int opaque = 0;
    int l;
		for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++){
        im->tpixels[j][i] = gdTrueColorAlpha(p_pixel_data_byte[j*x_size_+i],
									p_pixel_data_byte[j*x_size_+i+n],
									p_pixel_data_byte[j*x_size_+i+n + n],
									(p_pixel_data_byte[j*x_size_+ i + n + n + y_size_*x_size_ ] >= 100) ? 0 : 127);

      }
		}
	}
	else // ((num_bands_ == 3) || (num_bands_ == 1))
	{
		im->alphaBlendingFlag	= 0;
		im->saveAlphaFlag		= 0;
		for (int j=0;j<y_size_;j++)
		{
			for (int i=0;i<x_size_;i++)
				im->tpixels[j][i] = gdTrueColor(p_pixel_data_byte[j*x_size_+i],p_pixel_data_byte[j*x_size_+i+n],p_pixel_data_byte[j*x_size_+i+n+n]);
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
	
BOOL RasterBuffer::SaveToPngData (void* &p_data_dst, int &size)
{

	if ((x_size_ ==0)||(y_size_ == 0)||(data_type_!=GDT_Byte)) return FALSE;
	if (p_color_table_==NULL) return SaveToPng24Data(p_data_dst,size);
	
	gdImagePtr im	= gdImageCreate(x_size_,y_size_);
	im->colorsTotal = p_color_table_->GetColorEntryCount();
	//im->
	for (int i=0;(i<im->colorsTotal)&&(i<gdMaxColors);i++)
	{
		const GDALColorEntry *p_color_entry = p_color_table_->GetColorEntry(i);
		
		im->red[i]		= p_color_entry->c1;
		im->green[i]	= p_color_entry->c2;
		im->blue[i]		= p_color_entry->c3;
		im->open[i]		= 0;
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


BOOL	RasterBuffer::SaveToTiffData	(void* &p_data_dst, int &size)
{
  
  srand(999);
  string			tiff_in_mem = ("/vsimem/tiffinmem" + ConvertIntToString(rand()));

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
	
	//SaveDataToFile("e:\\1.tif",p_data_dst,size);
	//delete[]p_data_dst;
	//ToDo: delete p_ds
	return TRUE;
}





/*
BOOL  RasterBuffer::MergeUsingBlack (RasterBuffer oBackGround, RasterBuffer &oMerged)
{
	if ((this->get_x_size()!=oBackGround.get_x_size())||
		(this->get_y_size()!=oBackGround.get_y_size())||
		(num_bands_!=oBackGround.get_num_bands()))
	{
		return FALSE;
	}

	oMerged.CreateBuffer(num_bands_,x_size_,y_size_);
	BYTE *pMergedData = (BYTE*)oMerged.getData();

	memcpy(pMergedData,oBackGround.getData(),x_size_*y_size_*num_bands_);

	int k,s;
	int n = x_size_*y_size_;

	for (int i=0;i<y_size_;i++)
	{
		int l = i*x_size_;
		for (int j=0;j<x_size_;j++)
		{
			s=0;
			for (k=0;k<num_bands_;k++)
			{
				if (this->p_data[l+j+s]!=0) break;
				s+=n;
			}
			if (k==num_bands_) 
			{
				s=0;
				for (k=0;k<num_bands_;k++)
				{
					pMergedData[l+j+s]!=this->p_data[l+j+s];
					s+=n;
				}
			}
		}
	}
	return TRUE;
}
*/

/*
BOOL	RasterBuffer::ResizeAndConvertToRGB	(int nNewWidth, int nNewHeight)
{
	if ((x_size_==0)||(y_size_==0)) return FALSE;
	if (this->pTable!=NULL) ConvertFromIndexToRGB();

	gdImagePtr im	= gdImageCreateTrueColor(x_size_,y_size_);
	int n = x_size_*y_size_;
	if (num_bands_==1) n =0;
	int color = 0;
	for (int j=0;j<y_size_;j++)
	{
		for (int i=0;i<x_size_;i++)
		{
			color = 65536*this->p_data[j*x_size_+i] + 256*this->p_data[j*x_size_+i+n]+this->p_data[j*x_size_+i+n+n];
			im->tpixels[j][i] = color;
		}
	}
	gdImagePtr im_out;
	im_out = gdImageCreateTrueColor(nNewWidth,nNewHeight);
	gdImageCopyResampled(im_out, im, 0, 0, 0, 0, im_out->sx, im_out->sy, im->sx, im->sy);  
	BYTE	*pDataOut = new BYTE[3*nNewWidth*nNewHeight];
	n=nNewWidth*nNewHeight;
	int num;
	for (int j=0;j<nNewHeight;j++)
	{
		for (int i=0;i<nNewWidth;i++)
		{
			color = im_out->tpixels[j][i];
			num = j*nNewWidth+i;
			pDataOut[num] = color/65536;
			color-=pDataOut[num]*65536;
			pDataOut[n+num] = color/256;
			color-=256*pDataOut[n+num];
			pDataOut[n+n+num] = color;
		}
	}

	gdImageDestroy(im);
	gdImageDestroy(im_out);
	delete[]p_data;
	p_data = pDataOut;
	x_size_ = nNewWidth;
	y_size_ = nNewHeight;

	return TRUE;
}
*/



/*
BOOL RasterBuffer::makeZero(LONG nLeft, LONG nTop, LONG nWidth, LONG nHeight, LONG nNoDataValue)
{
	if (p_data==NULL) return FALSE;
	if ((nLeft<0)||(nLeft+nWidth>x_size_)) return FALSE;
	if ((nTop<0)||(nTop+nHeight>y_size_)) return FALSE;
	
	LONG n = x_size_*y_size_;

	int n1,n2;
	n1=0;
	for (int j=nTop;j<nTop+nHeight;j++)
	{	
		n1=j*x_size_;
		for (int i=nLeft;i<nLeft+nWidth;i++)
		{
			n2=0;			
			for (int k=0;k<num_bands_;k++)
			{
				((BYTE*)p_data)[n1+i+n2] = nNoDataValue;
				n2+=n;
			}
		}	
	}
	return TRUE;	
}
*/


BOOL RasterBuffer::InitByValue(int value)
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
BOOL RasterBuffer::InitByValue(T type, int value)
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



BOOL	RasterBuffer::StretchDataTo8Bit(double *minValues, double *maxValues)
{
	void *p_pixel_data_new = GetPixelDataBlock(0,0,x_size_,y_size_,TRUE,minValues,maxValues);
	int bands = num_bands_;
	int width = x_size_;
	int height = y_size_;
	ClearBuffer();
	if (!CreateBuffer(bands,width,height,p_pixel_data_new,GDT_Byte)) return FALSE;
	delete[]((BYTE*)p_pixel_data_new);

	return TRUE;
}


void*	RasterBuffer::GetPixelDataBlock (int left, int top, int w, int h,  BOOL stretchTo8Bit, double *minValues, double *maxValues)
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
			return GetPixelDataBlock(t,left,top,w,h,stretchTo8Bit,minValues,maxValues);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return GetPixelDataBlock(t,left,top,w,h,stretchTo8Bit,minValues,maxValues);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return GetPixelDataBlock(t,left,top,w,h,stretchTo8Bit,minValues,maxValues);
		}
		default:
			return NULL;
	}
	return NULL;
}

///*
template <typename T>
void*	RasterBuffer::GetPixelDataBlock (T type, int left, int top, int w, int h,  
									BOOL stretch_to_8bit, double *p_min_values, double *p_max_values)
{
	if (num_bands_==0) return NULL;
	int					n = w*h;
	unsigned __int64	m = x_size_*y_size_;
	T				*p_pixel_block_t;
	BYTE			*p_pixel_block_byte;

	if (stretch_to_8bit) p_pixel_block_byte	= new BYTE[num_bands_*n];
	else p_pixel_block_t		= new T[num_bands_*n];

	
	T *p_pixel_data_t= (T*)p_pixel_data_;
	double d;
	for (int k=0;k<num_bands_;k++)
	{
		for (int j=left;j<left+w;j++)
		{
			for (int i=top;i<top+h;i++)
			{
				if (stretch_to_8bit)
				{
          if (p_pixel_data_t[m*k+i*x_size_+j] == 0) p_pixel_block_byte[n*k+(i-top)*w+j-left] = 0;
          else
          {  
					  d = max(min(p_pixel_data_t[m*k+i*x_size_+j],p_max_values[k]),p_min_values[k]);
					  p_pixel_block_byte[n*k+(i-top)*w+j-left] = (int)(0.5+255*((d-p_min_values[k])/(p_max_values[k]-p_min_values[k])));
          }
				}
				else p_pixel_block_t[n*k+(i-top)*w+j-left] = p_pixel_data_t[m*k+i*x_size_+j];
			}
		}
	}			

	if (stretch_to_8bit)	return p_pixel_block_byte;
	else return p_pixel_block_t;
}
//*/


BOOL		RasterBuffer::AddPixelDataToHistogram(Histogram *p_hist)
{
	switch (data_type_)
	{
		case GDT_Byte:
		{
			BYTE t = 1;
			return AddPixelDataToHistogram(t,p_hist);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return AddPixelDataToHistogram(t,p_hist);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return AddPixelDataToHistogram(t,p_hist);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return AddPixelDataToHistogram(t,p_hist);
		}
		default:
			return NULL;
	}
}

void*		RasterBuffer::GetDataZoomedOut	()
{
	void *p_pixel_data_zoomedout;
	switch (data_type_)
	{
		case GDT_Byte:
		{
			BYTE t = 1;
			return GetDataZoomedOut(t);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return GetDataZoomedOut(t);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return GetDataZoomedOut(t);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return GetDataZoomedOut(t);
		}
		default:
			return NULL;
	}
}

template<typename T>
void* RasterBuffer::GetDataZoomedOut	(T type)
{
	unsigned int a	= x_size_*y_size_/4;
	unsigned w = x_size_/2;
	unsigned h = y_size_/2;
	T	*p_pixel_data_zoomedout	= NULL;
	if(! (p_pixel_data_zoomedout = new T[num_bands_*a]) )
	{
		return NULL;
	}
	unsigned int m =0;
	T	*p_pixel_data_t		= (T*)p_pixel_data_;

	for (int k=0;k<num_bands_;k++)
	{
		for (int i=0;i<h;i++)
		{
			for (int j=0;j<w;j++)
			{
				p_pixel_data_zoomedout[m + i*w + j] = (	p_pixel_data_t[(m<<2) + (i<<1)*(x_size_) + (j<<1)]+
												p_pixel_data_t[(m<<2) + ((i<<1)+1)*(x_size_) + (j<<1)]+
												p_pixel_data_t[(m<<2) + (i<<1)*(x_size_) + (j<<1) +1]+
												p_pixel_data_t[(m<<2) + ((i<<1)+1)*(x_size_) + (j<<1) + 1])/4;
			}
		}
		m+=a;
	}
	return p_pixel_data_zoomedout;
}


template <typename T>	
BOOL    RasterBuffer::AddPixelDataToHistogram(T type, Histogram *p_hist)
{
  T *p_pixel_data_t = (T*)p_pixel_data_;
  unsigned int n = x_size_*y_size_;
  unsigned int m = 0;
  for (int i=0;i<y_size_;i++)
  {
    for (int j=0;j<x_size_;j++)
    {
      for (int k=0;k<num_bands_;k++)
      {
        p_hist->AddValue(k,p_pixel_data_t[k*n+m]);
      }
      m++;
    }
  }
  return TRUE;
}

/*
BOOL	RasterBuffer::dataIO	(BOOL operationFlag, 
								int left, int top, int w, int h, 
								void *p_data, 
								int bands = 0, BOOL stretchTo8Bit = FALSE, double min = 0, double max = 0)
{
	if (operationFlag==FALSE)
	{
		void *pData_ = copyData(left,top,w,h);
	}
	else
	{
		return setData(left,top,w,h,p_data,bands);

	}
	return TRUE;
}
*/
BOOL	RasterBuffer::IsAlphaBand()
{
	return alpha_band_defined_;
}


BOOL  RasterBuffer::CreateAlphaBandByPixelLinePolygon (VectorBorder *p_vb)
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

    if ((!VectorBorder::CalcIntersectionBetweenLineAndPixelLineGeometry(j,p_vb->get_ogr_geometry_ref(),num_points,p_x)) || 
        (num_points == 0) || ((num_points%2)==1))
      continue;
            
    for (int k=0;k<num_points;k+=2)
    {
      for (int i=p_x[k];i<=p_x[k+1];i++)
      {
        if (i<0) continue;
        if (i>=x_size_) break;
        vector_mask[n+i]=1;
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



BOOL	RasterBuffer::CreateAlphaBandByRGBColor(BYTE	*pRGB, int tolerance)
{
	if ((this-p_pixel_data_ == NULL) || (data_type_!=GDT_Byte) || (num_bands_>4) || (p_color_table_)) return FALSE;

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

//BOOL	RasterBuffer::createAlphaBandByValue(int	value);


BOOL	RasterBuffer::SetPixelDataBlock (int left, int top, int w, int h, void *p_block_data, int bands)
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
BOOL	RasterBuffer::SetPixelDataBlock (T type, int left, int top, int w, int h, void *p_block_data, int bands)
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


BOOL	RasterBuffer::set_color_table (GDALColorTable *p_color_table)
{
	p_color_table_ = p_color_table->Clone();
	
	return TRUE;
}


GDALColorTable*	RasterBuffer::get_color_table_ref ()
{
	return p_color_table_;
}


}