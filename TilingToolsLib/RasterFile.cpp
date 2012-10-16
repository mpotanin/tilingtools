#include "StdAfx.h"
#include "RasterFile.h"
#include "GeometryFuncs.h"
#include "str.h"



//wstring str_buf;
//_TCHAR buf[256];

int _stdcall PrintNoProgress ( double dfComplete, const char *pszMessage, 
                      void * pProgressArg )
{
	return 1;
}

 
void RasterFile::setGeoReference(double dResolution, double dULx, double dULy)
{
	this->m_dResolution = dResolution;
	this->m_dULx = dULx;
	this->m_dULy = dULy;
	this->m_isGeoReferenced = TRUE;
}


/*
void RasterFile::adjustGeoReference (double dResolution)
{

	this->m_dResolution = dResolution;
	if (fabs(this->m_dULx - floor((this->m_dULx/(this->m_dResolution*TILE_SIZE))+0.5)*this->m_dResolution*TILE_SIZE) < fabs(this->m_dResolution/2)+E &&
		fabs(this->m_dULy - floor((this->m_dULy/(this->m_dResolution*TILE_SIZE))+0.5)*this->m_dResolution*TILE_SIZE) < fabs(this->m_dResolution/2)+E)
	{
		this->m_dULx = floor((this->m_dULx/(TILE_SIZE*this->m_dResolution))+0.5)*TILE_SIZE*this->m_dResolution;
		this->m_dULy = floor((this->m_dULy/(TILE_SIZE*this->m_dResolution))+0.5)*TILE_SIZE*this->m_dResolution;
	}
	else
	{
		this->m_dULx = floor((this->m_dULx/this->m_dResolution)+0.5)*this->m_dResolution;
		this->m_dULy = floor((this->m_dULy/this->m_dResolution)+0.5)*this->m_dResolution;
	}


}
*/


void RasterFile::delete_all()
{
	delete(m_poDataset);
	m_poDataset = NULL;
	m_strRasterFile	= L"";
	m_nBands			= 0;
	//m_strImageFormat	= L"";
	m_isGeoReferenced = FALSE;
}


BOOL RasterFile::readGeoReferenceFromWLD (double dShiftX, double dShiftY)
{
	if (this->m_strRasterFile==L"") return FALSE;
	//CPathT<wstring> oPath(this->m_strRasterFile);

	wstring strWLDFile = this->m_strRasterFile;
	if (strWLDFile.length()>1)
	{
		strWLDFile[strWLDFile.length()-2]=strWLDFile[strWLDFile.length()-1];
		strWLDFile[strWLDFile.length()-1]=L'w';
	}

	FILE *fp = NULL;

	if (!(fp = _wfopen (strWLDFile.c_str(), L"r"))) return FALSE;

	
	if (1!=fwscanf(fp,L"%lf",&this->m_dResolution)) return FALSE;
	
	double t;
	if (1!=fwscanf(fp,L"%lf",&t)) return FALSE;
	if (1!=fwscanf(fp,L"%lf",&t)) return FALSE;
	if (1!=fwscanf(fp,L"%lf",&t)) return FALSE;

	if (1!=fwscanf(fp,L"%lf",&this->m_dULx)) return FALSE;
	if (1!=fwscanf(fp,L"%lf",&this->m_dULy)) return FALSE;

	this->m_dULx +=dShiftX; 
	this->m_dULy +=dShiftY;
	fclose(fp);
	

	return TRUE;
}

BOOL RasterFile::close()
{
	delete_all();
	return FALSE;
}

BOOL RasterFile::init(wstring strRasterFile, BOOL isGeoReferenced, double dShiftX, double dShiftY)
{
	delete_all();
	GDALDriver *poDriver = NULL;
	
	string strRasterFileUTF8;
	wstrToUtf8(strRasterFileUTF8,strRasterFile);
	m_poDataset = (GDALDataset *) GDALOpen(strRasterFileUTF8.c_str(), GA_ReadOnly );
	

	if (m_poDataset==NULL)
	{
		wcout<<L"Error: RasterFile::init: can't open raster image"<<endl;
		return FALSE;
	}
	
	this->m_strRasterFile = strRasterFile;

	if (!(poDriver = m_poDataset->GetDriver()))
	{
		wcout<<L"Error: RasterFile::init: can't get GDALDriver from image"<<endl;
		return FALSE;
	}

	this->m_nBands	= m_poDataset->GetRasterCount();
	if (this->m_nBands==0) 
	{
		wcout<<L"Error: RasterFile::init: can't read raster bands from file: "<<strRasterFile<<endl;
		return FALSE;
	}

	this->m_nHeight = m_poDataset->GetRasterYSize();
	this->m_nWidth	= m_poDataset->GetRasterXSize();
	
	this->m_nNoDataValue = m_poDataset->GetRasterBand(1)->GetNoDataValue(&this->m_bNoDataValueDefined);


	/*
	this->m_strImageFormat = poDriver->GetDescription();
	if (m_strImageFormat == "GIF")
	{
		m_strImageFormat = RasterFile::FORMAT_GIF;
	}
	else if (m_strImageFormat == "PNG")
	{
		m_strImageFormat = RasterFile::FORMAT_PNG;
	}
	else if (m_strImageFormat == "JPEG")
	{
		m_strImageFormat = RasterFile::FORMAT_JPEG;
	}
	else if (m_strImageFormat == "BMP")
	{
		m_strImageFormat = RasterFile::FORMAT_BMP;
	}
	else if (m_strImageFormat == "GTiff")
	{
		m_strImageFormat = RasterFile::FORMAT_GTIFF;
	}
	else if (m_strImageFormat == "HFA")
	{
		m_strImageFormat = RasterFile::FORMAT_HFA;
	}
	else
	{
		wcout<<L"Error: RasterFile::init: bad image format"<<endl;
		return FALSE;
	}
	*/

	if (isGeoReferenced)
	{
		double GeoTransform[6];

		if (readGeoReferenceFromWLD(dShiftX,dShiftY)) this->m_isGeoReferenced = TRUE;
		else if (m_poDataset->GetGeoTransform(GeoTransform)== CE_None)			
		{
			this->m_dULx = GeoTransform[0] + dShiftX;
			this->m_dULy = GeoTransform[3] + dShiftY;
			this->m_dResolution = GeoTransform[1];
			this->m_isGeoReferenced = TRUE;
		}
		else
		{
			wcout<<L"Error: RasterFile::init: can't read georeference"<<endl;
			return FALSE;
		}
	}
	
	return TRUE;
}


