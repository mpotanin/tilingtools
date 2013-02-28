#include "stdafx.h"
#include "TilingFuncs.h"

using namespace GMX;

const int GMX_MAX_BUFFER_WIDTH	= 32;
const int GMX_MAX_TILES_IN_CACHE = 0xFFFF;


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

BOOL GMXMakeTiling		(GMXilingParameters		*poParams)
{


	LONG t_ = time(0);
	srand(t_%10000);
	BundleOfRasterFiles		oBundle;
	int TILE_SIZE = MercatorTileGrid::TILE_SIZE;
	//poParams->nJpegQuality = JPEG_BASE_ZOOM_QUALITY;
	
	if (!oBundle.init(poParams->inputPath,poParams->mercType,poParams->vectorFile,poParams->dShiftX,poParams->dShiftY))
	{
		cout<<"Error: read input data by path: "<<poParams->inputPath<<endl;
		return FALSE;
	}

	int baseZoom = (poParams->baseZoom == 0) ? oBundle.calculateBestMercZoom() : poParams->baseZoom;
	if (baseZoom<=0)
	{
		cout<<"Error: can't calculate base zoom for tiling"<<endl;
		return FALSE;
	}
	
	cout<<"Base zoom "<<baseZoom<<": calculating number of tiles: ";
	int expectedTiles	= oBundle.calculateNumberOfTiles(baseZoom);	
	cout<<expectedTiles<<endl;

	if (expectedTiles == 0) return FALSE;

	cout<<"0% ";
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
		
	
	
	GMXMakeBaseZoomTiling(poParams,&oBundle,expectedTiles,poITilePyramid);
	cout<<" done."<<endl;

	int minZoom = (poParams->minZoom <=0) ? 1 : poParams->minZoom;
	if (minZoom == baseZoom) return TRUE;

	cout<<"Pyramid tiles: ";//<<endl;
	cout<<"calculating number of tiles: ";
	VectorBorder	bundleEnvelope(oBundle.getMercatorEnvelope(),poParams->mercType);
	int generatedTiles = 0;
	expectedTiles = 0;
	GMXCreatePyramidalTiles(	bundleEnvelope,
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
		GMXCreatePyramidalTiles(	bundleEnvelope,
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


BOOL GMXTilingFromBuffer (GMXilingParameters				*poParams, 
						   RasterBuffer					&oBuffer, 
						   BundleOfRasterFiles			*poBundle, 
						   int							nX, 
						   int							nY,
						   int							z,
						   int							nTilesExpected, 
						   int							&nTilesGenerated,
						   ITilePyramid					*ITilePyramid)
{  
	int TILE_SIZE = MercatorTileGrid::TILE_SIZE;

	for (int x = nX; x < nX+oBuffer.getXSize()/TILE_SIZE; x += 1)
	{
		for (int y = nY; y < nY+oBuffer.getYSize()/TILE_SIZE; y += 1)
		{	
			OGREnvelope oTileEnvelope = MercatorTileGrid::calcEnvelopeByTile(z,x,y);
			if (!poBundle->intersects(oTileEnvelope)) continue;
			

			RasterBuffer oTileBuffer;

			void *tileData = oBuffer.getDataBlock((x-nX)*TILE_SIZE, (y-nY)*TILE_SIZE,TILE_SIZE,TILE_SIZE);
			oTileBuffer.createBuffer(	oBuffer.getBandsCount(),
											TILE_SIZE,
											TILE_SIZE,
											tileData,
											oBuffer.getDataType(),
											oBuffer.getNoDataValue(),
											oBuffer.isAlphaBand(),
											oBuffer.getColorTableRef());
			delete[]tileData;
			

			if (ITilePyramid != NULL)
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
				ITilePyramid->addTile(z,x,y,(BYTE*)pData,size);
				delete[]((BYTE*)pData);
				nTilesGenerated++;
			}
			
			GMXPrintTilingProgress(nTilesExpected,nTilesGenerated);
		}
	}
	
	return TRUE;
}


BOOL GMXMakeBaseZoomTiling	(	GMXilingParameters		*poParams, 
								BundleOfRasterFiles		*poBundle, 
								int						nExpected, 
								ITilePyramid				*ITilePyramid)
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


	int minx,maxx,miny,maxy;
	MercatorTileGrid::calcTileRange(poBundle->getMercatorEnvelope(),zoom,minx,miny,maxx,maxy);
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
			RasterBuffer mercBuffer;

			if (!poBundle->warpToMercBuffer(zoom,bufferEnvelope,mercBuffer,poParams->pNoDataValue,poParams->pBackgroundColor))
			{
				cout<<"Error: BaseZoomTiling: warping to merc fail"<<endl;
				return FALSE;
			}

			if ( poParams->pTransparentColor != NULL ) mercBuffer.createAlphaBandByColor(poParams->pTransparentColor);
			
			if ((poParams->tileType == JPEG_TILE || poParams->tileType == PNG_TILE)&&(mercBuffer.getDataType()!=GDT_Byte))
			{
				if (!mercBuffer.stretchDataTo8Bit(0,255))
				{
					cout<<"Error: can't stretch data to 8 bit"<<endl;
					return FALSE;
				}
			}

			//ToDo
			//Если длина очереди тайлинга < Lmax, то запустить новую порцию на тайлинг
			//(при этом требуется проконтролировать: многопоточность записи в ITilePyramid, безопасность mercBuffer).
			//Если длина очереди >=Lmax, то ждать 



			if (!GMXTilingFromBuffer(	poParams,
										mercBuffer,
										poBundle,
										curr_min_x,
										curr_min_y,
										zoom,
										nExpected,
										generatedTiles,
										ITilePyramid))
			{
				cout<<"Error: BaseZoomTiling: GMXTilingFromBuffer fail"<<endl;
				return FALSE;
			}
		}
	}

	return TRUE;
}


