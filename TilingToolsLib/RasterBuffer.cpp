#include "StdAfx.h"
#include "RasterBuffer.h"
#include "FileSystemFuncs.h"



RasterBuffer::RasterBuffer(void)
{
	pData = NULL;	
	pTable = NULL;
	bAlphaBand	= FALSE;
	pNoDataValue = NULL;
}


RasterBuffer::~RasterBuffer(void)
{
	clearBuffer();
}


int* RasterBuffer::getNoDataValue()
{
	return this->pNoDataValue;
}


BOOL RasterBuffer::setNoDataValue(int noDataValue)
{
	this->pNoDataValue = new int[1];
	this->pNoDataValue[0] = noDataValue;
	return TRUE;
}



void RasterBuffer::clearBuffer()
{
	delete(pNoDataValue);
	pNoDataValue = NULL;
	if (pData!=NULL)
	{
		switch (dataType)
		{
			case GDT_Byte:
				delete[]((BYTE*)pData);
				break;
			case GDT_UInt16:
				delete[]((unsigned __int16*)pData);
				break;
			case GDT_Int16:
				delete[]((__int16*)pData);
				break;
			case GDT_Float32:
				delete[]((float*)pData);
				break;
		}
		pData = NULL;
	}
	
	if (pTable!=NULL)
	{
		GDALDestroyColorTable(pTable);
		pTable = NULL;
	}	
}



BOOL RasterBuffer::createBuffer	(int			nBands,
								 int			nXSize,
								 int			nYSize,
								 void			*pDataSrc,
								 GDALDataType	dataType,
								 int			*pNoDataValue,
								 BOOL			bAlphaBand	
								 )
{
	clearBuffer();

	this->nBands		= nBands;
	this->nXSize		= nXSize;
	this->nYSize		= nYSize;
	this->dataType		= dataType;
	this->bAlphaBand	= bAlphaBand;
	if (pNoDataValue) setNoDataValue(pNoDataValue[0]);

	switch (dataType)
	{
		case GDT_Byte:
				this->pData = new BYTE[nBands*nXSize*nYSize];
				this->dataSize = 1;
				break;
		case GDT_UInt16:
				this->pData = new unsigned __int16[nBands*nXSize*nYSize];
				this->dataSize = 2;
				break;
		case GDT_Int16:
				this->pData = new __int16[nBands*nXSize*nYSize];
				this->dataSize = 3;
				break;
		case GDT_Float32:
				this->pData = new float[nBands*nXSize*nYSize];
				this->dataSize = 4;
				break;			
		default:
			return FALSE;
	}

	if (pDataSrc !=NULL)				memcpy(this->pData,pDataSrc,dataSize*nBands*nXSize*nYSize);
	else	if (this->pNoDataValue) this->initByValue(this->pNoDataValue[0]);
	else	this->initByValue(0);

	return TRUE;
}


BOOL RasterBuffer::createBuffer		(RasterBuffer *pSrcBuffer)
{
	if (!createBuffer(	pSrcBuffer->getBandsCount(),
						pSrcBuffer->getXSize(),
						pSrcBuffer->getYSize(),
						pSrcBuffer->getDataRef(),
						pSrcBuffer->getDataType(),
						pSrcBuffer->pNoDataValue,						
						pSrcBuffer->bAlphaBand
						)) return FALSE;

	if (pSrcBuffer->pTable) this->setColorMeta(pSrcBuffer->pTable);
	return TRUE;	
}



BOOL RasterBuffer::initByRGBColor	 (BYTE rgb[3])
{
	if (this->dataType != GDT_Byte) return FALSE;
	if (this->pData == NULL) return FALSE;

	BYTE *pDataByte = (BYTE*)pData;
	__int64 n = this->nXSize*this->nYSize;
	if (this->nBands < 3)
	{
		for (__int64 i = 0;i<n;i++)
			pDataByte[i] = rgb[0];
	}
	else
	{
		for (__int64 i = 0;i<n;i++)
		{
			pDataByte[i]		= rgb[0];
			pDataByte[i+n]		= rgb[1];
			pDataByte[i+n+n]	= rgb[2];
		}
	}
	return TRUE;
}

			