BOOL RasterFile::getToBuffer	(RasterBuffer &oImageBuffer,
										double min_x,
										double max_y,
										double max_x,
										double min_y
										)
{
	if (!this->m_isGeoReferenced) return FALSE;
	int nLeft, nTop, nWidth, nHeight;
	
	this->calcFromProjectionToPixels(min_x,max_y,&nLeft,&nTop);
	this->calcFromProjectionToPixels(max_x,min_y,&nWidth,&nHeight);
	return this->getToBuffer(oImageBuffer,nLeft,nTop,nWidth-nLeft,nHeight-nTop);
}


BOOL RasterFile::getToBuffer(RasterBuffer &oImageBuffer,
									int nLeft,
									int nTop,
									int nWidth,
									int nHeight
									//int Zero = 0
									)

{
	if (!m_poDataset)
	{
		//str_buf = "createVirtual: NULL m_poDataset";
		//strError+="createVirtual: NULL m_poDataset"; 
		return FALSE;
	}
	
	int nXOff_adjust = (nLeft>=0)	? nLeft : 0;
	int nYOff_adjust = (nTop>=0)	? nTop : 0;

	int nBufferXSize_adjust = nWidth;
	int nBufferYSize_adjust = nHeight;

	int nWidth_adjust = nWidth-(nXOff_adjust-nLeft);
	int nHeight_adjust = nHeight-(nYOff_adjust-nTop);

    if (m_nWidth<nLeft+nWidth)  nWidth_adjust  = m_nWidth-nXOff_adjust;
	if (nTop+nHeight>m_nHeight) nHeight_adjust = m_nHeight-nYOff_adjust;

	if (!oImageBuffer.createBuffer(m_nBands,nWidth,nHeight,NULL,m_poDataset->GetRasterBand(1)->GetRasterDataType()))
	{
		//str_buf = "getToBuffer: Can't create buffer";
        //strError+="getToBuffer: Can't create buffer";
		throw ERROR;
	}

	if ((nWidth_adjust<nWidth)||(nHeight_adjust<nHeight)) 
	{	
		if (m_bNoDataValueDefined) oImageBuffer.initByNoDataValue(this->m_nNoDataValue);
		else oImageBuffer.initByValue(0);
	}
	if ((nWidth_adjust>0)&&(nHeight_adjust>0))
	{
		void *pData = oImageBuffer.getDataRef();
		
		if (nWidth_adjust<nWidth)
		{
			nBufferXSize_adjust = (int)ceil((nWidth*(((double)nWidth_adjust)/((double)nWidth)))-0.5);
		}
		if (nHeight_adjust<nHeight)
		{
			nBufferYSize_adjust = (int)ceil((nHeight*(((double)nHeight_adjust)/((double)nHeight)))-0.5);
		}

		void *pData_offset = pData;
		int nOffsetX_adjust;
		
		nOffsetX_adjust = (nLeft+nWidth<=m_nWidth) ? nWidth-nBufferXSize_adjust : (int)ceil( nWidth*(((double)(nXOff_adjust-nLeft))/((double)nWidth))-0.5);
		if (nWidth-nBufferXSize_adjust-nOffsetX_adjust<0) nOffsetX_adjust--;

		int nOffsetY_adjust;
		nOffsetY_adjust = (nTop+nHeight<=m_nHeight) ? nHeight-nBufferYSize_adjust : (int)ceil( nHeight*(((double)(nYOff_adjust-nTop))/((double)nHeight))-0.5);
		if (nHeight-nBufferYSize_adjust-nOffsetY_adjust<0) nOffsetY_adjust--;

		int data_size = 1;
		switch (m_poDataset->GetRasterBand(1)->GetRasterDataType())
		{
			case GDT_Byte:
				pData = &((BYTE*)pData_offset)[nWidth*nOffsetY_adjust+nOffsetX_adjust];
				data_size = 1;
				break;
			case GDT_UInt16:
				pData = &((unsigned __int16*)pData_offset)[nWidth*nOffsetY_adjust+nOffsetX_adjust];
				data_size = 2;
				break;
			case GDT_Int16:
				pData = &((__int16*)pData_offset)[nWidth*nOffsetY_adjust+nOffsetX_adjust];
				data_size = 2;
				break;
			case GDT_Float32:
				pData = &((float*)pData_offset)[nWidth*nOffsetY_adjust+nOffsetX_adjust];
				data_size = 4;
				break;
			default:
				pData = &((BYTE*)pData_offset)[nWidth*nOffsetY_adjust+nOffsetX_adjust];
				data_size = 1;
				break;
		}

		if (CE_Failure == m_poDataset->RasterIO(GF_Read,nXOff_adjust,nYOff_adjust,nWidth_adjust,nHeight_adjust,pData,nBufferXSize_adjust,nBufferYSize_adjust,m_poDataset->GetRasterBand(1)->GetRasterDataType(),m_nBands,NULL,0,nWidth*data_size,data_size*nHeight*nWidth))
		{
			//str_buf = "RasterIO: Can't read to buffer from the image";
			//strError+="RasterIO: Can't read to buffer from the image"; 
			throw ERROR;
		}

		if (m_poDataset->GetRasterBand(1)->GetColorInterpretation()==GCI_PaletteIndex)
		{
			oImageBuffer.setColorMeta(m_poDataset->GetRasterBand(1)->GetColorTable()->Clone());
		}
	}
	
	return TRUE;
}

