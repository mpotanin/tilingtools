#include "stdafx.h"
#include "TilingFuncs.h"

using namespace GMX;

const int	GMX_MAX_BUFFER_WIDTH	= 32;
const int	GMX_MAX_TILES_IN_CACHE = 0xFFFF;
int			GMX_MAX_THREADS	= 2;
int			GMX_CURR_NUM_OF_THREADS = 1;


BOOL GMXPrintTilingProgress (int nExpectedTiles, int nGeneratedTiles)
{
	//cout<<nGeneratedTiles<<" "<<endl;
	if (nGeneratedTiles - (int)ceil((nExpectedTiles/10.0)*(nGeneratedTiles*10/nExpectedTiles))  ==0)
	{
		cout<<(nGeneratedTiles*10/nExpectedTiles)*10<<" ";
		fflush(stdout);
	}
	return TRUE;
}


BOOL GMXMakeTiling		(GMXTilingParameters		*poParams)
{
	LONG t_ = time(0);
	srand(t_%10000);
	BundleOfRasterFiles		oBundle;
	int TILE_SIZE = MercatorTileGrid::TILE_SIZE;
	//poParams->nJpegQuality = JPEG_BASE_ZOOM_QUALITY;
	
	if (!oBundle.init(poParams->inputPath,poParams->mercType,poParams->vectorFile,poParams->dShiftX,poParams->dShiftY))
	{
		cout<<"ERROR: read input data by path: "<<poParams->inputPath<<endl;
		return FALSE;
	}

	int baseZoom = (poParams->baseZoom == 0) ? oBundle.calculateBestMercZoom() : poParams->baseZoom;
	if (baseZoom<=0)
	{
		cout<<"ERROR: can't calculate base zoom for tiling"<<endl;
		return FALSE;
	}
	
	cout<<"Base zoom "<<baseZoom<<": calculating number of tiles: ";
	int expectedTiles	= oBundle.calculateNumberOfTiles(baseZoom);	
	cout<<expectedTiles<<endl;

	if (expectedTiles == 0) return FALSE;


	unsigned int maxTilesInCache = (poParams->maxTilesInCache == 0) ? GMX_MAX_TILES_IN_CACHE : poParams->maxTilesInCache; 
	unsigned int adjustedMaxTilesInCash =	(poParams->tileType == JPEG_TILE) ? maxTilesInCache	:
											(poParams->tileType == PNG_TILE) ? maxTilesInCache/3	: maxTilesInCache/20;
	BOOL	useBuffer = (expectedTiles*1.36 <= adjustedMaxTilesInCash) ? true : false;
	
	
	ITilePyramid	*poITilePyramid =	NULL;

	//ToDo
	if (poParams->useContainer)
	{
		if (GetExtension(poParams->containerFile) == "tiles")
				poITilePyramid = new GMXTileContainer(	poParams->containerFile,
														poParams->tileType,
														poParams->mercType,
														oBundle.getMercatorEnvelope(),
														baseZoom,
														useBuffer);
		else	poITilePyramid = new MBTileContainer(	poParams->containerFile,
														poParams->tileType,
														poParams->mercType,
														oBundle.getMercatorEnvelope());
	}
	else		poITilePyramid = new TileFolder(poParams->poTileName, useBuffer);
		
	
	
	GMXMakeZoomFromBundle(poParams,&oBundle,expectedTiles,poITilePyramid);
	cout<<" done."<<endl;

	int minZoom = (poParams->minZoom <=0) ? 1 : poParams->minZoom;
	if (minZoom == baseZoom) return TRUE;

	cout<<"Pyramid tiles: ";//<<endl;
	cout<<"calculating number of tiles: ";
	VectorBorder	bundleEnvelope(oBundle.getMercatorEnvelope(),poParams->mercType);
	int generatedTiles = 0;
	expectedTiles = 0;
	GMXMakePyramidFromBaseZoom(	bundleEnvelope,
								baseZoom,
								minZoom,
								poParams,
								expectedTiles,
								generatedTiles,
								TRUE,
								poITilePyramid,
								poParams->nJpegQuality);
	cout<<expectedTiles<<endl;
	if (expectedTiles > 0) 
	{
		cout<<"0% ";
		GMXMakePyramidFromBaseZoom(	bundleEnvelope,
									baseZoom,
									minZoom,
									poParams,
									expectedTiles,
									generatedTiles,
									FALSE,
									poITilePyramid,
									poParams->nJpegQuality);
		cout<<" done."<<endl;
	}

	poITilePyramid->close();
	delete(poITilePyramid);

	return TRUE;
}


