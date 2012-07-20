#include "StdAfx.h"
#include "RasterBuffer.h"



RasterBuffer::RasterBuffer(void)
{
	pData = NULL;	
	pTable = NULL;
	for (int i=0;i<3;i++)
		backgroundColor[i] = 0;
}


RasterBuffer::~RasterBuffer(void)
{
	clearBuffer();
}


BOOL RasterBuffer::setBackgroundColor(int rgb[3])
{
	for (int i=0;i<3;i++)
		backgroundColor[i] = rgb[i];
	return TRUE;
}


void RasterBuffer::clearBuffer()
{
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



BOOL RasterBuffer::createBuffer	(int nBands_set,
									int nBufferXSize_set,
									int nBufferYSize_set,
									void *pData_set,
									GDALDataType dataType)
{
	clearBuffer();

	this->nBands		= nBands_set;
	this->nBufferXSize	= nBufferXSize_set;
	this->nBufferYSize	= nBufferYSize_set;
	this->dataType		= dataType;

	switch (dataType)
	{
		case GDT_Byte:
				pData = new BYTE[nBands*nBufferXSize*nBufferYSize];
				this->dataSize = 1;
				break;
		case GDT_UInt16:
				pData = new unsigned __int16[nBands*nBufferXSize*nBufferYSize];
				this->dataSize = 2;
				break;
		case GDT_Int16:
				pData = new __int16[nBands*nBufferXSize*nBufferYSize];
				this->dataSize = 3;
				break;
		case GDT_Float32:
				pData = new float[nBands*nBufferXSize*nBufferYSize];
				this->dataSize = 4;
				break;			
		default:
			return FALSE;
	}

	if (pData_set !=NULL)
	{
		memcpy(pData,pData_set,dataSize*nBands*nBufferXSize*nBufferYSize);
	}
	 return TRUE;
}


BOOL RasterBuffer::createBuffer		(RasterBuffer *pBuffer)
{
	if (!createBuffer(pBuffer->getBandsCount(),pBuffer->getBufferXSize(),pBuffer->getBufferYSize(),pBuffer->getBufferData(),pBuffer->getBufferDataType())) return FALSE;

	if (pBuffer->pTable) this->setColorMeta(pBuffer->pTable);
	for (int i=0;i<3;i++)
		this->backgroundColor[i] = pBuffer->backgroundColor[i];
	return TRUE;	
}

BOOL	RasterBuffer::createBufferFromTiffData	(void *pDataSrc, int size)
{

	VSIFileFromMemBuffer("/vsimem/tiffinmem",(BYTE*)pDataSrc,size,0);
	GDALDataset *poDS = (GDALDataset*) GDALOpen("/vsimem/tiffinmem",GA_ReadOnly);

	createBuffer(poDS->GetRasterCount(),poDS->GetRasterXSize(),poDS->GetRasterYSize(),NULL,poDS->GetRasterBand(1)->GetRasterDataType());
	poDS->RasterIO(GF_Read,0,0,nBufferXSize,nBufferXSize,pData,nBufferXSize,nBufferYSize,dataType,nBands,NULL,0,0,0); 
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
			k = j*this->nBufferXSize+i;		
			pDataB[k] = color>>16;
			pDataB[n+ k] = (color%65536)>>8;
			pDataB[n+ n+ k] = color%256;
		}
	}

	gdImageDestroy(im);
	return TRUE;

}