BOOL	RasterFile::getMinMaxPixelValues (double &minVal, double &maxVal)
{
	//void 

	return TRUE;
}

BOOL RasterFile::getNoDataValue (int *pNoDataValue)
{
	if (! this->m_bNoDataValueDefined) return FALSE;
	(*pNoDataValue) = this->m_nNoDataValue;
	return TRUE;
}


BOOL RasterFile::writeWLDFile (wstring strRasterFile, double dULx, double dULy, double dRes)
{
	if (strRasterFile.length()>1)
	{
		strRasterFile[strRasterFile.length()-2]=strRasterFile[strRasterFile.length()-1];
		strRasterFile[strRasterFile.length()-1]='w';
	}
	

	FILE *fp;

	if (!(fp=_wfopen(strRasterFile.c_str(),L"w"))) return FALSE;

	fwprintf(fp,L"%.10lf\n",dRes);
	fwprintf(fp,L"%lf\n",0.0);
	fwprintf(fp,L"%lf\n",0.0);
	fwprintf(fp,L"%.10lf\n",-dRes);
	fwprintf(fp,L"%.10lf\n",dULx);
	fwprintf(fp,L"%.10lf\n",dULy);

	fclose(fp);

	return TRUE;
}




//Метод вычисляет по координатам в метрах координаты в пикселах 
BOOL RasterFile::calcFromProjectionToPixels (double dblX, double dblY, int *nX, int *nY)
{
	if (!this->m_isGeoReferenced) return FALSE;
	
	(*nX) = (int)ceil(((dblX - this->m_dULx)/this->m_dResolution)-0.5);
	(*nY) = (int)ceil(((this->m_dULy-dblY)/this->m_dResolution)-0.5);

	return TRUE;
}



RasterFile::RasterFile()
{
	///*
	FORMAT_JPEG = L"JPEG";
	FORMAT_GIF = L"GIF";
	FORMAT_PNG = L"PNG";
	FORMAT_GTIFF = L"GTIFF";
	FORMAT_BMP = L"BMP";
	FORMAT_HFA = L"HFA";
	//*/
	m_poDataset = NULL;
	m_strRasterFile	= L"";
	m_nBands			= 0;
	//m_strImageFormat	= L"";
	m_isGeoReferenced = FALSE;
	m_nNoDataValue = 0;
}

RasterFile::RasterFile(wstring strRasterFile, BOOL isGeoReferenced)
{
	FORMAT_JPEG = L"JPEG";
	FORMAT_GIF = L"GIF";
	FORMAT_PNG = L"PNG";
	FORMAT_GTIFF = L"GTIFF";
	FORMAT_BMP = L"BMP";
	FORMAT_HFA = L"HFA";
	//*/
	m_poDataset = NULL;
	m_strRasterFile	= L"";
	m_nBands			= 0;
	//m_strImageFormat	= L"";
	m_isGeoReferenced = FALSE;
	m_nNoDataValue = 0;
	
	init(strRasterFile,isGeoReferenced);
}


RasterFile::~RasterFile(void)
{
	delete_all();
}




//Метод вычисляет по координатам в пикселах координаты в метрах
BOOL RasterFile::calcFromPixelsToProjection (int nX, int nY, double *dblX, double *dblY)
{
	if (!m_isGeoReferenced) return FALSE;
	
	(*dblX) = this->m_dULx + nX*this->m_dResolution;
	(*dblY) = this->m_dULy - nY*this->m_dResolution;
	
	return TRUE;
}




void RasterFile::GetPixelSize (int &nWidth, int &nHeight)
{
	if (this->m_poDataset == NULL)
		nWidth=0;
	else
		nWidth = this->m_poDataset->GetRasterXSize();

	if (this->m_poDataset == NULL)
		nHeight=0;
	else
		nHeight = this->m_poDataset->GetRasterYSize();
}

void RasterFile::GetGeoReference (double &dULx, double &dULy, double &dRes)
{
	if (this->m_isGeoReferenced)
	{
		dULx = this->m_dULx;
		dULy = this->m_dULy;
		dRes = this->m_dResolution;
	}
	else
	{
		dULx =0;
		dULy =0;
		dRes =0;
	}
}

double	RasterFile::GetResolution()
{
		return this->m_dResolution;
}


/*
BOOL RasterFile::createTifFileInMercatorProjection (wstring fileName, int width, int height, int bands,  double ulx, double uly, double res)
{
	GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	string fileNameUTF8;
	wstrToUtf8(fileNameUTF8,fileName);

	GDALDataset	*poDataset = poDriver->Create(fileNameUTF8.c_str(),width,height,bands,GDT_Byte,NULL);
	if (poDataset == NULL) return FALSE;

	//poDataset->SetProjection("merc +lon_0=0 +k=1 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs +towgs84=0,0,0");
	
	//poDataset->SetProjection("PROJCS[\"WGS_84_WORLD_MERCATOR\",GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]],PROJECTION[\"Mercator\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"Meter\",1],PARAMETER[\"standard_parallel_1\",0.0]]");
	
	poDataset->SetProjection("PROJCS[\"unnamed\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]]]");

	double	geotransform[6];
	geotransform[0] = ulx; geotransform[1] = res; geotransform[2] = 0; geotransform[3] = uly; geotransform[4] = 0; geotransform[5] = -res;
	poDataset->SetGeoTransform(geotransform);

	int n = 4000; 
	for (int top = 0; top <height; top +=n )
	{
		int h  = (top+n>height) ? top + n -height : n;
		unsigned int N =  width*h*bands;
		BYTE *data = new BYTE[N];
		for (unsigned int k = 0; k<N; k++)
			data[k] = 0;
		if (CE_Failure	== poDataset->RasterIO(GF_Write,0,top,width,h,data,width,h,GDT_Byte,bands,NULL,0,0,0))
		{
			GDALClose(poDataset);
			return FALSE;
		}

		delete[]data;
	}
	
	//if (CE_Failure	== poDataset->RasterIO(GF_Write,0,0,oImageBuffer.getXSize(),oImageBuffer.getYSize(),(BYTE*)oImageBuffer.getData(),oImageBuffer.getXSize(),oImageBuffer.getYSize(),GDT_Byte,oImageBuffer.getBandsCount(),NULL,0,0,0))
	//{
	//	GDALClose(poDataset);
	//	return FALSE;
	//}
	
	GDALClose(poDataset);
	return TRUE;
}
*/