BOOL GMXTilingFromBuffer (GMXTilingParameters			*poParams, 
						   RasterBuffer					*poBuffer, 
						   BundleOfRasterFiles			*poBundle, 
						   int							nX, 
						   int							nY,
						   int							z,
						   int							nTilesExpected, 
						   int							*pnTilesGenerated,
						   ITilePyramid					*poTilePyramid)
{  
	int TILE_SIZE = MercatorTileGrid::TILE_SIZE;

	for (int x = nX; x < nX+poBuffer->getXSize()/TILE_SIZE; x += 1)
	{
		for (int y = nY; y < nY+poBuffer->getYSize()/TILE_SIZE; y += 1)
		{	
			OGREnvelope oTileEnvelope = MercatorTileGrid::calcEnvelopeByTile(z,x,y);
			if (!poBundle->intersects(oTileEnvelope)) continue;
			
			
			RasterBuffer oTileBuffer;
			void *tileData = poBuffer->getDataBlock((x-nX)*TILE_SIZE, (y-nY)*TILE_SIZE,TILE_SIZE,TILE_SIZE);
			oTileBuffer.createBuffer(	poBuffer->getBandsCount(),
											TILE_SIZE,
											TILE_SIZE,
											tileData,
											poBuffer->getDataType(),
											poBuffer->getNoDataValue(),
											poBuffer->isAlphaBand(),
											poBuffer->getColorTableRef());
			delete[]tileData;
			

			if (poTilePyramid != NULL)
			{
				
				void *pData=NULL;
				int size = 0;
				switch (poParams->tileType)
				{
					case JPEG_TILE:
						{
							oTileBuffer.SaveToJpegData(85,pData,size);
							break;
						}
					case PNG_TILE:
						{
							oTileBuffer.SaveToPngData(pData,size);
							break;
						}
					default:
						oTileBuffer.SaveToTiffData(pData,size);
				}
				poTilePyramid->addTile(z,x,y,(BYTE*)pData,size);
				delete[]((BYTE*)pData);
				(*pnTilesGenerated)++;
			}
			
			GMXPrintTilingProgress(nTilesExpected,(*pnTilesGenerated));
		}
	}
	
	return TRUE;
}

BOOL GMXCleanAfterTilingFromBufer (GMX::RasterBuffer				*poBuffer)
{
	delete(poBuffer);
	return TRUE;
}

DWORD WINAPI GMXCallTilingFromBuffer( LPVOID lpParam )
{
	GMXTilingFromBufferParams	*poParams = (GMXTilingFromBufferParams*)lpParam;
	//ToDo
	//Обработка ошибки
	DWORD result = GMXTilingFromBuffer(poParams->poTilingParams,
						poParams->poBuffer,
						poParams->poBundle,
						poParams->nULx,
						poParams->nULy,
						poParams->z,
						poParams->nTilesExpected,
						poParams->pnTilesGenerated,
						poParams->poTilePyramid);
	poParams->pfCleanAfterTiling(poParams->poBuffer);
	GMX_CURR_NUM_OF_THREADS--;
	return result;
}

HANDLE GMXAsyncTilingFromBuffer (	GMXTilingParameters			*poTilingParams, 
								RasterBuffer				*poBuffer, 
								BundleOfRasterFiles			*poBundle, 
								int							nX, 
								int							nY,
								int							z,
								int							nTilesExpected, 
								int							*pnTilesGenerated,
								ITilePyramid				*poTilePyramid,
								unsigned long				&threadId)
{
	//запустить GMXTilingFromBuffer в отдельном потоке
	GMXTilingFromBufferParams	*poParams = new GMXTilingFromBufferParams;
	
	poParams->poTilingParams		= poTilingParams;
	poParams->poBuffer				= poBuffer;
	poParams->poBundle				= poBundle;
	poParams->nTilesExpected		= nTilesExpected;
	poParams->nULx					= nX;
	poParams->nULy					= nY;
	poParams->pnTilesGenerated		= pnTilesGenerated;
	poParams->poTilePyramid		= poTilePyramid;	
	poParams->z					= z;
	poParams->pfCleanAfterTiling	= GMXCleanAfterTilingFromBufer;


	//Увеличить количество потоков
	GMX_CURR_NUM_OF_THREADS++;
	return CreateThread(NULL,0,GMXCallTilingFromBuffer,poParams,0,&threadId);
	//ToDo
	//Если ошибка, уменьшить количество потоков
	//ждать окончание потока, потом закрыть handle 
	//Уменьшить количество потоков
	//msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
	//return TRUE;

}