BOOL GMXCreatePyramidalTiles (VectorBorder		&oVectorBorder, 
						  int					nBaseZoom, 
						  int					nMinZoom, 
						  GMXilingParameters		*poParams, 
						  int					&nExpectedTiles, 
						  int					&nGeneratedTiles, 
						  BOOL					bOnlyCalculate, 
						  ITilePyramid			*ITilePyramid,
						  int					nJpegQuality
						  )
{

	RasterBuffer oBuffer;
	BOOL b;

	GMXCreateZoomOutTile(	oVectorBorder,0,0,0,nBaseZoom,nMinZoom,poParams,oBuffer,nExpectedTiles,
							nGeneratedTiles,bOnlyCalculate,ITilePyramid,nJpegQuality);

	if (oVectorBorder.getEnvelope().MaxX>-MercatorTileGrid::getULX())
	{
		GMXCreateZoomOutTile(	oVectorBorder,1,2,0,nBaseZoom,nMinZoom,poParams,oBuffer,nExpectedTiles,
								nGeneratedTiles,bOnlyCalculate,ITilePyramid,nJpegQuality);
		GMXCreateZoomOutTile(oVectorBorder,1,2,1,nBaseZoom,nMinZoom,poParams,oBuffer,nExpectedTiles,
								nGeneratedTiles,bOnlyCalculate,ITilePyramid,nJpegQuality);
	}

	return TRUE;
}

BOOL GMXCreateZoomOutTile (VectorBorder	&oVectorBorder, 
					  int				nCurrZoom, 
					  int				nX, 
					  int				nY, 
					  int				nBaseZoom, 
					  int				nMinZoom, 
					  GMXilingParameters	*poParams, 
					  RasterBuffer		&oTileBuffer,  
					  int				&nExpectedTiles, 
					  int				&nGeneratedTiles, 
					  BOOL				bOnlyCalculate, 
					  ITilePyramid		*ITilePyramid, 
					  int				nJpegQuality)
{

	int start;
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
						cout<<"Error: reading jpeg-data"<<endl;
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
					quarterTileExists[i*2+j] = GMXCreateZoomOutTile(oVectorBorder,
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

		if (!ZoomOutTileBuffer(quarterTileBuffer,quarterTileExists, oTileBuffer)) return FALSE;
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





BOOL ZoomOutTileBuffer (RasterBuffer srcQuarterTile[4], BOOL quarterTileExists[4], RasterBuffer &zoomOutTileBuffer) 
{
	int i;
	for (i = 0; i<4;i++)
		if (quarterTileExists[i]) break;
	if (i==4) return FALSE;
	
	int bands;
	GDALDataType gDT;
	
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