OGREnvelope RasterFile::GetEnvelope()
{
	OGREnvelope oEnvelope;
	oEnvelope.MinX = this->m_dULx;
	oEnvelope.MaxY = this->m_dULy;
	oEnvelope.MaxX = this->m_dULx + this->m_nWidth*this->m_dResolution;
	oEnvelope.MinY = this->m_dULy - this->m_nHeight*this->m_dResolution;

	return oEnvelope;
};

GDALDataset*	RasterFile::getDataset()
{
	return this->m_poDataset;
}


BOOL	RasterFile::getSpatialRef(OGRSpatialReference	&oSR)
{
	const char* strProjRef = GDALGetProjectionRef(this->m_poDataset);
	if (OGRERR_NONE!=oSR.SetFromUserInput(strProjRef))
	{
		if (FileExists(RemoveExtension(this->m_strRasterFile)+L".prj"))
		{
			wstring prjFile		= RemoveExtension(this->m_strRasterFile)+L".prj";
			string prjFileUTF8;
			wstrToUtf8(prjFileUTF8,prjFile);
			if (OGRERR_NONE==oSR.SetFromUserInput(prjFileUTF8.c_str())) return TRUE;
		}
	}
	else return TRUE;

	return FALSE;
}

BOOL	RasterFile::getDefaultSpatialRef (OGRSpatialReference	&oSRS, MercatorProjType mercType)
{
	if (fabs(this->m_dULx)<180 && fabs(this->m_dULy)<90)
	{
		oSRS.SetWellKnownGeogCS("WGS84"); 
	}
	else
	{
		if (mercType == WORLD_MERCATOR)
		{
			oSRS.SetWellKnownGeogCS("WGS84");
			oSRS.SetMercator(0,0,1,0,0);
		}
		else
		{
			oSRS.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
		}
	}

	return TRUE;
}


OGREnvelope	RasterFile::getEnvelopeInMercator (MercatorProjType	mercType)
{
	const int numPoints = 100;
	OGRSpatialReference oSRS;

	if (!getSpatialRef(oSRS)) getDefaultSpatialRef(oSRS,mercType);

	OGRLinearRing	oLR;
	OGREnvelope		srcEnvelope = GetEnvelope();
	oLR.addPoint(srcEnvelope.MinX,srcEnvelope.MaxY);
	for (int i=1; i<numPoints;i++)
		oLR.addPoint(srcEnvelope.MinX + i*(srcEnvelope.MaxX-srcEnvelope.MinX)/numPoints , srcEnvelope.MaxY);
	
	oLR.addPoint(srcEnvelope.MaxX,srcEnvelope.MaxY);
	for (int i=1; i<numPoints;i++)
		oLR.addPoint(srcEnvelope.MaxX,srcEnvelope.MaxY - i*(srcEnvelope.MaxY-srcEnvelope.MinY)/numPoints);

	oLR.addPoint(srcEnvelope.MaxX,srcEnvelope.MinY);
	for (int i=1; i<numPoints;i++)
		oLR.addPoint(srcEnvelope.MaxX - i*(srcEnvelope.MaxX-srcEnvelope.MinX)/numPoints , srcEnvelope.MinY);
	
	oLR.addPoint(srcEnvelope.MinX,srcEnvelope.MinY);
	for (int i=1; i<numPoints;i++)
		oLR.addPoint(srcEnvelope.MinX,srcEnvelope.MinY + i*(srcEnvelope.MaxY-srcEnvelope.MinY)/numPoints);
	oLR.closeRings();

	OGRPolygon		oPoly;
	oPoly.addRing(&oLR);
	VectorBorder	mercPolygon;
	VectorFile::transformOGRPolygonToMerc(&oPoly,mercPolygon,mercType,&oSRS);
	
	return	mercPolygon.GetEnvelope();
}