BOOL GMXMakeZoomFromBundle	(	GMXTilingParameters		*poParams, 
								BundleOfRasterFiles		*poBundle, 
								int						nExpected, 
								ITilePyramid			*poTilePyramid)
{

	//получить экстент бандла
	//вычислить зум
	//пройти в цикле по экстенту:
		//формируем под-экстент
		//проверяем пересение с вектором
		//перепроицируем в под-экстент
		//запускаем тайлинг из буфера
	int			generatedTiles = 0;
	//OGREnvelope bundleMercEnvelope = poBundle->getMercatorEnvelope();
	int			zoom = (poParams->baseZoom == 0) ? poBundle->calculateBestMercZoom() : poParams->baseZoom;
	double		res = MercatorTileGrid::calcResolutionByZoom(zoom);

	bool		bStretchTo8Bit = false;
	double		*stretchMinValues = NULL, *stretchMaxValues = NULL;
	if (poParams->bAutoStretchTo8Bit)
	{
		RasterFile oRF((*poBundle->getFileList().begin()),TRUE);
		double *min,*max,*mean,*stdDev;
		int bands;
		if ( (poParams->tileType == JPEG_TILE || poParams->tileType == PNG_TILE) && 
			 (oRF.getGDALDatasetRef()->GetRasterBand(1)->GetRasterDataType() != GDT_Byte))
		{
			bStretchTo8Bit = true;
			cout<<"WARNING: input raster doesn't match 8 bit/band. Auto stretching to 8 bit will be performed"<<endl;

			if (!oRF.computeStatistics(bands,min,max,mean,stdDev))
			{
				cout<<"ERROR: compute statistics failed for "<<(*poBundle->getFileList().begin())<<endl;
				return false;
			}
			stretchMinValues = new double[bands];
			stretchMaxValues = new double[bands];
			for (int i=0;i<bands;i++)
			{
				stretchMinValues[i] = mean[i] - 2*stdDev[i];
				stretchMaxValues[i] = mean[i] + 2*stdDev[i];
			}
			delete[]min;delete[]max;delete[]mean;delete[]stdDev;
		}
	}

	cout<<"0% ";
	fflush(stdout);

	//if (poBundle->getFileList().size()>1) GMX_MAX_THREADS = 1;

	int minx,maxx,miny,maxy;
	MercatorTileGrid::calcTileRange(poBundle->getMercatorEnvelope(),zoom,minx,miny,maxx,maxy);

	HANDLE			threadHandle = NULL;
	unsigned long	threadId;
	for (int curr_min_x = minx; curr_min_x<=maxx; curr_min_x+=GMX_MAX_BUFFER_WIDTH)
	{
		int curr_max_x =	(curr_min_x + GMX_MAX_BUFFER_WIDTH - 1 > maxx) ? 
							maxx :  
							curr_min_x + GMX_MAX_BUFFER_WIDTH - 1;
		
		for (int curr_min_y = miny; curr_min_y<=maxy; curr_min_y+=GMX_MAX_BUFFER_WIDTH)
		{
			int curr_max_y =	(curr_min_y + GMX_MAX_BUFFER_WIDTH - 1 > maxy) ? 
								maxy :  
								curr_min_y + GMX_MAX_BUFFER_WIDTH - 1;
			
			OGREnvelope bufferEnvelope = MercatorTileGrid::calcEnvelopeByTileRange(	zoom,
																					curr_min_x,
																					curr_min_y,
																					curr_max_x,
																					curr_max_y);
			if (!poBundle->intersects(bufferEnvelope)) continue;
			RasterBuffer *poMercBuffer = new RasterBuffer();

			if (!poBundle->warpToMercBuffer(zoom,bufferEnvelope,poMercBuffer,poParams->pNoDataValue,poParams->pBackgroundColor))
			{
				cout<<"ERROR: BaseZoomTiling: warping to merc fail"<<endl;
				return FALSE;
			}
		
			if ( poParams->pTransparentColor != NULL ) poMercBuffer->createAlphaBandByColor(poParams->pTransparentColor);
			
			if (bStretchTo8Bit)
			{
				if (!poMercBuffer->stretchDataTo8Bit(stretchMinValues,stretchMaxValues))
				{
					cout<<"ERROR: can't stretch raster values to 8 bit"<<endl;
					return FALSE;
				}
				
			}
			
			//ToDo
			//(при этом требуется проконтролировать: многопоточность записи в ITilePyramid, безопасность mercBuffer).
			if (GMX_MAX_THREADS == 1)
			{
				if (!GMXTilingFromBuffer(poParams,
										poMercBuffer,
										poBundle,
										curr_min_x,
										curr_min_y,
										zoom,
										nExpected,
										&generatedTiles,
										poTilePyramid))
				{
					cout<<"ERROR: BaseZoomTiling: GMXTilingFromBuffer fail"<<endl;
					return FALSE;
				}
				delete(poMercBuffer);
			}
			else 
			{
				while (GMX_CURR_NUM_OF_THREADS==GMX_MAX_THREADS)
					Sleep(250);
				if (threadHandle!=NULL) 
				{
					CloseHandle(threadHandle);
					threadHandle = NULL;
				}
				if (!(threadHandle = GMXAsyncTilingFromBuffer(	poParams,
																poMercBuffer,
																poBundle,
																curr_min_x,
																curr_min_y,
																zoom,
																nExpected,
																&generatedTiles,
																poTilePyramid,
																threadId)))
				{
					cout<<"ERROR: BaseZoomTiling: GMXTilingFromBuffer fail"<<endl;
					return FALSE;
				}
			}
		}
	}

	while (GMX_CURR_NUM_OF_THREADS>1)
		Sleep(250);
	if (threadHandle!=NULL) 
	{
		CloseHandle(threadHandle);
		threadHandle = NULL;
	}
	return TRUE;
}