BOOL	RasterBuffer::createBufferFromTiffData	(void *pDataSrc, int size)
{

	VSIFileFromMemBuffer("/vsimem/tiffinmem",(BYTE*)pDataSrc,size,0);
	GDALDataset *poDS = (GDALDataset*) GDALOpen("/vsimem/tiffinmem",GA_ReadOnly);

	createBuffer(poDS->GetRasterCount(),poDS->GetRasterXSize(),poDS->GetRasterYSize(),NULL,poDS->GetRasterBand(1)->GetRasterDataType());
	poDS->RasterIO(GF_Read,0,0,nXSize,nXSize,pData,nXSize,nYSize,dataType,nBands,NULL,0,0,0); 
	int	isNoData;
	int noDataValue = poDS->GetRasterBand(1)->GetNoDataValue(&isNoData);
	if (isNoData) setNoDataValue(noDataValue);

	GDALClose(poDS);
	VSIUnlink("/vsimem/tiffinmem");
	return TRUE;
}


BOOL	RasterBuffer::createBufferFromJpegData (void *pDataSrc, int size)
{

	clearBuffer();
	gdImagePtr im;
	if (!  (im =	gdImageCreateFromJpegPtr(size, pDataSrc))) return FALSE;
	
	createBuffer(3,im->sx,im->sy,NULL,GDT_Byte);
	//gdImageDestroy(im);
	//return TRUE;
	BYTE	*pDataB	= (BYTE*)pData;

	
	int n = im->sx * im->sy;
	int color, k;
	for (int j=0;j<im->sy;j++)
	{
		for (int i=0;i<im->sx;i++)
		{
			color = im->tpixels[j][i];
			k = j*this->nXSize+i;		
			pDataB[k]		= (color>>16) & 0xff;
			pDataB[n+ k]	= (color>>8) & 0xff;
			pDataB[n+ n+ k] = color & 0xff;
		}
	}

	gdImageDestroy(im);
	return TRUE;

}


BOOL	RasterBuffer::createBufferFromPngData (void *pDataSrc, int size)
{
	gdImagePtr im;
	if (!  (im =	gdImageCreateFromPngPtr(size, pDataSrc))) return FALSE;
	
	if (im->alphaBlendingFlag) createBuffer(4,im->sx,im->sy,NULL,GDT_Byte,NULL,TRUE);
	else createBuffer(3,im->sx,im->sy,NULL,GDT_Byte);

	BYTE	*pDataB	= (BYTE*)pData;

	int n = im->sx * im->sy;
	int color, k;
	for (int j=0;j<im->sy;j++)
	{
		for (int i=0;i<im->sx;i++)
		{
			color = im->tpixels[j][i];
			k = j*this->nXSize+i;
			if (im->alphaBlendingFlag)
			{
				pDataB[n+ n + n + k] = ((color>>24) >=100) ? 100 : 0;
				color = color & 0xffffff;
			}
			pDataB[k]		= (color>>16) & 0xff;
			pDataB[n+ k]	= (color>>8) & 0xff;
			pDataB[n+ n+ k] = color & 0xff;
		}
	}

	gdImageDestroy(im);
	return TRUE;

}


BOOL RasterBuffer::SaveBufferToFile (wstring fileName, int quality)
{
	void *pDataDst = NULL;
	int size = 0;
	BOOL result = SaveBufferToFileAndData(fileName,pDataDst,size,quality);
	delete[]((BYTE*)pDataDst);
	return result;
}


BOOL	RasterBuffer::SaveBufferToFileAndData	(wstring fileName, void* &pDataDst, int &size, int quality)
{
	int n_jpg = fileName.find(L".jpg");
	int n_png = fileName.find(L".png");
	int n_tif = fileName.find(L".tif");
	
	
	if (n_jpg>0)
	{
		if (!SaveToJpegData(quality,pDataDst,size)) return FALSE;
	}
	else if (n_png>0)
	{
		if (!SaveToPngData(pDataDst,size)) return FALSE;
	}
	else if (n_tif>0)
	{
		if (!SaveToTiffData(pDataDst,size)) return FALSE;
	}
	else return FALSE;

	BOOL result = TRUE;
	if (!SaveDataToFile(fileName,pDataDst,size)) result = FALSE;
	return TRUE;
}