/*

BOOL RasterFile::GenerateImage (RasterBuffer &oImageBuffer,
										wstring	strRasterFile,
										wstring	strImageFormat)
{

	GDALDriver *poDriver = NULL;
	if (strImageFormat == this->FORMAT_BMP) poDriver = GetGDALDriverManager()->GetDriverByName("BMP");
	else if(strImageFormat == this->FORMAT_GTIFF) poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	else if(strImageFormat == this->FORMAT_HFA) poDriver = GetGDALDriverManager()->GetDriverByName("HFA");
	else if(strImageFormat == this->FORMAT_JPEG) poDriver = GetGDALDriverManager()->GetDriverByName("JPEG");
	else if(strImageFormat == this->FORMAT_PNG) poDriver = GetGDALDriverManager()->GetDriverByName("PNG");


	if (poDriver==NULL) return FALSE;

	string strRasterFileUTF8;
	wstrToUtf8(strRasterFileUTF8,strRasterFile);


	if ((strImageFormat == this->FORMAT_BMP)||
		(strImageFormat== this->FORMAT_HFA)||
		(strImageFormat == this->FORMAT_GTIFF))
	{
		GDALDataset	*poData = poDriver->Create(strRasterFileUTF8.c_str(),oImageBuffer.getXSize(),oImageBuffer.getYSize(),
							oImageBuffer.getBandsCount(),GDT_Byte,NULL);
		if (poData == NULL) return FALSE;

		//if (CE_Failure == m_poDataset->RasterIO(GF_Read,nXOff_adjust,nYOff_adjust,nWidth_adjust,nHeight_adjust,pData,nBufferXSize_adjust,nBufferYSize_adjust,GDT_Byte,m_nBands,NULL,0,nWidth,nHeight*nWidth))
		if (CE_Failure	== poData->RasterIO(GF_Write,0,0,oImageBuffer.getXSize(),oImageBuffer.getYSize(),(BYTE*)oImageBuffer.getData(),oImageBuffer.getXSize(),oImageBuffer.getYSize(),GDT_Byte,oImageBuffer.getBandsCount(),NULL,0,0,0))
		{
			GDALClose(poData);
			return FALSE;
		}

		GDALClose(poData);
	}
	//poDriver->

	return TRUE;
}
BOOL RasterFile::cutToBufferByVectorBorder	(VectorBorder		&oVectorBorder,
											 RasterBuffer			&oBuffer,
											 double					_min_x,
											 double					_min_y,
											 double					_max_x,
											 double					_max_y,
											 int					Zero
											 )
{
	
	double min_x, min_y, max_y, max_x;
	if ((_min_x==0)&&(_max_x==0)) 
	{
		min_x = this->m_dULx;
		min_y=this->m_dULy;
		max_y = this->m_dULy+this->m_dResolution*this->m_nHeight;
		max_x=this->m_dULx+this->m_dResolution*this->m_nWidth;
		if (!oVectorBorder.AdjustBounds(&min_x,&min_y,&max_x,&max_y)) 
		{
			//strError+="Zero intersection - empty or bad area";
			//throw ERROR;
			oBuffer.createBuffer(this->m_nBands,this->m_nHeight,this->m_nWidth);
			oBuffer.initByValue(Zero);
			return FALSE;
		};
	}
	else
	{
		min_x = _min_x;
		max_x = _max_x;
		min_y = _min_y;
		max_y = _max_y;
	}

	int right,bottom,left,top; 
	this->calcFromProjectionToPixels(min_x,max_y,&left,&top);
	this->calcFromProjectionToPixels(max_x,min_y,&right,&bottom);
	
	if (!this->getToBuffer(oBuffer,left,top,right-left,bottom-top))
	{
		wcout<<L"RasterFile::cutToBufferByVectorBorder: can't read data from image to buffer"<<endl;
		return FALSE;
	};
	

	BYTE	*pData = (BYTE*)oBuffer.getData();
	int		nBands = oBuffer.getBandsCount();
	list<double>	PointsIntersection;

	for (double y_curr=max_y;y_curr>=min_y;y_curr-=this->m_dResolution)
	{
		int nPointsOfIntersection;
		double *pPoints = oVectorBorder.CalcHorizontalLineIntersection(y_curr,nPointsOfIntersection);
		PointsIntersection.clear();
		for (int k=0;k<nPointsOfIntersection;k++)
			PointsIntersection.push_back(pPoints[k]);
		delete[] pPoints;

		if (PointsIntersection.size()%2 == 0)
		{
			if (PointsIntersection.size()==0) oBuffer.makeZero(0,(int)floor(0.5+((max_y-y_curr)/this->m_dResolution)),right-left,1,Zero);	
			else
			{
				//PointsIntersection.sort();

				PointsIntersection.push_front(min_x);
				PointsIntersection.push_back (max_x);
				PointsIntersection.sort();
				int x1,x2,y;
				for (list<double>::iterator iter = PointsIntersection.begin();iter!=PointsIntersection.end();iter++)
				{
					this->calcFromProjectionToPixels((*iter),y_curr,&x1,&y);
					//if (x1<left) continue;
					iter++;
					this->calcFromProjectionToPixels((*iter),y_curr,&x2,&y);
					if (x1<left) x1=left;
					if (x1>=right) break;
					if (x2>right) x2=right;
					if (x2<left) continue;
					oBuffer.makeZero(x1-left,y-top,x2-x1,1,Zero);
				}
			}
		}
	}

	return TRUE;
}


BOOL RasterFile::cutAreaUsingPixels		(const int	nPoints, 
										int			*poPointsArray,
										wstring		strImageFormat,
										wstring		strAreaImage,
										BOOL		bGenerateWldFile
										)
{
	
	if (nPoints ==0) return FALSE;

	double	* Points = NULL;
	Points	= new double[2*nPoints];

	for (int i=0;i<2*nPoints;i++)
		Points[i]=poPointsArray[i];

	if (this->m_poDataset == NULL) 
	{
		wcout<<L"Image isn't initialized"<<endl;	
		return FALSE;
	}

	VectorBorder oVectorBorder;

	
	if (!oVectorBorder.InitByPoints(nPoints,Points))
	{
		wcout<<L"Error: RasterFile::cutAreaUsingPixels: bad polygon, can't init shp-object"<<endl;
		return FALSE;
	}
	RasterBuffer oBuffer;
	if (!cutToBufferByVectorBorder(oVectorBorder,oBuffer)) return FALSE;
	

	if (!GenerateImage(oBuffer,strAreaImage,strImageFormat))
	{
		wcout<<L"Error: RasterFile::cutAreaUsingPixels: can't write image file from buffer"<<endl;
		return FALSE;
	}
	
	delete(Points);

	return TRUE;
}


BOOL RasterFile::MergeTilesByVectorBorder(	RasterFile			&oBackGroundImage,
											VectorBorder	&oVectorBorder,
											double				min_x,
											double				min_y,
											double				max_x,
											double				max_y,
											RasterBuffer		&oBuffer
										)
{
		
	
		RasterBuffer oBack;
		int n_min_x,n_max_y,n_min_y,n_max_x;
		oBackGroundImage.calcFromProjectionToPixels(min_x,max_y,&n_min_x,&n_min_y);
		oBackGroundImage.calcFromProjectionToPixels(max_x,min_y,&n_max_x,&n_max_y);
		//oBack.m
		oBackGroundImage.getToBuffer(oBack,n_min_x,n_min_y,n_max_x-n_min_x,n_max_y-n_min_y);


		RasterBuffer oFore;
		this->cutToBufferByVectorBorder(oVectorBorder,oFore,min_x,min_y,max_x,max_y,RasterBuffer::ZeroColor);
		

		int n_width = oBack.getXSize(), n_height = oBack.getYSize();
		//RasterBuffer oBuffer;
		oBuffer.createBuffer(oBack.getBandsCount(),n_width,n_height);
		memcpy(oBuffer.getData(),oBack.getData(),oBack.getBandsCount()*n_width*n_height);
		
		//if (!((min_x==4419584)&&(max_y==5965824)))
		//{
			BYTE *pData = (BYTE*)oBuffer.getData();
			BYTE *pForeData = (BYTE*)oFore.getData();
			int n = n_width*n_height;

			for (int i=0;i<n_height;i++)
			{
				int m = i*n_width;
				for (int j=0;j<n_width;j++)
				{
					if (!((pForeData[m+j]==RasterBuffer::ZeroColor)&&(pForeData[m+j+n]==RasterBuffer::ZeroColor)&&(pForeData[m+j+n+n]==RasterBuffer::ZeroColor)))
					{
						pData[m+j] = pForeData[m+j];
						pData[m+j+n] = pForeData[m+j+n];
						pData[m+j+n+n] = pForeData[m+j+n+n];
					}
				}
			}
		//}
	
		//oBuffer.SaveToJpgFile(strTileName);
		return TRUE;
};
*/