BOOL GMXMakePyramidFromBaseZoom (	VectorBorder		&oVectorBorder, 
								int					nBaseZoom, 
								int					nMinZoom, 
								GMXTilingParameters		*poParams, 
								int					&nExpectedTiles, 
								int					&nGeneratedTiles, 
								BOOL					bOnlyCalculate, 
								ITilePyramid			*ITilePyramid,
								int					nJpegQuality
								)
{

	RasterBuffer oBuffer;
	BOOL b;

	GMXMakeZoomOutTile(	oVectorBorder,0,0,0,nBaseZoom,nMinZoom,poParams,oBuffer,nExpectedTiles,
							nGeneratedTiles,bOnlyCalculate,ITilePyramid,nJpegQuality);

	if (oVectorBorder.getEnvelope().MaxX>-MercatorTileGrid::getULX())
	{
		GMXMakeZoomOutTile(	oVectorBorder,1,2,0,nBaseZoom,nMinZoom,poParams,oBuffer,nExpectedTiles,
								nGeneratedTiles,bOnlyCalculate,ITilePyramid,nJpegQuality);
		GMXMakeZoomOutTile(oVectorBorder,1,2,1,nBaseZoom,nMinZoom,poParams,oBuffer,nExpectedTiles,
								nGeneratedTiles,bOnlyCalculate,ITilePyramid,nJpegQuality);
	}

	return TRUE;
}

