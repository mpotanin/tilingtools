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
		wcout<<L"CopyTiles [-from input folder or container] [-to destination folder or container] [-border vector border] [-type input tiles type (jpg|png|tif)] [-zooms MinZoom-MaxZoom] [-proj input tiles projection (0 - World_Mercator, 1 - Web_Mercator)] [-src_template source tile name template] [-dest_template destination tile name template]\n"<<endl;
		wcout<<L"Example  (copy tiles, default: type=jpg, proj=0 ):"<<endl;
		wcout<<L"CopyTiles -from c:\\all_tiles -to c:\\moscow_reg_tiles -border c:\\moscow_reg.shp -zooms 6-14"<<endl;
		return 0;
	}


	int		nMinZoom = 0;
	int		nMaxZoom;
	BOOL	bSrcContainerFile;
	BOOL	bDestContainerFile;
	wstring srcPath			= ReadParameter(L"-from",argc,argv);
	wstring destPath		= ReadParameter(L"-to",argc,argv);
	wstring borderFilePath	= ReadParameter(L"-border",argc,argv);
	wstring strZooms		= ReadParameter(L"-zooms",argc,argv);
	wstring strTileType		= MakeLower(ReadParameter(L"-type",argc,argv));
	wstring strProjType		= MakeLower(ReadParameter(L"-proj",argc,argv));
	wstring strLogFile		= ReadParameter(L"-log_file",argc,argv);
	wstring strSrcTemplate	= MakeLower(ReadParameter(L"-src_template",argc,argv));
	wstring strDestTemplate	= MakeLower(ReadParameter(L"-dest_template",argc,argv));

	 
	FILE *logFile = NULL;
	if (strLogFile!=L"")
	{
		if((logFile = _wfreopen(strLogFile.c_str(), L"w", stdout)) == NULL)
		{
			wcout<<L"Error: can't open log file: "<<strLogFile<<endl;
			exit(-1);
		}
	}


	///*
	//srcPath				= L"C:\\SCN1-e2350921_cut.tiles";
	//destPath			= L"C:\\Mosaic_001";
	//borderFilePath	= L"C:\\Work\\Projects\\TilingTools\\autotest\\border\\markers.tab";
	//strZooms			= L"1-5";
	//strProjType		= L"1";
	//strSrcTemplate	= L"standard";
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

	/*
	if (borderFilePath == L"")
	{
		wcout<<L"Error: missing \"-border\" parameter"<<endl;
		return 0;
	}
	*/

	if (!FileExists(srcPath))
	{
		wcout<<L"Error: can't find input folder or file: "<<srcPath<<endl;
		return 0;
	}

	bSrcContainerFile = IsDirectory(srcPath) ? FALSE : TRUE;
	
	if (MakeLower(GetExtension(destPath))==L"tiles")
	{
		bDestContainerFile = TRUE;
		if (FileExists(destPath))
		{
			if (!DeleteFile(destPath.c_str()))
			{
				wcout<<L"Error: can't delete file: "<<destPath<<endl;
				return 0;
			}
		}
	}
	else
	{
		bDestContainerFile = FALSE;
		if (!FileExists(destPath))
		{
			//if (
			if (!CreateDirectory(destPath.c_str(),NULL))
			{
				wcout<<L"Error: can't create folder: "<<destPath<<endl;
				return 0;
			}
		}
	}
	
	if ((borderFilePath!=L"") && !FileExists(borderFilePath))
	{
		wcout<<L"Error: can't open file: "<<borderFilePath<<endl;
		return 0;
	}
	
	if (strZooms==L"")
	{
		nMinZoom=(nMaxZoom=-1);
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
	

	MercatorProjType mercType = ((strProjType == L"") || (strProjType == L"0") || (strProjType == L"world_mercator")|| (strProjType == L"epsg:3395")) ?
								WORLD_MERCATOR : WEB_MERCATOR;

	if ((strDestTemplate!=L"") && 
		(strDestTemplate!=L"standard")&&
		(strDestTemplate!=L"kosmosnimki"))
		if (!StandardTileName::validateTemplate(strDestTemplate)) return FALSE;

	
	if ((!bSrcContainerFile) &&
		(strSrcTemplate!=L"") && 
		(strSrcTemplate!=L"standard")&&
		(strSrcTemplate!=L"kosmosnimki"))
		if (!StandardTileName::validateTemplate(strSrcTemplate)) return FALSE;

	
	TileType tileType;
	if (strTileType==L"")
	{
		if (bSrcContainerFile)
		{
			TileContainer *poSrcContainer	= TileContainer::openForReading(srcPath);
			if (poSrcContainer==NULL)
			{
				wcout<<L"Can't read input file: "<<srcPath<<endl;
				return 0;
			}

			strTileType = TileName::tileExtension(poSrcContainer->getTileType());
			mercType	= poSrcContainer->getProjType();
			wcout<<L"Input container info: tileType="<<TileName::tileExtension(poSrcContainer->getTileType());
			wcout<<L", proj="<<(poSrcContainer->getProjType()==WEB_MERCATOR)<<endl;
			delete(poSrcContainer);
		}
		else
		{
			if ((strSrcTemplate!=L"") && 
				(strSrcTemplate!=L"kosmosnimki") && 
				(strSrcTemplate!=L"standard"))
				strTileType = strSrcTemplate.substr(strSrcTemplate.rfind(L".")+1,strSrcTemplate.length()-strSrcTemplate.rfind(L".")-1);
			else
				strTileType = L"jpg";
		}
	}
	
	tileType = ((strTileType == L"") ||  (strTileType == L"jpg") || (strTileType == L"jpeg") || (strTileType == L".jpg")) ?
					JPEG_TILE : ((strTileType == L"png") || (strTileType == L".png")) ? PNG_TILE : TIFF_TILE;


	TileName		*poSrcTileName = NULL;
	if (!bSrcContainerFile)
	{
		if ((strSrcTemplate==L"") || (strSrcTemplate==L"kosmosnimki"))
			poSrcTileName = new KosmosnimkiTileName(srcPath,tileType);
		else if (strSrcTemplate==L"standard") 
			poSrcTileName = new StandardTileName(srcPath,(L"{z}\\{x}\\{z}_{x}_{y}."+TileName::tileExtension(tileType)));
		else 
			poSrcTileName = new StandardTileName(srcPath,strSrcTemplate);
	}

	TilePyramid	*poSrcTilePyramid = NULL;
	if (bSrcContainerFile)	poSrcTilePyramid = TileContainer::openForReading(srcPath);
	else					poSrcTilePyramid = new TileFolder(poSrcTileName, FALSE);				



	TileName		*poDestTileName = NULL;
	TilePyramid		*poDestTilePyramid = NULL;

	if (!bDestContainerFile)
	{
		if ((strDestTemplate==L"") || (strDestTemplate==L"kosmosnimki"))
			poDestTileName = new KosmosnimkiTileName(destPath,tileType);
		else if (strDestTemplate==L"standard") 
			poDestTileName = new StandardTileName(destPath,(L"{z}\\{x}\\{z}_{x}_{y}."+TileName::tileExtension(tileType)));
		else 
			poDestTileName = new StandardTileName(destPath,strDestTemplate);

		poDestTilePyramid = new TileFolder(poDestTileName,FALSE);
	}
	else
	{
		if (borderFilePath!=L"")
		{
			//ToDo
		}
		else
		{
			int tileBounds[128];
			if(!poSrcTilePyramid->getTileBounds(tileBounds)) 
			{
				wcout<<L"Error: no tiles found in "<<srcPath<<endl;
				return 0;
			}
			poDestTilePyramid = new TileContainer(tileBounds,TRUE,destPath,tileType,mercType);//Folder(poDestTileName,FALSE); 
		}
	}
	



	list<__int64> oTileList;
	wcout<<L"calculating number of tiles: ";
	wcout<<poSrcTilePyramid->getTileList(oTileList,nMinZoom,nMaxZoom,borderFilePath)<<endl;
	
	if (oTileList.size()>0)
	{
		wcout<<"coping tiles: 0% ";
		int tilesCopied = 0;
		for (list<__int64>::iterator iter = oTileList.begin(); iter!=oTileList.end();iter++)
		{
			int x,y,z;
			BYTE	*tileData = NULL;
			unsigned int		tileSize = 0;
			if (poSrcTilePyramid->tileXYZ((*iter),z,x,y))
			{
				poSrcTilePyramid->getTile(z,x,y,tileData,tileSize);
				if (tileSize>0) 
					if(poDestTilePyramid->addTile(z,x,y,tileData,tileSize)) tilesCopied++;
				delete[]tileData;
			}
			PrintTilingProgress(oTileList.size(),tilesCopied);
		}
	}
	if (logFile) fclose(logFile);
	if (poDestTilePyramid) poDestTilePyramid->close();

	delete(poSrcTilePyramid);//->closeContainer();
	//delete(poSrcTilePyramid);
	delete(poSrcTileName);
	delete(poDestTileName);
	delete(poDestTilePyramid);
	//*/
	return 0;
}

