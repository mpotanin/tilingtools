// CopyTiles.cpp : Defines the entry point for the console application.
//

// CopyTiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	if (!LoadGdal(argc,argv)) return 0;
	GDALAllRegister();



	if (argc == 1)
	{
		wcout<<L"Usage: "<<endl;
		wcout<<L"CopyTiles [-from source folder or container file] [-to destination folder] [-border vector border] [-type (jpg|png)][-zooms MinZoom-MaxZoom][-proj tiles projection (0 - World_Mercator, 1 - Web_Mercator)]"<<endl;
		wcout<<L"Example  (copy tiles, default: type=jpg, proj=0 ):"<<endl;
		wcout<<L"CopyTiles -from c:\\all_tiles -to c:\\moscow_reg_tiles -border c:\\moscow_reg.shp -zooms 6-14"<<endl;
		return 0;
	}


	int		nMinZoom = 0;
	int		nMaxZoom;
	BOOL	bUseContainerFile;
	wstring srcPath			= MakeLower(ReadParameter(L"-from",argc,argv));
	wstring destPath		= MakeLower(ReadParameter(L"-to",argc,argv));
	wstring borderFilePath	= MakeLower(ReadParameter(L"-border",argc,argv));
	wstring strZooms		= MakeLower(ReadParameter(L"-zooms",argc,argv));
	wstring strTileType		= MakeLower(ReadParameter(L"-type",argc,argv));
	wstring strProjType		= MakeLower(ReadParameter(L"-proj",argc,argv));

	//srcPath				= L"C:\\Work\\Projects\\TilingTools\\autotest\\result\\scn_120719_Vrangel_island_SWA_tiles";
	//destPath			= L"C:\\Work\\Projects\\TilingTools\\autotest\\result\\copy2";
	//borderFilePath		= L"C:\\Work\\Projects\\TilingTools\\autotest\\border\\markers.tab";
	//strZooms			= L"1-3";
	//strProjType			= L"1";
	//*/

	if (srcPath == L"")
	{
		wcout<<L"Error: missing \"-from\" parameter"<<endl;
		return 0;
	}

	if (destPath == L"")
	{
		wcout<<L"Error: missing \"-to\" parameter"<<endl;
		return 0;
	}

	if (borderFilePath == L"")
	{
		wcout<<L"Error: missing \"-border\" parameter"<<endl;
		return 0;
	}

	if (strZooms == L"")
	{
		wcout<<L"Error: missing \"-zooms\" parameter"<<endl;
		return 0;
	}

	if (!FileExists(srcPath))
	{
		wcout<<L"Error: can't find input folder or file: "<<srcPath<<endl;
		return 0;
	}

	bUseContainerFile = IsDirectory(srcPath) ? FALSE : TRUE;
	
	if (destPath!=L"")
	{
		if (!FileExists(destPath))
		{
			if (!CreateDirectory(destPath.c_str(),NULL))
			{
				wcout<<L"Error: can't create folder: "<<destPath<<endl;
				return 0;
			}
		}
	}
	
	
	if (!FileExists(borderFilePath))
	{
		wcout<<L"Error: can't find file: "<<borderFilePath<<endl;
		return 0;
	}
	
	if (strZooms==L"")
	{
		wcout<<L"Error: you must specify min-max zoom levels"<<endl;
		return 0;
	}
	else
	{
		if (strZooms.find(L"_")>=0)
		{
			nMinZoom = (int)_wtof(strZooms.substr(0,strZooms.find(L"-")).data());
			nMaxZoom = (int)_wtof(strZooms.substr(strZooms.find(L"-")+1,strZooms.size()-strZooms.find(L"-")-1).data());
			if (nMinZoom>nMaxZoom) {int t; t=nMaxZoom; nMaxZoom = nMinZoom; nMinZoom = t;}
		}
	}
	
	TileType tileType = ((strTileType == L"") ||  (strTileType == L"jpg") || (strTileType == L"jpeg") || (strTileType == L".jpg")) ?
			JPEG_TILE : ((strTileType == L"png") || (strTileType == L".png")) ? PNG_TILE : TIFF_TILE;



	MercatorProjType mercType = ((strProjType == L"") || (strProjType == L"0") || (strProjType == L"world_mercator")|| (strProjType == L"epsg:3395")) ?
								WORLD_MERCATOR : WEB_MERCATOR;
	TileName		*poSrcTileName = NULL;
	if (!bUseContainerFile)
	{
		if (mercType == WORLD_MERCATOR)	poSrcTileName = new KosmosnimkiTileName(srcPath,tileType);
		else							poSrcTileName = new StandardTileName(srcPath,tileType);
	}
	TileContainer	*poSrcTileContainer = NULL;
	if (bUseContainerFile)	poSrcTileContainer = new TileContainerFile(srcPath);
	else					poSrcTileContainer = new TileContainerFolder(poSrcTileName);				

	TileName		*poDestTileName = NULL;
	if (mercType == WORLD_MERCATOR)	poDestTileName = new KosmosnimkiTileName(destPath,tileType);
	else							poDestTileName = new StandardTileName(destPath,tileType);
	TileContainerFolder destTileContainer(poDestTileName); 
	
		
	list<__int64> oTileList;
	wcout<<L"calculating number of tiles ... ";
	wcout<<poSrcTileContainer->getTileList(oTileList,nMinZoom,nMaxZoom,borderFilePath)<<endl;
	if (oTileList.size()>0)
	{
		wcout<<"coping tiles: 0% ";
		int tilesCopied = 0;
		for (list<__int64>::iterator iter = oTileList.begin(); iter!=oTileList.end();iter++)
		{
			int x,y,z;
			BYTE	*tileData = NULL;
			unsigned int		tileSize = 0;
			if (poSrcTileContainer->tileXYZ((*iter),x,y,z))
			{
				poSrcTileContainer->getTile(x,y,z,tileData,tileSize);
				if (tileSize>0) 
					if(destTileContainer.addTile(x,y,z,tileData,tileSize)) tilesCopied++;
				delete[]tileData;
			}
			PrintTilingProgress(oTileList.size(),tilesCopied);
		}
	}

	
	delete(poSrcTileContainer);//->closeContainer();
	//delete(poSrcTileContainer);
	delete(poSrcTileName);
	delete(poDestTileName);
	//*/
	return 0;
}

