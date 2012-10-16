#include "stdafx.h"
#include "TilingFuncs.h"

//const int MAX_BUFFER_TILES_WIDTH2	= 16;
const int MAX_BUFFER_TILES_WIDTH	= 32;
//const int MAX_TILES_IN_CACHE = 100;
const int MAX_TILES_IN_CACHE = 0xFFFF;
//const int MAX_TILES_IN_CACHE = 0xF;

//const int MAX_TILES_IN_CACHE = 1000;

//int JPEG_BASE_ZOOM_QUALITY = 80;

//_TCHAR buf[256];

BOOL PrintTilingProgress (int nExpectedTiles, int nGeneratedTiles)
{
	//wcout<<nGeneratedTiles<<L" "<<endl;
	if (nGeneratedTiles - (int)ceil((nExpectedTiles/10.0)*(nGeneratedTiles*10/nExpectedTiles))  ==0)
	{
		wcout<<(nGeneratedTiles*10/nExpectedTiles)*10<<L" ";
		fflush(stdout);
	}
	return TRUE;
}

BOOL MakeTiling		(TilingParameters		&oParams)
{


	LONG t_ = time(0);
	srand(t_%10000);
	BundleOfRasterFiles		oBundle;
	TILE_SIZE = MercatorTileGrid::TILE_SIZE;
	//oParams.nJpegQuality = JPEG_BASE_ZOOM_QUALITY;
	
	if (!oBundle.InitFromRasterFile(oParams.inputFile,oParams.mercType,oParams.vectorFile,oParams.dShiftX,oParams.dShiftY))
	{
		wcout<<L"Error: can't open input file: "<<oParams.inputFile<<endl;
		return FALSE;
	}

	int baseZoom = (oParams.baseZoom == 0) ? oBundle.calculateBestMercZoom() : oParams.baseZoom;
	if (baseZoom<=0)
	{
		wcout<<L"Error: can't calculate base zoom for tiling"<<oParams.inputFile<<endl;
		return FALSE;
	}
	
	wcout<<L"Base zoom: calculating number of tiles: ";
	int expectedTiles	= oBundle.calculateNumberOfTiles(baseZoom);	
	wcout<<expectedTiles<<endl;

	wcout<<L"0% ";
	unsigned int maxTilesInCache = (oParams.maxTilesInCache == 0) ? MAX_TILES_IN_CACHE : oParams.maxTilesInCache; 
	unsigned int adjustedMaxTilesInCash =	(oParams.tileType == JPEG_TILE) ? maxTilesInCache	:
											(oParams.tileType == PNG_TILE) ? maxTilesInCache/3	: maxTilesInCache/20;
	BOOL	useBuffer = (expectedTiles*1.36 <= adjustedMaxTilesInCash) ? true : false;
	
	
	TilePyramid	*poTilePyramid =	NULL;
	if (oParams.useContainer)	poTilePyramid = new TileContainer(oBundle.getMercatorEnvelope(),baseZoom,useBuffer,oParams.containerFile,oParams.tileType,oParams.mercType);
	else						poTilePyramid = new TileFolder(oParams.poTileName, useBuffer);
		
	
	
	BaseZoomTiling2(oParams,&oBundle,expectedTiles,poTilePyramid);
	wcout<<L" done."<<endl;

	int minZoom = (oParams.minZoom <=0) ? 1 : oParams.minZoom;
	if (minZoom == baseZoom) return TRUE;

	wcout<<L"Pyramid tiles: ";//<<endl;
	wcout<<L"calculating number of tiles: ";
	VectorBorder	oVectorBorder;
	oVectorBorder.InitByEnvelope(oBundle.getMercatorEnvelope());
	int generatedTiles = 0;
	expectedTiles = 0;
	CreatePyramidalTiles(oVectorBorder,baseZoom,1,oParams,expectedTiles,generatedTiles,TRUE,poTilePyramid,oParams.nJpegQuality);
	wcout<<expectedTiles<<endl;
	if (expectedTiles > 0) 
	{
		wcout<<L"0% ";
		CreatePyramidalTiles(oVectorBorder,baseZoom,1,oParams,expectedTiles,generatedTiles,FALSE,poTilePyramid,oParams.nJpegQuality);
		wcout<<L" done."<<endl;
	}

	poTilePyramid->close();
	delete(poTilePyramid);

	return TRUE;
}