void RasterFile::readMetaData ()
{
	const char *proj_data = this->m_poDataset->GetProjectionRef();
	OGRSpatialReference oSpRef;
	OGRErr err = oSpRef.SetWellKnownGeogCS(proj_data);
};



BundleOfRasterFiles::BundleOfRasterFiles(void)
{
	//this->m_poImages = NULL;
	//this->m_nLength = 0;
}

void BundleOfRasterFiles::close_all(void)
{
	
	//m_nLength = 0;
	//m_poImages = NULL;
	//m_strFilesList.empty();
	//this->m_oImagesBounds.empty();
	//this->m_poImagesBorders.empty();

	for (list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end();iter++)
	{
		delete((*iter).second.second);
	}
	dataList.empty();

}

BundleOfRasterFiles::~BundleOfRasterFiles(void)
{
	close_all();
}


int BundleOfRasterFiles::ImagesCount()
{
	return dataList.size();
}


int BundleOfRasterFiles::InitFromFilesList(list<wstring> strFilesList, MercatorProjType mercType, double dShiftX, double dShiftY)
{
	close_all();

	for (std::list<wstring>::iterator iter = strFilesList.begin(); iter!=strFilesList.end(); iter++)
		addItemToBundle((*iter),VectorFile::GetVectorFileByRasterFileName(*iter),dShiftX,dShiftY);
	this->mercType = mercType;
	return dataList.size();
}


BOOL	BundleOfRasterFiles::addItemToBundle (wstring rasterFile, wstring	vectorFile, double dShiftX, double dShiftY)
{	
	RasterFile oImage;
	if (!oImage.init(rasterFile,TRUE,dShiftX,dShiftY))
	{
		wcout<<L"Error: can't init. image: "<<rasterFile<<endl;
		return 0;
	}

	VectorBorder	*border = new VectorBorder();
	if (!VectorFile::OpenAndCreatePolygonInMercator(vectorFile,*border,mercType)) border = NULL;

	pair<wstring,pair<OGREnvelope,VectorBorder*>> p;
	p.first			= rasterFile;
	p.second.first	= oImage.getEnvelopeInMercator(mercType);
	p.second.second = border;
	dataList.push_back(p);
	return TRUE;
}



int BundleOfRasterFiles::InitFromFolder(wstring folderPath, wstring rasterType,  MercatorProjType mercType,  double dShiftX, double dShiftY)
{
	list<wstring> strFilesList;

	if (!FindFilesInFolderByExtension (strFilesList,folderPath,rasterType,false))
		return 0;
	return InitFromFilesList(strFilesList,mercType,dShiftX, dShiftY);
}


int	BundleOfRasterFiles::InitFromRasterFile (wstring rasterFile, MercatorProjType mercType, wstring vectorFile, double dShiftX, double dShiftY)
{
	this->mercType = mercType;
	return addItemToBundle(rasterFile,vectorFile,dShiftX,dShiftY);
}




/*
wstring BundleOfRasterFiles::BestImage(double min_x, double min_y, double max_x, double max_y, double &max_intersection)
{
	if (m_nLength == 0) return L"";
	if (m_nLength == 1) return (*m_strFilesList.begin());
	list<wstring>::iterator iter;
	wstring strBestImage;
    max_intersection = 0;
	list<OGREnvelope>::iterator iter_ = m_oImagesBounds.begin(); 
	for (iter = m_strFilesList.begin();iter!=m_strFilesList.end();iter++)
	{
		
		OGREnvelope oEnvelope = (*iter_);
		double min_x_ = max(min_x,oEnvelope.MinX);
		double max_x_ = min(max_x,oEnvelope.MaxX);
		double max_y_ = min(max_y,oEnvelope.MaxY);
		double min_y_ = max(min_y,oEnvelope.MinY);
		double intersection = ((max_x_-min_x_)*(max_y_-min_y_))/((max_x-min_x)*(max_y-min_y));
		if ((intersection>max_intersection)&&(max_x_-min_x_>=0))
		{
			max_intersection = intersection;
			strBestImage = (*iter);
		}
		iter_++;
	}

	return strBestImage;
}
*/

/*
OGREnvelope BundleOfRasterFiles::GetEnvelopeOfImage(wstring strFileName)
{
	list<OGREnvelope>::iterator iter_ = this->m_oImagesBounds.begin();
	OGREnvelope oEnvelope;

	for (list<wstring>::iterator iter = this->m_strFilesList.begin();iter!=this->m_strFilesList.end();iter++)
	{
		if ((*iter)==strFileName) return *iter_;
		iter_++;
	}
	//return oEnvelope;
}
*/