BOOL GMXMakeZoomOutTile (VectorBorder	&oVectorBorder, 
					  int				nCurrZoom, 
					  int				nX, 
					  int				nY, 
					  int				nBaseZoom, 
					  int				nMinZoom, 
					  GMXTilingParameters	*poParams, 
					  RasterBuffer		&oTileBuffer,  
					  int				&nExpectedTiles, 
					  int				&nGeneratedTiles, 
					  BOOL				bOnlyCalculate, 
					  ITilePyramid		*ITilePyramid, 
					  int				nJpegQuality)
{

	if (nCurrZoom==nBaseZoom)
	{	
		if (!ITilePyramid->tileExists(nCurrZoom,nX,nY)) return FALSE;
		if (bOnlyCalculate) return TRUE;

		unsigned int size	= 0;
		BYTE		*pData	= NULL;
	
		ITilePyramid->getTile(nCurrZoom,nX,nY,pData,size);
		if (size ==0) return FALSE;
		switch (poParams->tileType)
		{
			case JPEG_TILE:
				{
					if(!oTileBuffer.createBufferFromJpegData(pData,size))
					{
						cout<<"ERROR: reading jpeg-data"<<endl;
						return FALSE;
					}
					break;
				}
			case PNG_TILE:
				{
					oTileBuffer.createBufferFromPngData(pData,size);
					break;
				}
			default:
				oTileBuffer.createBufferFromTiffData(pData,size);
		}
		delete[]((BYTE*)pData);
	
		return TRUE;
	}
	else
	{
		RasterBuffer	quarterTileBuffer[4];
		BOOL			quarterTileExists[4];

		for (int i=0;i<2;i++)
		{
			for (int j=0;j<2;j++)
			{
				if (oVectorBorder.intersects(nCurrZoom+1,2*nX+j,2*nY+i)) 
					quarterTileExists[i*2+j] = GMXMakeZoomOutTile(oVectorBorder,
															nCurrZoom+1,
															2*nX+j,
															2*nY+i,
															nBaseZoom,
															nMinZoom,
															poParams,quarterTileBuffer[i*2+j],
															nExpectedTiles,
															nGeneratedTiles,
															bOnlyCalculate,
															ITilePyramid,
															nJpegQuality);
				else quarterTileExists[i*2+j] = FALSE;
			}
		}
		
		if (nCurrZoom < nMinZoom)		return FALSE;
		if ((!quarterTileExists[0])&&
			(!quarterTileExists[1])&&
			(!quarterTileExists[2])&&
			(!quarterTileExists[3]) )	return FALSE;
	
		if (bOnlyCalculate)
		{
			nExpectedTiles++;
			return TRUE;
		}

		//ToDo
		//Проверить длину очереди на выполнение операций: ZoomOut + SaveData + запись результатов
		//Если < Lmax, то отправить на выполнение
		//Если >=Lmax, то ждать

		if (!GMXZoomOutTileBuffer(quarterTileBuffer,quarterTileExists, oTileBuffer)) return FALSE;
		void *pData=NULL;
		int size = 0;
		switch (poParams->tileType)
		{
			case JPEG_TILE:
				{
					oTileBuffer.SaveToJpegData(85,pData,size);
					break;
				}
			case PNG_TILE:
				{
					oTileBuffer.SaveToPng24Data(pData,size);
					break;
				}
			default:
				oTileBuffer.SaveToTiffData(pData,size);
		}


		ITilePyramid->addTile(nCurrZoom,nX,nY,(BYTE*)pData,size);
		delete[]((BYTE*)pData);
		nGeneratedTiles++;
		GMXPrintTilingProgress(nExpectedTiles,nGeneratedTiles);
	}

	return TRUE;
}





BOOL GMXZoomOutTileBuffer (RasterBuffer srcQuarterTile[4], BOOL quarterTileExists[4], RasterBuffer &zoomOutTileBuffer) 
{
	int i;
	for (i = 0; i<4;i++)
		if (quarterTileExists[i]) break;
	if (i==4) return FALSE;
	
	int TILE_SIZE = MercatorTileGrid::TILE_SIZE;
	zoomOutTileBuffer.createBuffer(srcQuarterTile[i].getBandsCount(),
									TILE_SIZE,
									TILE_SIZE,
									NULL,
									srcQuarterTile[i].getDataType(),
									srcQuarterTile[i].getNoDataValue(),
									srcQuarterTile[i].isAlphaBand());
	for (int n=0;n<4;n++)
	{
		if (quarterTileExists[n])
		{
			void *pZoomedData = srcQuarterTile[n].getDataZoomedOut();
			zoomOutTileBuffer.setDataBlock(	(n%2)*TILE_SIZE/2,
									(n/2)*TILE_SIZE/2,
									TILE_SIZE/2,
									TILE_SIZE/2,
									pZoomedData);
			delete[]pZoomedData;
		}
	}

	return TRUE;
}