BOOL RasterBuffer::convertFromPanToRGB ()
{
	if (this->nXSize==0 || this->nYSize == 0 || this->dataType != GDT_Byte) return FALSE;
	if (this->nBands!=1)		return FALSE;
	if (this->pData==NULL)	return FALSE;
	
	BYTE *pDataNew = new BYTE[3*this->nXSize*this->nYSize];
	int n = this->nXSize*this->nYSize;
	for (int j=0;j<this->nYSize;j++)
	{
		for (int i=0;i<this->nXSize;i++)
		{
			pDataNew[j*this->nXSize+i + n + n] =
				(pDataNew[j*this->nXSize+i +n] = (pDataNew[j*this->nXSize+i] = pDataNew[j*this->nXSize+i]));
		}
	}
	
	this->nBands = 3;
	delete[]((BYTE*)pData);
	pData = pDataNew;
	return TRUE;
}



BOOL	RasterBuffer::convertFromIndexToRGB ()
{
	if (this->pTable!=NULL)
	{
		int n = this->nXSize*this->nYSize;
		BYTE *pDataNew = new BYTE[3*n];
		int m;
		const GDALColorEntry *pCEntry;
		for (int i=0;i<this->nXSize;i++)
		{
			for (int j=0;j<this->nYSize;j++)
			{
				m = j*this->nXSize+i;
				pCEntry = pTable->GetColorEntry(((BYTE*)pData)[m]);
				pDataNew[m] = pCEntry->c1;
				pDataNew[m+n] = pCEntry->c2;
				pDataNew[m+n+n] = pCEntry->c3;
			}
		}
		delete[]((BYTE*)pData);
		this->pData = pDataNew;
		this->nBands = 3;
		GDALDestroyColorTable(this->pTable);
		this->pTable = NULL;
	}
	return TRUE;
}


BOOL RasterBuffer::SaveToJpegData (int quality, void* &pDataDst, int &size)
{
	
	if ((this->nXSize ==0)||(this->nYSize == 0)||(this->dataType!=GDT_Byte)) return FALSE;
	gdImagePtr im	= gdImageCreateTrueColor(this->nXSize,this->nYSize);
	
	int n = (this->nBands < 3) ? 0 : this->nXSize*this->nYSize;
	int color = 0;
	BYTE	*pDataB = (BYTE*)pData;
	for (int j=0;j<this->nYSize;j++)
	{
		for (int i=0;i<this->nXSize;i++)
			im->tpixels[j][i] = gdTrueColor(pDataB[j*this->nXSize+i],pDataB[j*this->nXSize+i+n],pDataB[j*this->nXSize+i+n+n]);
	}
	
	if (!(pDataDst = (BYTE*)gdImageJpegPtr(im,&size,quality)))
	{
		gdImageDestroy(im);
		return FALSE;
	}

	gdImageDestroy(im);
	return TRUE;
}


