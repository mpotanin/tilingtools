#include "StdAfx.h"
#include "RasterFile.h"
#include "GeometryFuncs.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"
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


void RasterFile::delete_all()
{
	delete(m_poDataset);
	m_poDataset = NULL;
	m_strRasterFile	= L"";
	m_nBands			= 0;
	//m_strImageFormat	= L"";
	m_isGeoReferenced = FALSE;
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
	this->m_nHeight = m_poDataset->GetRasterYSize();
	this->m_nWidth	= m_poDataset->GetRasterXSize();
	this->m_nNoDataValue = m_poDataset->GetRasterBand(1)->GetNoDataValue(&this->m_bNoDataValueDefined);
	this->m_oGDALDataType	= m_poDataset->GetRasterBand(1)->GetRasterDataType();


	if (isGeoReferenced)
	{
		double GeoTransform[6];
		if (m_poDataset->GetGeoTransform(GeoTransform)== CE_None)			
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

/*
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
*/





RasterFile::RasterFile()
{
	///*
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



void RasterFile::getPixelSize (int &nWidth, int &nHeight)
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

void RasterFile::getGeoReference (double &dULx, double &dULy, double &dRes)
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

double	RasterFile::getResolution()
{
		return this->m_dResolution;
}







OGREnvelope RasterFile::GetEnvelope()
{
	OGREnvelope oEnvelope;
	oEnvelope.MinX = this->m_dULx;
	oEnvelope.MaxY = this->m_dULy;
	oEnvelope.MaxX = this->m_dULx + this->m_nWidth*this->m_dResolution;
	oEnvelope.MinY = this->m_dULy - this->m_nHeight*this->m_dResolution;

	return oEnvelope;
};

GDALDataset*	RasterFile::getGDALDatasetRef()
{
	return this->m_poDataset;
}

BOOL RasterFile::readSpatialRefFromMapinfoTabFile (wstring tabFilePath, OGRSpatialReference *poSRS)
{
	FILE *fp;
	if (!(fp = _wfopen(tabFilePath.c_str(),L"r"))) return FALSE;
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *pTabFileData = new char[size];
	fread(pTabFileData,sizeof(char),size,fp);
	string strTabFileData(pTabFileData);
	delete[]pTabFileData;
	if (strTabFileData.find("CoordSys Earth Projection") == string::npos) return FALSE;
	int start_pos	= strTabFileData.find("CoordSys Earth Projection");
	int end_pos		= (strTabFileData.find('\n',start_pos) != string::npos)		? 
														strTabFileData.size() : 
														strTabFileData.find('\n',start_pos);
	string strMapinfoProj = strTabFileData.substr(start_pos, end_pos-start_pos+1);
	//cout<<"Mapinfo: "<<strProj<<endl;
	if (!OGRERR_NONE==poSRS->importFromMICoordSys(strMapinfoProj.c_str())) return FALSE;
	
	return TRUE;
}


BOOL	RasterFile::getSpatialRef(OGRSpatialReference	&oSR)
{
	const char* strProjRef = GDALGetProjectionRef(this->m_poDataset);
	if (OGRERR_NONE == oSR.SetFromUserInput(strProjRef)) return TRUE;

	if (FileExists(RemoveExtension(this->m_strRasterFile)+L".prj"))
	{
		wstring prjFile		= RemoveExtension(this->m_strRasterFile)+L".prj";
		string prjFileUTF8;
		wstrToUtf8(prjFileUTF8,prjFile);
		return 	(OGRERR_NONE==oSR.SetFromUserInput(prjFileUTF8.c_str()));	
	}
	else if (FileExists(RemoveExtension(this->m_strRasterFile)+L".tab"))
		return readSpatialRefFromMapinfoTabFile(RemoveExtension(this->m_strRasterFile)+L".tab",&oSR);

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
	OGRSpatialReference rasterFileSR;

	if (!getSpatialRef(rasterFileSR)) getDefaultSpatialRef(rasterFileSR,mercType);

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

	oLR.assignSpatialReference(&rasterFileSR);
	
	OGRSpatialReference oSpatialMerc;
	MercatorTileGrid::setMercatorSpatialReference(mercType,&oSpatialMerc);
	
	oLR.transformTo(&oSpatialMerc);
	OGRwkbGeometryType type = oLR.getGeometryType();

	VectorBorder::adjustFor180DegreeIntersection(&oLR);


	OGREnvelope resultEnvelope;
	oLR.getEnvelope(&resultEnvelope);

	return	resultEnvelope;
}	





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


int	BundleOfRasterFiles::init (wstring inputPath, MercatorProjType mercType, wstring vectorFile, double dShiftX, double dShiftY)
{
	close_all();
	list<wstring> strFilesList;
	if (!FindFilesInFolderByPattern(strFilesList,inputPath)) return 0;

	this->mercType = mercType;
	for (std::list<wstring>::iterator iter = strFilesList.begin(); iter!=strFilesList.end(); iter++)
	{
		if (strFilesList.size()==1) addItemToBundle((*iter),vectorFile,dShiftX,dShiftY);
		else addItemToBundle((*iter),VectorBorder::getVectorFileNameByRasterFileName(*iter),dShiftX,dShiftY);
	}
	return dataList.size();

	return 1;
}




BOOL	BundleOfRasterFiles::addItemToBundle (wstring rasterFile, wstring	vectorFile, double dShiftX, double dShiftY)
{	
	RasterFile oImage;
	if (!oImage.init(rasterFile,TRUE,dShiftX,dShiftY))
	{
		wcout<<L"Error: can't init. image: "<<rasterFile<<endl;
		return 0;
	}

	VectorBorder	*border = VectorBorder::createFromVectorFile(vectorFile,mercType);
	pair<wstring,pair<OGREnvelope,VectorBorder*>> p;
	p.first			= rasterFile;
	p.second.first	= oImage.getEnvelopeInMercator(mercType);
	p.second.second = border;
	dataList.push_back(p);
	return TRUE;
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
																							(*iter).second.second->getEnvelope()
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
	rf.getPixelSize(srcWidth,srcHeight);
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
		if ((*iter).second.second->intersects(mercatorEnvelope)) oList.push_back((*iter).first);

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
				if ((*iter).second.second->intersects(mercatorEnvelope)) return TRUE;
			}
			else return TRUE;			
		}
	}
	
	return FALSE;
}


