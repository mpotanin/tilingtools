#include "stdafx.h"
#include "TilingFuncs.h"


//const int MAX_BUFFER_TILES_WIDTH2	= 16;
const int MAX_BUFFER_TILES_WIDTH	= 32;
//const int MAX_TILES_IN_CACHE = 100;
const int MAX_TILES_IN_CACHE = 0xFFFF;

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
	
	wcout<<L"Base zoom: calculating number of tiles ... ";
	int expectedTiles	= oBundle.calculateNumberOfTiles(baseZoom);	
	wcout<<expectedTiles<<endl;

	wcout<<L"making base zoom tiles: 0% ";
	unsigned int adjustedMaxTilesInCash =	(oParams.tileType == JPEG_TILE) ? MAX_TILES_IN_CACHE	:
											(oParams.tileType == PNG_TILE) ? MAX_TILES_IN_CACHE/3	: MAX_TILES_IN_CACHE/20;
	BOOL	useBuffer = (expectedTiles*1.36 <= adjustedMaxTilesInCash) ? true : false;
	TilesContainer	tilesContainer(	oBundle.getMercatorEnvelope(),baseZoom,useBuffer,oParams.useContainer,
									oParams.poTileName,oParams.containerFile,oParams.tileType,oParams.mercType);
	BaseZoomTiling2(oParams,&oBundle,expectedTiles,&tilesContainer);
	wcout<<L" done."<<endl;

	int minZoom = (oParams.minZoom <=0) ? 1 : oParams.minZoom;
	if (minZoom == baseZoom) return TRUE;

	wcout<<L"Pyramid tiles: ";//<<endl;
	wcout<<L"calculating number of tiles ... ";
	VectorBorder	oVectorBorder;
	oVectorBorder.InitByEnvelope(oBundle.getMercatorEnvelope());
	int generatedTiles = 0;
	expectedTiles = 0;
	CreaterPyramidalTiles(oVectorBorder,baseZoom,1,oParams,expectedTiles,generatedTiles,TRUE,&tilesContainer,oParams.nJpegQuality);
	wcout<<expectedTiles<<endl;
	if (expectedTiles == 0) return TRUE;

	wcout<<L"making pyramid tiles: 0% ";
	CreaterPyramidalTiles(oVectorBorder,baseZoom,1,oParams,expectedTiles,generatedTiles,FALSE,&tilesContainer,oParams.nJpegQuality);
	wcout<<L" done."<<endl;
	if (oParams.useContainer) tilesContainer.closeContainerFile();

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
					   TilesContainer				*tilesContainer)
					   //vector<pair<wstring,pair<void*,int>>> *tilesCash)
{  
	for (int x = nX; x < nX+oBuffer.getBufferXSize()/TILE_SIZE; x += 1)
	{
		for (int y = nY; y < nY+oBuffer.getBufferYSize()/TILE_SIZE; y += 1)
		{	
			OGREnvelope oTileEnvelope = MercatorTileGrid::calcEnvelopeByTile(z,x,y);
			if (!poBundle->intersects(oTileEnvelope)) continue;
			

			RasterBuffer oTileBuffer;
			if (oParams.backgroundColor)
				oTileBuffer.setBackgroundColor(oParams.backgroundColor);
			BYTE *pData = (BYTE*)oBuffer.getBlockFromBuffer(
				(x-nX)*TILE_SIZE,
				(y-nY)*TILE_SIZE,
				TILE_SIZE,
				TILE_SIZE);
			oTileBuffer.createBuffer(oBuffer.getBandsCount(), TILE_SIZE, TILE_SIZE, pData,oBuffer.getBufferDataType());
			delete [] pData;
		
			if (tilesContainer != NULL)
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
				tilesContainer->addTile(x,y,z,(BYTE*)pData,size);
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
						TilesContainer			*tilesContainer)
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
		int curr_max_x = (curr_min_x + MAX_BUFFER_TILES_WIDTH - 1 > maxx) ? maxx :  curr_min_x + MAX_BUFFER_TILES_WIDTH - 1;
		for (int curr_min_y = miny; curr_min_y<=maxy; curr_min_y+=MAX_BUFFER_TILES_WIDTH)
		{
			int curr_max_y = (curr_min_y + MAX_BUFFER_TILES_WIDTH - 1 > maxy) ? maxy :  curr_min_y + MAX_BUFFER_TILES_WIDTH - 1;
			OGREnvelope bufferEnvelope = MercatorTileGrid::calcEnvelopeByTileRange(zoom,curr_min_x,curr_min_y,curr_max_x,curr_max_y);//?
			if (!poBundle->intersects(bufferEnvelope)) continue;

			RasterBuffer mercBuffer;
			
			if (!poBundle->warpMercToBuffer(zoom,bufferEnvelope,mercBuffer))
			{
				//
			}
			
			if ((oParams.tileType == JPEG_TILE || oParams.tileType == PNG_TILE)&&(mercBuffer.getBufferDataType()!=GDT_Byte))
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
									tilesContainer))
			{
				//
			}
			//TilingFromBuffer
		}
	}





	//MercatorTileGrid::
	

	return TRUE;
}