BOOL RasterBuffer::SaveToPng24Data (void* &pDataDst, int &size)
{
	if ((this->nXSize ==0)||(this->nYSize == 0)||(this->dataType!=GDT_Byte)) return FALSE;
	
	gdImagePtr im		= gdImageCreateTrueColor(this->nXSize,this->nYSize);
	
	BYTE	*pDataByte	= (BYTE*)pData;
	int n = (this->nBands == 3 || this->nBands == 4) ? 	this->nYSize*this->nXSize : 0;
	int opaque = 0;

	if ((this->nBands == 4) || (this->nBands == 2))
	{
		im->alphaBlendingFlag	= 1;
		im->saveAlphaFlag		= 1;
		int n = (this->nBands == 4) ? 	this->nYSize*this->nXSize : 0;
		int opaque = 0;
		for (int j=0;j<this->nYSize;j++)
		{
			for (int i=0;i<this->nXSize;i++)
				im->tpixels[j][i] = gdTrueColorAlpha(pDataByte[j*this->nXSize+i],
									pDataByte[j*this->nXSize+i+n],
									pDataByte[j*this->nXSize+i+n + n],
									(pDataByte[j*this->nXSize+ i + n + n + this->nYSize*this->nXSize] >0) ? 127 : 0);
		}
	}
	else if (this->pNoDataValue )	// ((this->nBands == 3) || (this->nBands == 1))
	{
		im->alphaBlendingFlag = 1;
		im->saveAlphaFlag	= 1;
		
		for (int j=0;j<this->nYSize;j++)
		{
			for (int i=0;i<this->nXSize;i++)
			{
				opaque = (	(pDataByte[j*this->nXSize+i]			== this->pNoDataValue[0]) &&
							(pDataByte[j*this->nXSize+ i + n]		== this->pNoDataValue[0]) &&
							(pDataByte[j*this->nXSize + i + n +n]	== this->pNoDataValue[0])
								) ? 127 : 0;
				im->tpixels[j][i] = gdTrueColorAlpha(	pDataByte[j*this->nXSize+i],
														pDataByte[j*this->nXSize+i+n],
														pDataByte[j*this->nXSize+i+n+n],
														opaque);
			}
		}
	}
	else // ((this->nBands == 3) || (this->nBands == 1))
	{
		im->alphaBlendingFlag	= 0;
		im->saveAlphaFlag		= 0;
		for (int j=0;j<this->nYSize;j++)
		{
			for (int i=0;i<this->nXSize;i++)
				im->tpixels[j][i] = gdTrueColor(pDataByte[j*this->nXSize+i],pDataByte[j*this->nXSize+i+n],pDataByte[j*this->nXSize+i+n+n]);
		}
	}


	if (!(pDataDst = (BYTE*)gdImagePngPtr(im,&size)))
	{
		gdImageDestroy(im);
		return FALSE;
	}

	gdImageDestroy(im);
	return TRUE;
}
	
BOOL RasterBuffer::SaveToPngData (void* &pDataDst, int &size)
{

	if ((this->nXSize ==0)||(this->nYSize == 0)||(this->dataType!=GDT_Byte)) return FALSE;
	if (this->pTable==NULL) return SaveToPng24Data(pDataDst,size);
	
	gdImagePtr im	= gdImageCreate(this->nXSize,this->nYSize);
	im->colorsTotal = pTable->GetColorEntryCount();
	for (int i=0;(i<im->colorsTotal)&&(i<gdMaxColors);i++)
	{
		const GDALColorEntry *pCEntry = pTable->GetColorEntry(i);
		
		im->red[i]		= pCEntry->c1;
		im->green[i]	= pCEntry->c2;
		im->blue[i]		= pCEntry->c3;
		im->open[i]		= 0;
	}

	for (int j=0;j<this->nYSize;j++)
	{
		for (int i=0;i<this->nXSize;i++)
		{			
			im->pixels[j][i] = ((BYTE*)pData)[j*this->nXSize+i];
		}
	}

	if (!(pDataDst = (BYTE*)gdImagePngPtr(im,&size)))
	{
		gdImageDestroy(im);
		return FALSE;
	}

	gdImageDestroy(im);
	return TRUE;
}


BOOL	RasterBuffer::SaveToTiffData	(void* &pDataDst, int &size)
{
	GDALDataset* poDS = (GDALDataset*)GDALCreate(
		GDALGetDriverByName("GTiff"),
        "/vsimem/tiffinmem",
		this->nXSize,
		this->nYSize,
		this->nBands,
		this->dataType,
		NULL
		);
	poDS->RasterIO(GF_Write,0,0,this->nXSize,this->nYSize,pData,this->nXSize,this->nYSize,dataType,nBands,NULL,0,0,0);
	GDALFlushCache(poDS);
	GDALClose(poDS);
	vsi_l_offset length; 
	
	BYTE * pDataDstBuf = VSIGetMemFileBuffer("/vsimem/tiffinmem",&length, FALSE);
	size = length;
	//GDALClose(poDS);
	memcpy((pDataDst = new BYTE[size]),pDataDstBuf,size);
	VSIUnlink("/vsimem/tiffinmem");
	
	//SaveDataToFile(L"e:\\1.tif",pDataDst,size);
	//delete[]pDataDst;
	//ToDo: delete poDS
	return TRUE;
}