list<wstring>	BundleOfRasterFiles::GetFilesList()
{
	std::list<wstring> filesList;
	for (std::list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end(); iter++)
		filesList.push_back((*iter).first);

	return filesList;
	//return this->m_strFilesList;
}

OGREnvelope BundleOfRasterFiles::getMercatorEnvelope()
{
	OGREnvelope	oEnvelope;
	oEnvelope.MaxY=(oEnvelope.MaxX = -1e+100);oEnvelope.MinY=(oEnvelope.MinX = 1e+100);
	if (dataList.size() == 0) return oEnvelope;

	for (list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end();iter++)
	{
		
		oEnvelope = ((*iter).second.second != NULL) ? CombineEnvelopes(	oEnvelope,
																		InetersectEnvelopes((*iter).second.first,
																							(*iter).second.second->GetEnvelope()
																							)
																		):
													 CombineEnvelopes(	oEnvelope,(*iter).second.first);
	}
	return oEnvelope; 
}


int	BundleOfRasterFiles::calculateNumberOfTiles (int zoom)
{
	int n = 0;
	double		res = MercatorTileGrid::calcResolutionByZoom(zoom);

	int minx,maxx,miny,maxy;
	MercatorTileGrid::calcTileRange(getMercatorEnvelope(),zoom,minx,miny,maxx,maxy);
	
	for (int curr_x = minx; curr_x<=maxx; curr_x++)
	{
		for (int curr_y = miny; curr_y<=maxy; curr_y++)
		{
			OGREnvelope tileEnvelope = MercatorTileGrid::calcEnvelopeByTile(zoom,curr_x,curr_y);//?
			if (intersects(tileEnvelope)) n++;
		}
	}

	return n;
}



int		BundleOfRasterFiles::calculateBestMercZoom()
{
	if (dataList.size()==0) return -1;

	RasterFile rf((*dataList.begin()).first,1);
	int srcWidth = 0, srcHeight = 0;
	rf.GetPixelSize(srcWidth,srcHeight);
	if (srcWidth<=0 || srcHeight <= 0) return false;
	OGREnvelope oMercEnvelope = (*dataList.begin()).second.first;

	double srcRes = min((oMercEnvelope.MaxX - oMercEnvelope.MinX)/srcWidth,(oMercEnvelope.MaxY - oMercEnvelope.MinY)/srcHeight);
	if (srcRes<=0) return -1;

	for (int z=0; z<23; z++)
	{
		if (MercatorTileGrid::calcResolutionByZoom(z) <srcRes || 
			(fabs(MercatorTileGrid::calcResolutionByZoom(z)-srcRes)/MercatorTileGrid::calcResolutionByZoom(z))<0.2) return z;
	}

	return -1;
}


/*
OGREnvelope		BundleOfRasterFiles::getEnvelopeOnlyByVectorBorders()
{
	OGREnvelope	oEnvelope;
	oEnvelope.MaxY=(oEnvelope.MaxX = -1e+100);oEnvelope.MinY=(oEnvelope.MinX = 1e+100);
	if (dataList.size() == 0) return oEnvelope;

	for (list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end();iter++)
	{
		oEnvelope = CombineEnvelopes(oEnvelope,(*iter).second.second->GetEnvelope());
	}

	return oEnvelope; 
}
*/



list<wstring>	 BundleOfRasterFiles::getFilesListByEnvelope(OGREnvelope mercatorEnvelope)
{
	std::list<wstring> oList;


	for (list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end();iter++)
	{
		if ((*iter).second.second->Intersects(mercatorEnvelope)) oList.push_back((*iter).first);

	}
	
	return oList;
}


BOOL	BundleOfRasterFiles::intersects(OGREnvelope mercatorEnvelope)
{
	for (list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end();iter++)
	{
		if (mercatorEnvelope.Intersects((*iter).second.first))
		{
			if ((*iter).second.second != NULL)
			{
				if ((*iter).second.second->Intersects(mercatorEnvelope)) return TRUE;
			}
			else return TRUE;			
		}
	}
	
	return FALSE;
}



BOOL	BundleOfRasterFiles::createBundleBorder (VectorBorder &border)
{
	OGRGeometry	*poUnion = NULL;
		
	for (list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end(); iter++)
	{
		if (poUnion == NULL)
		{
			poUnion = (*iter).second.second->GetOGRGeometry();
			continue;
		}
		OGRGeometry *temp = poUnion->Union((*iter).second.second->GetOGRGeometry());
		if (temp==NULL)
		{
			poUnion->empty();
			return TRUE;
		}
		poUnion->empty();
		poUnion = temp;

		/*
		VectorBorder oImageBorder;
		VectorFile::OpenAndCreatePolygonInMercator(VectorFile::GetVectorFileByRasterFileName(*iter),oImageBorder);//(wstring strVectorFile, VectorBorder &oPolygon);
		if (poUnion == NULL)
		{
			poUnion = oImageBorder.GetOGRGeometry()->clone();
			continue;
		}
		
		OGRGeometry *temp = poUnion->Union(oImageBorder.GetOGRGeometry());
		poUnion->empty();
		poUnion = temp;
		*/
	}

	border.InitByOGRGeometry(poUnion);
	poUnion->empty();

	/*
	RasterFile oRasterFile(outputTile,1);
	VectorBorder envelopeBorder;
	envelopeBorder.InitByEnvelope(oRasterFile.GetEnvelope());
	OGRGeometry *poIntersection = poUnion->Intersection(envelopeBorder.GetOGRGeometry());
	poUnion->empty();
	*/


	/*
	if (!VectorFile::CreateMercatorVectorFileByGeometry (poIntersection, RemoveExtension(outputTile)+L".shp"))
	{
		wcout<<L"can't create border for tile "<<outputTile<<endl;

	}
	poIntersection->empty();
	*/
	//return TRUE;
	return TRUE;
}