BOOL TilingFromBuffer (TilingParameters				&oParams, 
					   RasterBuffer					&oBuffer, 
					   BundleOfRasterFiles			*poBundle, 
					   int							nX, 
					   int							nY,
					   int							z,
					   int							nTilesExpected, 
					   int							&nTilesGenerated,
					   TilePyramid				*tilePyramid)
{  
	for (int x = nX; x < nX+oBuffer.getXSize()/TILE_SIZE; x += 1)
	{
		for (int y = nY; y < nY+oBuffer.getYSize()/TILE_SIZE; y += 1)
		{	
			OGREnvelope oTileEnvelope = MercatorTileGrid::calcEnvelopeByTile(z,x,y);
			if (!poBundle->intersects(oTileEnvelope)) continue;
			

			RasterBuffer oTileBuffer;
			
			void *tileData = oBuffer.copyData((x-nX)*TILE_SIZE, (y-nY)*TILE_SIZE,TILE_SIZE,TILE_SIZE);
			oTileBuffer.createBuffer(	oBuffer.getBandsCount(),
											TILE_SIZE,
											TILE_SIZE,
											tileData,
											oBuffer.getDataType(),
											oBuffer.getNoDataValue(),
											oBuffer.isAlphaBand());
			delete[]tileData;
			

			if (tilePyramid != NULL)
			{
				
				void *pData=NULL;
				int size = 0;
				switch (oParams.tileType)
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
				tilePyramid->addTile(z,x,y,(BYTE*)pData,size);
				delete[]((BYTE*)pData);
				nTilesGenerated++;
			}
			
			PrintTilingProgress(nTilesExpected,nTilesGenerated);
		}
	}
	
	return TRUE;
}


BOOL BaseZoomTiling2	(TilingParameters		&oParams, 
						BundleOfRasterFiles		*poBundle, 
						int						nExpected, 
						TilePyramid			*tilePyramid)
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
	int			zoom = (oParams.baseZoom == 0) ? poBundle->calculateBestMercZoom() : oParams.baseZoom;
	double		res = MercatorTileGrid::calcResolutionByZoom(zoom);


	int minx,maxx,miny,maxy;
	MercatorTileGrid::calcTileRange(poBundle->getMercatorEnvelope(),zoom,minx,miny,maxx,maxy);
	for (int curr_min_x = minx; curr_min_x<=maxx; curr_min_x+=MAX_BUFFER_TILES_WIDTH)
	{
		int curr_max_x =	(curr_min_x + MAX_BUFFER_TILES_WIDTH - 1 > maxx) ? 
							maxx :  
							curr_min_x + MAX_BUFFER_TILES_WIDTH - 1;
		for (int curr_min_y = miny; curr_min_y<=maxy; curr_min_y+=MAX_BUFFER_TILES_WIDTH)
		{
			int curr_max_y =	(curr_min_y + MAX_BUFFER_TILES_WIDTH - 1 > maxy) ? 
								maxy :  
								curr_min_y + MAX_BUFFER_TILES_WIDTH - 1;
			
			OGREnvelope bufferEnvelope = MercatorTileGrid::calcEnvelopeByTileRange(zoom,curr_min_x,curr_min_y,curr_max_x,curr_max_y);//?
			if (!poBundle->intersects(bufferEnvelope)) continue;
			RasterBuffer mercBuffer;
			if (!poBundle->warpMercToBuffer(zoom,bufferEnvelope,mercBuffer,oParams.pNoDataValue,oParams.pBackgroundColor))
			{
				wcout<<L"Error: BaseZoomTiling: warping to merc fail"<<endl;
				return FALSE;
			}

			if ( oParams.pTransparentColor != NULL ) mercBuffer.createAlphaBandByColor(oParams.pTransparentColor);
			
			if ((oParams.tileType == JPEG_TILE || oParams.tileType == PNG_TILE)&&(mercBuffer.getDataType()!=GDT_Byte))
			{
				if (!mercBuffer.stretchDataTo8Bit(0,255))
				{
					wcout<<L"Error: can't stretch data to 8 bit"<<endl;
					return FALSE;
				}
			}
			if (!TilingFromBuffer(	oParams,
									mercBuffer,
									poBundle,
									curr_min_x,
									curr_min_y,
									zoom,
									nExpected,
									generatedTiles,
									tilePyramid))
			{
				wcout<<L"Error: BaseZoomTiling: TilingFromBuffer fail"<<endl;
				return FALSE;
			}
		}
	}

	return TRUE;
}