BOOL	RasterBuffer::isAnyNoDataPixel()
{
	if (this->pNoDataValue == NULL) return FALSE;
	if (this->pData == NULL)		return FALSE;

	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return isAnyNoDataPixel(t);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return isAnyNoDataPixel(t);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return isAnyNoDataPixel(t);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return isAnyNoDataPixel(t);
		}
		default:
			return FALSE;
	}
	return FALSE;
}


template <typename T>	
BOOL	RasterBuffer::isAnyNoDataPixel	(T type)
{
	int n = nXSize*nYSize;
	T *pDataT = (T*)pData;
	int i;
	for (int k = 0;k<n;k++)
	{
		for (i=0;i<nBands;i++)
			if (pDataT[k + i*n] != this->pNoDataValue[0]) break;
		if (i==nBands) return TRUE; 
	}
	return FALSE;
}



/*
BOOL  RasterBuffer::MergeUsingBlack (RasterBuffer oBackGround, RasterBuffer &oMerged)
{
	if ((this->getXSize()!=oBackGround.getXSize())||
		(this->getYSize()!=oBackGround.getYSize())||
		(this->nBands!=oBackGround.getBandsCount()))
	{
		return FALSE;
	}

	oMerged.createBuffer(this->nBands,this->nXSize,this->nYSize);
	BYTE *pMergedData = (BYTE*)oMerged.getData();

	memcpy(pMergedData,oBackGround.getData(),this->nXSize*this->nYSize*this->nBands);

	int k,s;
	int n = this->nXSize*this->nYSize;

	for (int i=0;i<this->nYSize;i++)
	{
		int l = i*this->nXSize;
		for (int j=0;j<this->nXSize;j++)
		{
			s=0;
			for (k=0;k<this->nBands;k++)
			{
				if (this->pData[l+j+s]!=0) break;
				s+=n;
			}
			if (k==this->nBands) 
			{
				s=0;
				for (k=0;k<this->nBands;k++)
				{
					pMergedData[l+j+s]!=this->pData[l+j+s];
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
	if ((this->nXSize==0)||(this->nYSize==0)) return FALSE;
	if (this->pTable!=NULL) convertFromIndexToRGB();

	gdImagePtr im	= gdImageCreateTrueColor(this->nXSize,this->nYSize);
	int n = this->nXSize*this->nYSize;
	if (this->nBands==1) n =0;
	int color = 0;
	for (int j=0;j<this->nYSize;j++)
	{
		for (int i=0;i<this->nXSize;i++)
		{
			color = 65536*this->pData[j*this->nXSize+i] + 256*this->pData[j*this->nXSize+i+n]+this->pData[j*this->nXSize+i+n+n];
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
	delete[]pData;
	pData = pDataOut;
	this->nXSize = nNewWidth;
	this->nYSize = nNewHeight;

	return TRUE;
}
*/



/*
BOOL RasterBuffer::makeZero(LONG nLeft, LONG nTop, LONG nWidth, LONG nHeight, LONG nNoDataValue)
{
	if (pData==NULL) return FALSE;
	if ((nLeft<0)||(nLeft+nWidth>this->nXSize)) return FALSE;
	if ((nTop<0)||(nTop+nHeight>this->nYSize)) return FALSE;
	
	LONG n = nXSize*nYSize;

	int n1,n2;
	n1=0;
	for (int j=nTop;j<nTop+nHeight;j++)
	{	
		n1=j*nXSize;
		for (int i=nLeft;i<nLeft+nWidth;i++)
		{
			n2=0;			
			for (int k=0;k<this->nBands;k++)
			{
				((BYTE*)pData)[n1+i+n2] = nNoDataValue;
				n2+=n;
			}
		}	
	}
	return TRUE;	
}
*/