//ToDo
BOOL BundleOfRasterFiles::warpMercToBuffer (int zoom,	OGREnvelope	oMercEnvelope, RasterBuffer &oBuffer, int *pNoDataValue, BYTE *pDefaultColor)
{
	//создать виртуальный растр по oMercEnvelope и zoom
	//создать объект GDALWarpOptions 
	//вызвать ChunkAndWarpImage
	//вызвать RasterIO для виртуального растра
	//удалить виртуальный растр, удалить все объекты
	//создать RasterBuffer



	// Open input raster and create source dataset
	RasterFile		inputRF;
	if (!inputRF.init((*dataList.begin()).first,true))
	{
		cout<<L"Error: can't open input file: "<<(*dataList.begin()).first<<endl;
		return FALSE;
	}

	GDALDataset*	poSrcDS = inputRF.getDataset();
	GDALDataType	eDT		= GDALGetRasterDataType(GDALGetRasterBand(poSrcDS,1));
	int				bands	= poSrcDS->GetRasterCount();
	double			res		=  MercatorTileGrid::calcResolutionByZoom(zoom);
    
	
	//Initialize destination virtual dataset
	int				bufWidth	= int(((oMercEnvelope.MaxX - oMercEnvelope.MinX)/res)+0.5);
	int				bufHeight	= int(((oMercEnvelope.MaxY - oMercEnvelope.MinY)/res)+0.5);
	double			dfErrorThreshold = 0.125;
	GDALDataset*	poVrtDS = (GDALDataset*)GDALCreate(
								GDALGetDriverByName("GTiff"),
								"/vsimem/tiffinmem",
								bufWidth,
								bufHeight,
								bands,
								eDT,
								NULL
								);

	double			geotransform[6];

	geotransform[0] = oMercEnvelope.MinX;

	geotransform[1] = res;
	geotransform[2] = 0;
	geotransform[3] = oMercEnvelope.MaxY;
	geotransform[4] = 0;
	geotransform[5] = -res;
	poVrtDS->SetGeoTransform(geotransform);
	

	// Get Source coordinate system and set destination  
	char *pszSrcWKT;
	char *pszDstWKT = NULL;
	OGRSpatialReference inputSRS;
	if (!inputRF.getSpatialRef(inputSRS))
	{
		if(!inputRF.getDefaultSpatialRef(inputSRS,mercType))
		{
			cout<<L"Error: can't read spatial reference from input file: "<<(*dataList.begin()).first<<endl;
			return FALSE;
		}
	}
	inputSRS.exportToWkt(&pszSrcWKT);
	CPLAssert( pszSrcWKT != NULL && strlen(pszSrcWKT) > 0 );


	OGRSpatialReference outputSRS;
	if (mercType == WORLD_MERCATOR)
	{
		outputSRS.SetWellKnownGeogCS("WGS84");
		outputSRS.SetMercator(0,0,1,0,0);
	}
	else
	{
		outputSRS.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
	}
	outputSRS.exportToWkt( &pszDstWKT );

	poVrtDS->SetProjection(pszDstWKT);

	//poVrtDS->RasterIO(	GF_Write,0,0,bufWidth,bufHeight,oBuffer.getData(),
	//					bufWidth,bufHeight,oBuffer.getDataType(),
	//					oBuffer.getBandsCount(),NULL,0,0,0);
	//GDALFlushCache(poVrtDS);

	

	GDALWarpOptions *psWarpOptions = GDALCreateWarpOptions();

    psWarpOptions->hSrcDS = poSrcDS;
    psWarpOptions->hDstDS = poVrtDS;
	psWarpOptions->dfWarpMemoryLimit = 250000000; 
	

    psWarpOptions->nBandCount = 0;
   // psWarpOptions->pfnProgress = GDALTermProgress;   
	psWarpOptions->pfnProgress = PrintNoProgress;  

    // Establish reprojection transformer. 

	psWarpOptions->pTransformerArg = 
            GDALCreateApproxTransformer( GDALGenImgProjTransform, 
                                          GDALCreateGenImgProjTransformer(  poSrcDS, 
																			pszSrcWKT, 
																			poVrtDS,//hDstDS,
																			pszDstWKT,//GDALGetProjectionRef(hDstDS), 
																			FALSE, 0.0, 1 ),
										 dfErrorThreshold );

	psWarpOptions->pfnTransformer = GDALApproxTransform;

    // Initialize and execute the warp operation. 
	GDALWarpOperation oOperation;
	oOperation.Initialize( psWarpOptions );
    oOperation.ChunkAndWarpImage( 0,0,bufWidth,bufHeight);
	

	GDALDestroyApproxTransformer(psWarpOptions->pTransformerArg );
    GDALDestroyWarpOptions( psWarpOptions );
	//delete[]pszSrcWKT;
	OGRFree(pszDstWKT);
	OGRFree(pszSrcWKT);
	//GDALClose( poSrcDS );

	oBuffer.createBuffer(bands,bufWidth,bufHeight,NULL,eDT,pNoDataValue);
	int noDataValueFromFile = 0;
	if	(pNoDataValue) oBuffer.initByNoDataValue(pNoDataValue[0]);
	else if (pDefaultColor) oBuffer.initByRGBColor(pDefaultColor); 
	else if (inputRF.getNoDataValue(&noDataValueFromFile)) oBuffer.initByNoDataValue(noDataValueFromFile);

	 
	poVrtDS->RasterIO(	GF_Read,0,0,bufWidth,bufHeight,oBuffer.getDataRef(),
						bufWidth,bufHeight,oBuffer.getDataType(),
						oBuffer.getBandsCount(),NULL,0,0,0);
	//GDALFlushCache(poVrtDS);
	GDALClose(poVrtDS);
	
	//BYTE * pDataDstBuf = VSIGetMemFileBuffer("/vsimem/tiffinmem",&length, FALSE);
	//size = length;
	//GDALClose(poDS);
	//memcpy((pDataDst = new BYTE[size]),pDataDstBuf,size);
	VSIUnlink("/vsimem/tiffinmem");
	



	//  GDALClose( poDstDS );


   
	return TRUE;
}