/*
BOOL	BundleOfRasterFiles::createBundleBorder (VectorBorder &border)
{
	OGRGeometry	*poUnion = NULL;
		
	for (list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end(); iter++)
	{
		if (poUnion == NULL)
		{
			poUnion = (*iter).second.second->getOGRGeometryRef();
			continue;
		}
		OGRGeometry *temp = poUnion->Union((*iter).second.second->getOGRGeometryRef());
		if (temp==NULL)
		{
			poUnion->empty();
			return TRUE;
		}
		poUnion->empty();
		poUnion = temp;

		VectorBorder oImageBorder;
		VectorFile::OpenAndCreatePolygonInMercator(VectorFile::GetVectorFileByRasterFileName(*iter),oImageBorder);//(wstring strVectorFile, VectorBorder &oPolygon);
		if (poUnion == NULL)
		{
			poUnion = oImageBorder.getOGRGeometryRef()->clone();
			continue;
		}
		
		OGRGeometry *temp = poUnion->Union(oImageBorder.getOGRGeometryRef());
		poUnion->empty();
		poUnion = temp;
		
	}

	border.InitByOGRGeometry(poUnion);
	poUnion->empty();

	
	return TRUE;
}
*/




BOOL BundleOfRasterFiles::warpMercToBuffer (int zoom,	OGREnvelope	oMercEnvelope, RasterBuffer &oBuffer, int *pNoDataValue, BYTE *pDefaultColor)
{
	//создать виртуальный растр по oMercEnvelope и zoom
	//создать объект GDALWarpOptions 
	//вызвать ChunkAndWarpImage
	//вызвать RasterIO для виртуального растра
	//удалить виртуальный растр, удалить все объекты
	//создать RasterBuffer

	if (dataList.size()==0) return FALSE;
	string fileNameUTF8;
	wstrToUtf8(fileNameUTF8,(*dataList.begin()).first);
	GDALDataset	*poSrcDS = (GDALDataset*)GDALOpen(fileNameUTF8.c_str(),GA_ReadOnly );
	if (poSrcDS==NULL)
	{
		wcout<<L"Error: can't open raster file: "<<(*dataList.begin()).first<<endl;
		return FALSE;
	}
	GDALDataType	eDT		= GDALGetRasterDataType(GDALGetRasterBand(poSrcDS,1));
	int				bands	= poSrcDS->GetRasterCount();
	BOOL			bNoDataValueFromFile;
	int				nNoDataValueFromFile = (int) poSrcDS->GetRasterBand(1)->GetNoDataValue(&bNoDataValueFromFile);
	GDALClose(poSrcDS);

	double			res			=  MercatorTileGrid::calcResolutionByZoom(zoom);
	int				bufWidth	= int(((oMercEnvelope.MaxX - oMercEnvelope.MinX)/res)+0.5);
	int				bufHeight	= int(((oMercEnvelope.MaxY - oMercEnvelope.MinY)/res)+0.5);
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
	OGRSpatialReference outputSRS;
	char *pszDstWKT = NULL;
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

	if (bNoDataValueFromFile) poVrtDS->GetRasterBand(1)->SetNoDataValue(nNoDataValueFromFile);
	

	for (list<pair<wstring,pair<OGREnvelope,VectorBorder*>>>::iterator iter = dataList.begin(); iter!=dataList.end();iter++)
	{
		//check if image envelope intersects destination buffer envelope
		if (!(*iter).second.first.Intersects(oMercEnvelope)) continue;
		
		// Open input raster and create source dataset
		if (dataList.size()==0) return FALSE;
		wstrToUtf8(fileNameUTF8,(*iter).first);
		poSrcDS = (GDALDataset*)GDALOpen(fileNameUTF8.c_str(),GA_ReadOnly );

	
		if (poSrcDS==NULL)
		{
			wcout<<L"Error: can't open raster file: "<<(*iter).first<<endl;
			continue;
		}
			
		// Get Source coordinate system and set destination  
		char *pszSrcWKT	= NULL;
		RasterFile	inputRF((*iter).first,TRUE);
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


		GDALWarpOptions *psWarpOptions = GDALCreateWarpOptions();

		psWarpOptions->hSrcDS = poSrcDS;
		psWarpOptions->hDstDS = poVrtDS;
		psWarpOptions->dfWarpMemoryLimit = 250000000; 
		double			dfErrorThreshold = 0.125;

		psWarpOptions->nBandCount = 0;
		
		
		//Init cutline for source file
		///*
		if ((*iter).second.second)
		{
			VectorBorder	*poBorder = (*iter).second.second;
			OGRGeometry		*poOGRGeometry = poBorder->getOGRGeometryTransformed(&inputSRS);
			double	rasterFileTransform[6];
			if (CE_None == inputRF.getGDALDatasetRef()->GetGeoTransform(rasterFileTransform))
			{
				if (!((rasterFileTransform[0] == 0.) &&(rasterFileTransform[1]==1.)))
					psWarpOptions->hCutline = poBorder->getOGRPolygonTransformedToPixelLine(&inputSRS,rasterFileTransform);
			}
		}
		//*/
		//psWarpOptions->hCutline = ((OGRMultiPolygon*)(*iter).second.second)->getGeometryRef(0)->clone();
		//((OGRPolygon*)psWarpOptions->hCutline)->closeRings();

		

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
		OGRFree(pszSrcWKT);
		inputRF.close();
		//GDALClose( poSrcDS );
	}

	
	oBuffer.createBuffer(bands,bufWidth,bufHeight,NULL,eDT,pNoDataValue);
	int noDataValueFromFile = 0;
	if	(pNoDataValue) oBuffer.initByNoDataValue(pNoDataValue[0]);
	else if (pDefaultColor) oBuffer.initByRGBColor(pDefaultColor); 
	else if (bNoDataValueFromFile) oBuffer.initByNoDataValue(nNoDataValueFromFile);

	 
	poVrtDS->RasterIO(	GF_Read,0,0,bufWidth,bufHeight,oBuffer.getDataRef(),
						bufWidth,bufHeight,oBuffer.getDataType(),
						oBuffer.getBandsCount(),NULL,0,0,0);

	OGRFree(pszDstWKT);
	GDALClose(poVrtDS);
	VSIUnlink("/vsimem/tiffinmem");
	return TRUE;
}

/*

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

	GDALDataset*	poSrcDS = inputRF.getGDALDatasetRef();
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
*/