BOOL	RasterBuffer::createBufferFromPngData (void *pDataSrc, int size)
{
	gdImagePtr im;
	if (!  (im =	gdImageCreateFromPngPtr(size, pDataSrc))) return FALSE;
	
	createBuffer(3,im->sx,im->sy,NULL,GDT_Byte);
	BYTE	*pDataB	= (BYTE*)pData;

	int n = im->sx * im->sy;
	int color, k;
	for (int j=0;j<im->sy;j++)
	{
		for (int i=0;i<im->sx;i++)
		{
			color = im->tpixels[j][i];
			k = j*this->nBufferXSize+i;		
			pDataB[k] = color>>16;
			pDataB[n+ k] = (color%65536)>>8;
			pDataB[n+ n+ k] = color%256;
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
	if (this->nBufferXSize==0 || this->nBufferYSize == 0 || this->dataType != GDT_Byte) return FALSE;
	if (this->nBands!=1)		return FALSE;
	if (this->pData==NULL)	return FALSE;
	
	BYTE *pDataNew = new BYTE[3*this->nBufferXSize*this->nBufferYSize];
	int n = this->nBufferXSize*this->nBufferYSize;
	for (int j=0;j<this->nBufferYSize;j++)
	{
		for (int i=0;i<this->nBufferXSize;i++)
		{
			pDataNew[j*this->nBufferXSize+i + n + n] =
				(pDataNew[j*this->nBufferXSize+i +n] = (pDataNew[j*this->nBufferXSize+i] = pDataNew[j*this->nBufferXSize+i]));
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
		int n = this->nBufferXSize*this->nBufferYSize;
		BYTE *pDataNew = new BYTE[3*n];
		int m;
		const GDALColorEntry *pCEntry;
		for (int i=0;i<this->nBufferXSize;i++)
		{
			for (int j=0;j<this->nBufferYSize;j++)
			{
				m = j*this->nBufferXSize+i;
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
	
	if ((this->nBufferXSize ==0)||(this->nBufferYSize == 0)||(this->dataType!=GDT_Byte)) return FALSE;
	gdImagePtr im	= gdImageCreateTrueColor(this->nBufferXSize,this->nBufferYSize);
	
	int n = this->nBufferXSize*this->nBufferYSize;
	if (this->nBands == 1) n = 0;
	int color = 0;
	for (int j=0;j<this->nBufferYSize;j++)
	{
		for (int i=0;i<this->nBufferXSize;i++)
		{
			color = 65536*((BYTE*)pData)[j*this->nBufferXSize+i] + 256*((BYTE*)pData)[j*this->nBufferXSize+i+n]+((BYTE*)pData)[j*this->nBufferXSize+i+n+n];
			im->tpixels[j][i] = color;
		}
	}
	//int size =0;
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
	if ((this->nBufferXSize ==0)||(this->nBufferYSize == 0)||(this->dataType!=GDT_Byte)) return FALSE;
	
	gdImagePtr im		= gdImageCreateTrueColor(this->nBufferXSize,this->nBufferYSize);
	if (this->nBands==4)
	{
		//im->saveAlphaFlag=1;
		im->alphaBlendingFlag = 1;
	}
	
	int n = this->nBufferXSize*this->nBufferYSize;
	if (this->nBands == 1) n = 0;
	int color = 0;
	for (int j=0;j<this->nBufferYSize;j++)
	{
		for (int i=0;i<this->nBufferXSize;i++)
		{
			color = 65536*((BYTE*)pData)[j*this->nBufferXSize+i] + 256*((BYTE*)pData)[j*this->nBufferXSize+i+n]+((BYTE*)pData)[j*this->nBufferXSize+i+n+n];
			if (this->nBands == 4)
			{
				color = gdTrueColorAlpha(((BYTE*)pData)[j*this->nBufferXSize+i],((BYTE*)pData)[j*this->nBufferXSize+i+n],((BYTE*)pData)[j*this->nBufferXSize+i+n+n],((BYTE*)pData)[j*this->nBufferXSize+i+n+n+n]);
				//if (this->pData[j*this->nBufferXSize+i+n+n+n]==0) color	= gdTrueColorAlpha(255,255,255,127);
				//else color = gdTrueColor(this->pData[j*this->nBufferXSize+i],this->pData[j*this->nBufferXSize+i+n],this->pData[j*this->nBufferXSize+i+n+n]);
			}
			else color = gdTrueColor(((BYTE*)pData)[j*this->nBufferXSize+i],((BYTE*)pData)[j*this->nBufferXSize+i+n],((BYTE*)pData)[j*this->nBufferXSize+i+n+n]);
			im->tpixels[j][i] = color;
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

	if ((this->nBufferXSize ==0)||(this->nBufferYSize == 0)||(this->dataType!=GDT_Byte)) return FALSE;
	if (this->pTable==NULL) return SaveToPng24Data(pDataDst,size);
	
	gdImagePtr im	= gdImageCreate(this->nBufferXSize,this->nBufferYSize);
	im->colorsTotal = pTable->GetColorEntryCount();
	for (int i=0;(i<im->colorsTotal)&&(i<gdMaxColors);i++)
	{
		const GDALColorEntry *pCEntry = pTable->GetColorEntry(i);
		
		im->red[i]		= pCEntry->c1;
		im->green[i]	= pCEntry->c2;
		im->blue[i]		= pCEntry->c3;
		im->open[i]		= 0;
	}

	for (int j=0;j<this->nBufferYSize;j++)
	{
		for (int i=0;i<this->nBufferXSize;i++)
		{			
			im->pixels[j][i] = ((BYTE*)pData)[j*this->nBufferXSize+i];
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
		this->nBufferXSize,
		this->nBufferYSize,
		this->nBands,
		this->dataType,
		NULL
		);
	poDS->RasterIO(GF_Write,0,0,this->nBufferXSize,this->nBufferYSize,pData,this->nBufferXSize,this->nBufferYSize,dataType,nBands,NULL,0,0,0);
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


/*
BOOL  RasterBuffer::MergeUsingBlack (RasterBuffer oBackGround, RasterBuffer &oMerged)
{
	if ((this->getBufferXSize()!=oBackGround.getBufferXSize())||
		(this->getBufferYSize()!=oBackGround.getBufferYSize())||
		(this->nBands!=oBackGround.getBandsCount()))
	{
		return FALSE;
	}

	oMerged.createBuffer(this->nBands,this->nBufferXSize,this->nBufferYSize);
	BYTE *pMergedData = (BYTE*)oMerged.getBufferData();

	memcpy(pMergedData,oBackGround.getBufferData(),this->nBufferXSize*this->nBufferYSize*this->nBands);

	int k,s;
	int n = this->nBufferXSize*this->nBufferYSize;

	for (int i=0;i<this->nBufferYSize;i++)
	{
		int l = i*this->nBufferXSize;
		for (int j=0;j<this->nBufferXSize;j++)
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
	if ((this->nBufferXSize==0)||(this->nBufferYSize==0)) return FALSE;
	if (this->pTable!=NULL) convertFromIndexToRGB();

	gdImagePtr im	= gdImageCreateTrueColor(this->nBufferXSize,this->nBufferYSize);
	int n = this->nBufferXSize*this->nBufferYSize;
	if (this->nBands==1) n =0;
	int color = 0;
	for (int j=0;j<this->nBufferYSize;j++)
	{
		for (int i=0;i<this->nBufferXSize;i++)
		{
			color = 65536*this->pData[j*this->nBufferXSize+i] + 256*this->pData[j*this->nBufferXSize+i+n]+this->pData[j*this->nBufferXSize+i+n+n];
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
	this->nBufferXSize = nNewWidth;
	this->nBufferYSize = nNewHeight;

	return TRUE;
}
*/


BOOL	RasterBuffer::createAlphaBand(int *rgb)
{
	if ( nBands!=3 || dataType!=GDT_Byte) return FALSE;
	/*
	if (strColor.length() != 6) return FALSE;
	int rgb[3];
	for (int i=0;i<3;i++)
	{
		rgb[i] = 0;
		for (int j=0;j<2;j++)
		{
			switch (strColor[i*2+j])
			{
				case '0':
					break;
				case '1':
					rgb[i]+= 16*(1-j) +j;
					break;
				case '2':
					rgb[i]+=2*(16*(1-j) +j);
					break;
				case '3':
					rgb[i]+=3*(16*(1-j) +j);
					break;
				case '4':
					rgb[i]+=4*(16*(1-j) +j);
					break;
				case '5':
					rgb[i]+=5*(16*(1-j) +j);
					break;
				case '6':
					rgb[i]+=6*(16*(1-j) +j);
					break;
				case '7':
					rgb[i]+=7*(16*(1-j) +j);
					break;
				case '8':
					rgb[i]+=8*(16*(1-j) +j);
					break;
				case '9':
					rgb[i]+=9*(16*(1-j) +j);
					break;
				case 'a':
					rgb[i]+=10*(16*(1-j) +j);
					break;
				case 'b':
					rgb[i]+=11*(16*(1-j) +j);
					break;
				case 'c':
					rgb[i]+=12*(16*(1-j) +j);
					break;
				case 'd':
					rgb[i]+=13*(16*(1-j) +j);
					break;
				case 'e':
					rgb[i]+=14*(16*(1-j) +j);
					break;
				case 'f':
					rgb[i]+=15*(16*(1-j) +j);
					break;
				default:
					return FALSE;
			}
		}
	}
	*/
	int n = this->nBufferXSize*this->nBufferYSize;
	BYTE *pData_new = new BYTE[4*n];
	for (int j=0;j<this->nBufferYSize;j++)
	{
		for (int i=0;i<this->nBufferXSize;i++)
		{
			if ((((BYTE*)pData)[j*nBufferXSize+i]==rgb[0])&&(((BYTE*)pData)[j*nBufferXSize+i+n]==rgb[1])&&(((BYTE*)pData)[j*nBufferXSize+i+n+n]==rgb[2]))
				pData_new[j*nBufferXSize+i+n+n+n] = 0;	
			else
			{
				pData_new[j*nBufferXSize+i]			= ((BYTE*)pData)[j*nBufferXSize+i];
				pData_new[j*nBufferXSize+i+n]		= ((BYTE*)pData)[j*nBufferXSize+i+n];
				pData_new[j*nBufferXSize+i+n+n]		= ((BYTE*)pData)[j*nBufferXSize+i+n+n];
				pData_new[j*nBufferXSize+i+n+n+n]	= 1;
			}
		}
	}


	delete[]pData;
	
	this->nBands = 4;
	pData = pData_new;
	return TRUE;
}

/*
BOOL RasterBuffer::makeZero(LONG nLeft, LONG nTop, LONG nWidth, LONG nHeight, LONG nNoDataValue)
{
	if (pData==NULL) return FALSE;
	if ((nLeft<0)||(nLeft+nWidth>this->nBufferXSize)) return FALSE;
	if ((nTop<0)||(nTop+nHeight>this->nBufferYSize)) return FALSE;
	
	LONG n = nBufferXSize*nBufferYSize;

	int n1,n2;
	n1=0;
	for (int j=nTop;j<nTop+nHeight;j++)
	{	
		n1=j*nBufferXSize;
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

BOOL RasterBuffer::initByNoDataValue(int nNoDataValue)
{
	if (pData==NULL) return FALSE;

	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return initByNoDataValue(t,nNoDataValue);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return initByNoDataValue(t,nNoDataValue);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return initByNoDataValue(t,nNoDataValue);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return initByNoDataValue(t,nNoDataValue);
		}
		default:
			return NULL;
	}
	return TRUE;	
}

template <typename T>
BOOL RasterBuffer::initByNoDataValue(T type, int nNoDataValue)
{
	if (pData==NULL) return FALSE;

	unsigned __int64 n = nBands*nBufferXSize*nBufferYSize;
	T *pDataT = (T*)pData;
	for (unsigned __int64 i=0;i<n;i++)
		pDataT[i]=nNoDataValue;
	return TRUE;	
}

BOOL RasterBuffer::initByBackgroundColor()
{
	if (pData==NULL) return FALSE;
	if (this->nBands != 3 || this->dataType != GDT_Byte) return FALSE;
	LONG n = nBufferXSize*nBufferYSize;
	for (int i = 0;i<3;i++)
		for (int j=n*i;j<n*(i+1);j++)
			((BYTE*)pData)[j] = backgroundColor[i];

	return FALSE;
}

BOOL	RasterBuffer::stretchDataTo8Bit(double minVal, double maxVal)
{
	void *pDataNew = getBlockFromBuffer(0,0,nBufferXSize,nBufferYSize,TRUE,minVal,maxVal);
	int bands = nBands;
	int width = nBufferXSize;
	int height = nBufferYSize;
	clearBuffer();
	return createBuffer(bands,width,height,pDataNew,GDT_Byte);
}


void*	RasterBuffer::getBlockFromBuffer (int left, int top, int w, int h,  BOOL stretchTo8Bit, double min, double max)
{
	if (pData == NULL || this->nBufferXSize == 0 || this->nBufferYSize==0) return NULL;
	
	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t  = 1;
			return getBlockFromBuffer(t,left,top,w,h);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return getBlockFromBuffer(t,left,top,w,h);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return getBlockFromBuffer(t,left,top,w,h);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return getBlockFromBuffer(t,left,top,w,h);
		}
		default:
			return NULL;
	}
	return NULL;
}

///*
template <typename T>
void*	RasterBuffer::getBlockFromBuffer (T type, int left, int top, int w, int h,  BOOL stretchTo8Bit, double minVal, double maxVal)
{
	if (nBands==0) return NULL;
	int					n = w*h;
	unsigned __int64	m = nBufferXSize*nBufferYSize;
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
				if (!stretchTo8Bit) pBlockT[n*k+(i-top)*w+j-left] = pDataT[m*k+i*nBufferXSize+j];
				else
				{
					d = max(min(pDataT[m*k+i*nBufferXSize+j],maxVal),minVal);
					pBlockB[n*k+(i-top)*w+j-left] = (int)(0.5+255*((d-minVal)/(maxVal-minVal)));
				}
			}
		}
	}			

	if (stretchTo8Bit)	return pBlockB;
	else return pBlockT;
}
//*/

BOOL		RasterBuffer::createSimpleZoomOut	(RasterBuffer &oBufferDst)
{
	void *pDataZoomedOut;
	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t = 1;
			if (!createSimpleZoomOut(t,pDataZoomedOut)) return FALSE;
			if(!oBufferDst.createBuffer(nBands,nBufferXSize/2,nBufferYSize/2,pDataZoomedOut,dataType)) return FALSE;
			delete[]((BYTE*)pDataZoomedOut);
			break;
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			if (!createSimpleZoomOut(t,pDataZoomedOut)) return FALSE;
			if(!oBufferDst.createBuffer(nBands,nBufferXSize/2,nBufferYSize/2,pDataZoomedOut,dataType)) return FALSE;
			delete[]((BYTE*)pDataZoomedOut);
			break;
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			if (!createSimpleZoomOut(t,pDataZoomedOut)) return FALSE;
			if(!oBufferDst.createBuffer(nBands,nBufferXSize/2,nBufferYSize/2,pDataZoomedOut,dataType)) return FALSE;
			delete[]((BYTE*)pDataZoomedOut);
			break;
		}
		case GDT_Float32:
		{
			float t = 1.1;
			if (!createSimpleZoomOut(t,pDataZoomedOut)) return FALSE;
			if(!oBufferDst.createBuffer(nBands,nBufferXSize/2,nBufferYSize/2,pDataZoomedOut,dataType)) return FALSE;
			delete[]((BYTE*)pDataZoomedOut);
			break;
		}
		default:
			return FALSE;
	}
	return TRUE;
}

template<typename T>
BOOL RasterBuffer::createSimpleZoomOut	(T type, void* &pDataDst)
{
	unsigned int a	= nBufferXSize*nBufferYSize/4;
	unsigned w = nBufferXSize/2;
	unsigned h = nBufferYSize/2;
	T	*pDataDstT	= NULL;
	if(! (pDataDstT = new T[nBands*a]) ) return FALSE;
	
	unsigned int m =0;
	T	*pDataT		= (T*)pData;

	for (int k=0;k<nBands;k++)
	{
		for (int i=0;i<h;i++)
		{
			for (int j=0;j<w;j++)
			{
				pDataDstT[m + i*w + j] = (	pDataT[(m<<2) + (i<<1)*(nBufferXSize) + (j<<1)]+
												pDataT[(m<<2) + ((i<<1)+1)*(nBufferXSize) + (j<<1)]+
												pDataT[(m<<2) + (i<<1)*(nBufferXSize) + (j<<1) +1]+
												pDataT[(m<<2) + ((i<<1)+1)*(nBufferXSize) + (j<<1) + 1])/4;
			}
		}
		m+=a;
	}
	//delete[]poData;
	pDataDst = pDataDstT;
	return TRUE;
}

/*
BOOL	RasterBuffer::dataIO	(BOOL operationFlag, 
								int left, int top, int w, int h, 
								void *pData, 
								int bands = 0, BOOL stretchTo8Bit = FALSE, double min = 0, double max = 0)
{
	if (operationFlag==FALSE)
	{
		void *pData_ = getBlockFromBuffer(left,top,w,h);
	}
	else
	{
		return writeBlockToBuffer(left,top,w,h,pData,bands);

	}
	return TRUE;
}
*/

BOOL	RasterBuffer::writeBlockToBuffer (int left, int top, int w, int h, void *pBlockData, int bands)
{
	if (pData == NULL || this->nBufferXSize == 0 || this->nBufferYSize==0) return NULL;
	bands = (bands==0) ? nBands : bands;

	switch (dataType)
	{
		case GDT_Byte:
		{
			BYTE t = 1;
			return writeBlockToBuffer(t,left,top,w,h,pBlockData,bands);
		}
		case GDT_UInt16:
		{
			unsigned __int16 t = 257;
			return writeBlockToBuffer(t,left,top,w,h,pBlockData,bands);
		}
		case GDT_Int16:
		{
			__int16 t = -257;
			return writeBlockToBuffer(t,left,top,w,h,pBlockData,bands);
		}
		case GDT_Float32:
		{
			float t = 1.1;
			return writeBlockToBuffer(t,left,top,w,h,pBlockData,bands);
		}
		default:
			return FALSE;
	}
	return FALSE;
}

///*
template <typename T>
BOOL	RasterBuffer::writeBlockToBuffer (T type, int left, int top, int w, int h, void *pBlockData, int bands)
{
	bands = (bands==0) ? nBands : bands;

	int n = w*h;
	int m = nBufferXSize*nBufferYSize;
	T *pDataT = (T*)pData;
	T *pBlockDataT = (T*)pBlockData;


	for (int k=0;k<bands;k++)
	{
		for (int j=left;j<left+w;j++)
			for (int i=top;i<top+h;i++)
				pDataT[m*k+i*nBufferXSize+j] = pBlockDataT[n*k+(i-top)*w + j-left];
	}		

	return TRUE;
}

//*/


void* RasterBuffer::getBufferData()
{
	return pData;
}


int	RasterBuffer::getBandsCount()
{
	return nBands;	
}


int RasterBuffer::getBufferXSize()
{
	return nBufferXSize;	
}


int RasterBuffer::getBufferYSize()
{
	return nBufferYSize;	
}


GDALDataType RasterBuffer::getBufferDataType()
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