BOOL RasterBuffer::initByNoDataValue(int noDataValue)
{
	setNoDataValue(noDataValue);
	return initByValue(noDataValue);
}

BOOL RasterBuffer::initByValue(int value)
{
	if (pData==NULL) return FALSE;

	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return initByValue(t,value);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return initByValue(t,value);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return initByValue(t,value);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return initByValue(t,value);
		}
		default:
			return NULL;
	}
	return TRUE;	
}

template <typename T>
BOOL RasterBuffer::initByValue(T type, int value)
{
	if (pData==NULL) return FALSE;

	T *pDataT = (T*)pData;
	if (!this->bAlphaBand)
	{
		unsigned __int64 n = nBands*nXSize*nYSize;
		for (unsigned __int64 i=0;i<n;i++)
			pDataT[i]=value;
	}
	else
	{
		unsigned __int64 n = (nBands-1)*nXSize*nYSize;
		for (unsigned __int64 i=0;i<n;i++)
			pDataT[i]=value;
		n = nBands*nXSize*nYSize;
		for (unsigned __int64 i=(nBands-1)*nXSize*nYSize;i<n;i++)
			pDataT[i]=100;

	}
	return TRUE;	
}



BOOL	RasterBuffer::stretchDataTo8Bit(double minVal, double maxVal)
{
	void *pDataNew = copyData(0,0,nXSize,nYSize,TRUE,minVal,maxVal);
	int bands = nBands;
	int width = nXSize;
	int height = nYSize;
	clearBuffer();
	return createBuffer(bands,width,height,pDataNew,GDT_Byte);
}


void*	RasterBuffer::copyData (int left, int top, int w, int h,  BOOL stretchTo8Bit, double min, double max)
{
	if (pData == NULL || this->nXSize == 0 || this->nYSize==0) return NULL;
	
	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return copyData(t,left,top,w,h);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return copyData(t,left,top,w,h);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return copyData(t,left,top,w,h);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return copyData(t,left,top,w,h);
		}
		default:
			return NULL;
	}
	return NULL;
}

///*
template <typename T>
void*	RasterBuffer::copyData (T type, int left, int top, int w, int h,  BOOL stretchTo8Bit, double minVal, double maxVal)
{
	if (nBands==0) return NULL;
	int					n = w*h;
	unsigned __int64	m = nXSize*nYSize;
	T				*pBlockT;
	BYTE			*pBlockB;

	if (stretchTo8Bit)	pBlockB		= new BYTE[nBands*n];
	else				pBlockT		= new T[nBands*n];

	
	T *pDataT = (T*)pData;
	double d;
	for (int k=0;k<nBands;k++)
	{
		for (int j=left;j<left+w;j++)
		{
			for (int i=top;i<top+h;i++)
			{
				if (!stretchTo8Bit) pBlockT[n*k+(i-top)*w+j-left] = pDataT[m*k+i*nXSize+j];
				else
				{
					d = max(min(pDataT[m*k+i*nXSize+j],maxVal),minVal);
					pBlockB[n*k+(i-top)*w+j-left] = (int)(0.5+255*((d-minVal)/(maxVal-minVal)));
				}
			}
		}
	}			

	if (stretchTo8Bit)	return pBlockB;
	else return pBlockT;
}
//*/

void*		RasterBuffer::getDataZoomedOut	()
{
	void *pDataZoomedOut;
	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t = 1;
			return getDataZoomedOut(t);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return getDataZoomedOut(t);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return getDataZoomedOut(t);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return getDataZoomedOut(t);
		}
		default:
			return NULL;
	}
}

template<typename T>
void* RasterBuffer::getDataZoomedOut	(T type)
{
	unsigned int a	= nXSize*nYSize/4;
	unsigned w = nXSize/2;
	unsigned h = nYSize/2;
	T	*pZoomedOutData	= NULL;
	if(! (pZoomedOutData = new T[nBands*a]) )
	{
		return NULL;
	}
	unsigned int m =0;
	T	*pDataT		= (T*)pData;

	for (int k=0;k<nBands;k++)
	{
		for (int i=0;i<h;i++)
		{
			for (int j=0;j<w;j++)
			{
				pZoomedOutData[m + i*w + j] = (	pDataT[(m<<2) + (i<<1)*(nXSize) + (j<<1)]+
												pDataT[(m<<2) + ((i<<1)+1)*(nXSize) + (j<<1)]+
												pDataT[(m<<2) + (i<<1)*(nXSize) + (j<<1) +1]+
												pDataT[(m<<2) + ((i<<1)+1)*(nXSize) + (j<<1) + 1])/4;
			}
		}
		m+=a;
	}
	return pZoomedOutData;
}