BOOL CreaterPyramidalTiles (VectorBorder		&oVectorBorder, 
					  int					nBaseZoom, 
					  int					nMinZoom, 
					  TilingParameters		&oParams, 
					  int					&nExpectedTiles, 
					  int					&nGeneratedTiles, 
					  BOOL					bOnlyCalculate, 
					  TilesContainer		*tilesContainer,
					  //vector<pair<wstring,pair<void*,int>>> *tilesCash,
					  int					nJpegQuality
					  )
{

	RasterBuffer oBuffer;
	BOOL b;

	MakeZoomOutTile(oVectorBorder,0,0,0,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesContainer,nJpegQuality);
	//MakeZoomOutTile(oVectorBorder,0,0,0,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesCash,nJpegQuality);	
	
	//MakeZoomOutTile(oVectorBorder,1,-1,0,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesCash,nJpegQuality);
	//MakeZoomOutTile(oVectorBorder,1,-1,-1,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesCash,nJpegQuality);
	//MakeZoomOutTile(oVectorBorder,1,0,-1,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesCash,nJpegQuality);
	if (oVectorBorder.GetEnvelope().MaxX>-MercatorTileGrid::getULX())
	{
		MakeZoomOutTile(oVectorBorder,1,2,0,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesContainer,nJpegQuality);
		MakeZoomOutTile(oVectorBorder,1,2,1,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesContainer,nJpegQuality);
		
		//MakeZoomOutTile(oVectorBorder,1,2,0,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesCash,nJpegQuality);
		//MakeZoomOutTile(oVectorBorder,1,2,1,nBaseZoom,nMinZoom,oParams,oBuffer,b,nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesCash,nJpegQuality);
	}

	return TRUE;
}
//*/




BOOL MakeZoomOutTile (VectorBorder		&oVectorBorder, 
					  int				nCurrZoom, 
					  int				nX, 
					  int				nY, 
					  int				nBaseZoom, 
					  int				nMinZoom, 
					  TilingParameters	&oParams, 
					  RasterBuffer		&oBuffer,  
					  BOOL				&bBlackTile, 
					  int				&nExpectedTiles, 
					  int				&nGeneratedTiles, 
					  BOOL				bOnlyCalculate, 
					  TilesContainer	*tilesContainer, 
					  int				nJpegQuality)
{

	int start;

	if (nCurrZoom==nBaseZoom)
	{	
		if (bOnlyCalculate) 
		{
			bBlackTile = (!tilesContainer->tileExists(nX,nY,nCurrZoom));
			return TRUE;
		}
	
		unsigned int size	= 0;
		BYTE		*pData	= NULL;
		tilesContainer->getTile(nX,nY,nCurrZoom,pData,size);
		bBlackTile = (size == 0) ? TRUE : FALSE;
		if (!bBlackTile) 
		{
			switch (oParams.tileType)
			{
				case JPEG_TILE:
					{
						oBuffer.createBufferFromJpegData(pData,size);;
						break;
					}
				case PNG_TILE:
					{
						oBuffer.createBufferFromPngData(pData,size);
						break;
					}
				default:
					oBuffer.createBufferFromTiffData(pData,size);
			}
			delete[]((BYTE*)pData);
		}
		return TRUE;
	}
	else
	{
		RasterBuffer oBuffers[4];
		BOOL bBlackTiles[4];
		
		for (int i=0;i<2;i++)
		{
			for (int j=0;j<2;j++)
			{
				OGREnvelope oEnvelope = MercatorTileGrid::calcEnvelopeByTile(nCurrZoom+1,2*nX+j,2*nY+i);
				if (oVectorBorder.Intersects(oEnvelope)) MakeZoomOutTile(oVectorBorder,nCurrZoom+1,2*nX+j,2*nY+i,nBaseZoom,nMinZoom,oParams,oBuffers[i*2+j],bBlackTiles[i*2+j],nExpectedTiles,nGeneratedTiles,bOnlyCalculate,tilesContainer,nJpegQuality);
				else bBlackTiles[i*2+j] = TRUE;
			}
		}
		if (nCurrZoom < nMinZoom) return TRUE;

		int bands;
		GDALDataType gDT;
		bBlackTile	= TRUE;
		for (int i=0;i<4;i++)
		{
			if (!bBlackTiles[i])
			{
				bands		= oBuffers[i].getBandsCount();
				gDT			= oBuffers[i].getBufferDataType();
				bBlackTile	= FALSE;
			}
		}

		if (bBlackTile) return TRUE;
		
		if (bOnlyCalculate)
		{
			nExpectedTiles++;
			return TRUE;
		}
				
		for (int i=0;i<4;i++)
		{
			if (bBlackTiles[i])
			{
				oBuffers[i].createBuffer(	bands,
											TILE_SIZE,
											TILE_SIZE,
											NULL,
											gDT);
				oBuffers[i].initByNoDataValue();
			}
		}

		oBuffer.createBuffer(	bands,TILE_SIZE,TILE_SIZE,NULL,gDT);
		for (int i=0;i<4;i++)
		{
			RasterBuffer	zoomedOutBuffer;
			oBuffers[i].createSimpleZoomOut(zoomedOutBuffer);
			oBuffer.writeBlockToBuffer((i%2)*TILE_SIZE/2,(i/2)*TILE_SIZE/2,TILE_SIZE/2,TILE_SIZE/2,zoomedOutBuffer.getBufferData());
		}

		void *pData=NULL;
		int size = 0;
		switch (oParams.tileType)
		{
			case JPEG_TILE:
				{
					oBuffer.SaveToJpegData(85,pData,size);
					break;
				}
			case PNG_TILE:
				{
					oBuffer.SaveToPng24Data(pData,size);
					break;
				}
			default:
				oBuffer.SaveToTiffData(pData,size);
		}
		tilesContainer->addTile(nX,nY,nCurrZoom,(BYTE*)pData,size);
		delete[]((BYTE*)pData);
		nGeneratedTiles++;
		PrintTilingProgress(nExpectedTiles,nGeneratedTiles);
	}

	return TRUE;
}