BOOL CreatePyramidalTiles (VectorBorder		&oVectorBorder, 
						  int					nBaseZoom, 
						  int					nMinZoom, 
						  TilingParameters		&oParams, 
						  int					&nExpectedTiles, 
						  int					&nGeneratedTiles, 
						  BOOL					bOnlyCalculate, 
						  TilePyramid			*tilePyramid,
						  int					nJpegQuality
						  )
{

	RasterBuffer oBuffer;
	BOOL b;

	CreateZoomOutTile(oVectorBorder,0,0,0,nBaseZoom,nMinZoom,oParams,oBuffer,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilePyramid,nJpegQuality);
	if (oVectorBorder.GetEnvelope().MaxX>-MercatorTileGrid::getULX())
	{
		CreateZoomOutTile(oVectorBorder,1,2,0,nBaseZoom,nMinZoom,oParams,oBuffer,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilePyramid,nJpegQuality);
		CreateZoomOutTile(oVectorBorder,1,2,1,nBaseZoom,nMinZoom,oParams,oBuffer,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilePyramid,nJpegQuality);
	}

	return TRUE;
}

BOOL CreateZoomOutTile (VectorBorder	&oVectorBorder, 
					  int				nCurrZoom, 
					  int				nX, 
					  int				nY, 
					  int				nBaseZoom, 
					  int				nMinZoom, 
					  TilingParameters	&oParams, 
					  RasterBuffer		&oTileBuffer,  
					  int				&nExpectedTiles, 
					  int				&nGeneratedTiles, 
					  BOOL				bOnlyCalculate, 
					  TilePyramid		*tilePyramid, 
					  int				nJpegQuality)
{

	int start;
	if (nCurrZoom==nBaseZoom)
	{	
		if (!tilePyramid->tileExists(nCurrZoom,nX,nY)) return FALSE;
		if (bOnlyCalculate) return TRUE;

		unsigned int size	= 0;
		BYTE		*pData	= NULL;
		tilePyramid->getTile(nCurrZoom,nX,nY,pData,size);
		if (size ==0) return FALSE;
		switch (oParams.tileType)
		{
			case JPEG_TILE:
				{
					if(!oTileBuffer.createBufferFromJpegData(pData,size))
					{
						wcout<<L"Error: reading jpeg-data from cache"<<endl;
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
				OGREnvelope oEnvelope = MercatorTileGrid::calcEnvelopeByTile(nCurrZoom+1,2*nX+j,2*nY+i);
				if (oVectorBorder.Intersects(oEnvelope)) 
					quarterTileExists[i*2+j] = CreateZoomOutTile(oVectorBorder,
															nCurrZoom+1,
															2*nX+j,
															2*nY+i,
															nBaseZoom,
															nMinZoom,
															oParams,quarterTileBuffer[i*2+j],
															nExpectedTiles,
															nGeneratedTiles,
															bOnlyCalculate,
															tilePyramid,
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

		if (!ZoomOutTileBuffer(quarterTileBuffer,quarterTileExists, oTileBuffer)) return FALSE;
		
	


		void *pData=NULL;
		int size = 0;
		switch (oParams.tileType)
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
		tilePyramid->addTile(nCurrZoom,nX,nY,(BYTE*)pData,size);
		delete[]((BYTE*)pData);
		nGeneratedTiles++;
		PrintTilingProgress(nExpectedTiles,nGeneratedTiles);
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
			zoomOutTileBuffer.setData(	(n%2)*TILE_SIZE/2,
									(n/2)*TILE_SIZE/2,
									TILE_SIZE/2,
									TILE_SIZE/2,
									pZoomedData);
			delete[]pZoomedData;
		}
	}

	return TRUE;
}