/*
BOOL	RasterBuffer::dataIO	(BOOL operationFlag, 
								int left, int top, int w, int h, 
								void *pData, 
								int bands = 0, BOOL stretchTo8Bit = FALSE, double min = 0, double max = 0)
{
	if (operationFlag==FALSE)
	{
		void *pData_ = copyData(left,top,w,h);
	}
	else
	{
		return setData(left,top,w,h,pData,bands);

	}
	return TRUE;
}
*/
BOOL	RasterBuffer::isAlphaBand()
{
	return this->bAlphaBand;
}

BOOL	RasterBuffer::createAlphaBandByColor(BYTE	*pRGB)
{
	if ((this-pData == NULL) || (this->dataType!=GDT_Byte) || (this->nBands>3)) return FALSE;

	int n = this->nXSize *this->nYSize;
	BYTE	*pData_new = new BYTE[(this->nBands+1) * n];
	memcpy(pData_new,pData,this->nBands * n);
	int d = (this->nBands == 3) ? 1 : 0;

	for (int i=0; i<this->nYSize; i++)
	{
		for (int j=0; j<this->nXSize; j++)
			pData_new[i*nXSize + j + n + d*(n+n)] = (	(pData_new[i*nXSize + j] == pRGB[0] ) &&
														(pData_new[i*nXSize + j + d*n] == pRGB[0 +d] ) &&
														(pData_new[i*nXSize + j + d*(n+n)] == pRGB[0+d+d] ) ) 
														? 100 : 0;
	}
	delete[]((BYTE*)pData);
	pData = pData_new;
	this->nBands++;
	this->bAlphaBand = TRUE;
	return TRUE;
}

//BOOL	RasterBuffer::createAlphaBandByValue(int	value);


BOOL	RasterBuffer::setData (int left, int top, int w, int h, void *pBlockData, int bands)
{
	if (pData == NULL || this->nXSize == 0 || this->nYSize==0) return NULL;
	bands = (bands==0) ? nBands : bands;

	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t = 1;
			return setData(t,left,top,w,h,pBlockData,bands);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return setData(t,left,top,w,h,pBlockData,bands);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return setData(t,left,top,w,h,pBlockData,bands);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return setData(t,left,top,w,h,pBlockData,bands);
		}
		default:
			return FALSE;
	}
	return FALSE;
}

///*
template <typename T>
BOOL	RasterBuffer::setData (T type, int left, int top, int w, int h, void *pBlockData, int bands)
{
	bands = (bands==0) ? nBands : bands;

	int n = w*h;
	int m = nXSize*nYSize;
	T *pDataT = (T*)pData;
	T *pBlockDataT = (T*)pBlockData;


	for (int k=0;k<bands;k++)
	{
		for (int j=left;j<left+w;j++)
			for (int i=top;i<top+h;i++)
				pDataT[m*k+i*nXSize+j] = pBlockDataT[n*k+(i-top)*w + j-left];
	}		

	return TRUE;
}

//*/


void* RasterBuffer::getDataRef()
{
	return pData;
}


int	RasterBuffer::getBandsCount()
{
	return nBands;	
}


int RasterBuffer::getXSize()
{
	return nXSize;	
}


int RasterBuffer::getYSize()
{
	return nYSize;	
}


GDALDataType RasterBuffer::getDataType()
{
	return dataType;
}


BOOL	RasterBuffer::setColorMeta (GDALColorTable *pTable)
{
	this->pTable = pTable;
	
	return TRUE;
}


GDALColorTable*	RasterBuffer::getColorMeta ()
{
	return pTable;
}