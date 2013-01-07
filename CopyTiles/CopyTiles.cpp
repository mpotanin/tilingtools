// CopyTiles.cpp : Defines the entry point for the console application.
//

// CopyTiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
using namespace GMT;


int _tmain(int argc, _TCHAR* argv[])
{
	if (!LoadGDAL(argc,argv)) return 0;
	GDALAllRegister();
	OGRRegisterAll();



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
	wstring srcPath			=  ReadConsoleParameter(L"-from",argc,argv);
	wstring destPath		=  ReadConsoleParameter(L"-to",argc,argv);
	wstring borderFilePath	=  ReadConsoleParameter(L"-border",argc,argv);
	wstring strZooms		=  ReadConsoleParameter(L"-zooms",argc,argv);
	wstring strTileType		= MakeLower( ReadConsoleParameter(L"-tile_type",argc,argv));
	wstring strProjType		= MakeLower( ReadConsoleParameter(L"-proj",argc,argv));
	wstring strLogFile		=  ReadConsoleParameter(L"-log_file",argc,argv);
	wstring strSrcTemplate	= MakeLower( ReadConsoleParameter(L"-src_template",argc,argv));
	wstring strDestTemplate	= MakeLower( ReadConsoleParameter(L"-dest_template",argc,argv));


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
	//C:\Work\Projects\TilingTools\Release\copytiles -from C:\Work\Projects\TilingTools\autotest\result\Arctic_r06c03.2012193.terra.250m_tiles -to C:\Work\Projects\TilingTools\autotest\result\copy4 -border C:\Work\Projects\TilingTools\autotest\border_arctic\markers.tab -zooms 1-6
	//srcPath			= L"E:\\TestData\\ik_po_426174_0050002_ch1-3_8bit_merc.mbtiles";
	//destPath		= L"\\\\192.168.14.50\\ifs\\kosmo\\MWF\\UF\\LayerManager\\Maps\\TestPotanin\\mbtiles\\ik_po_426174_0050002_ch1-3_8bit_merc_4.tiles";

	//srcPath			= L"C:\\Work\\Projects\\TilingTools\\autotest\\result\\Arctic_r06c03.2012193.terra.250m_tiles";
    //destPath			= L"C:\\Work\\Projects\\TilingTools\\autotest\\A1204011102_merc_2.tiles";
	//srcPath				= L"C:\\Work\\Projects\\TilingTools\\autotest\\A1204011102_merc.mbtiles";

	//destPath			= L"C:\\Work\\Projects\\TilingTools\\autotest\\result\\copy1";
	//borderFilePath	= L"C:\\Work\\Projects\\TilingTools\\autotest\\border_arctic\\markers.tab";
	//strZooms			= L"1-5";
	//strProjType		= L"0";
	//strDestTemplate	= L"standard";
	//strSrcTemplate	= L"standard";
	//strTileType		= L"png";
	//*/

	if (srcPath == L"")
	{
		wcout<<L"Error: missing \"-from\" parameter"<<endl;
		return 0;
	}

	/*
	if (MakeLower(destPath).find(L".mbtiles") != wstring::npos)
	{
		wcout<<L"Error: copytiles currently supports MBTiles only for reading"<<endl;
		return 0;
	}
	*/
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

	//bSrcContainerFile = IsDirectory(srcPath) ? FALSE : TRUE;
	
	if (FileExists(destPath))
	{
		if (!IsDirectory(destPath))
		{
			if (!DeleteFile(destPath.c_str()))
			{
				wcout<<L"Error: can't delete existing file: "<<destPath<<endl;
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
	

	if ((strDestTemplate!=L"") &&  
		(strDestTemplate!=L"standard")&&
		(strDestTemplate!=L"kosmosnimki"))
		if (!StandardTileName::validateTemplate(strDestTemplate)) return FALSE;

	
	if ((IsDirectory(srcPath)) &&
		(strSrcTemplate!=L"") && 
		(strSrcTemplate!=L"standard")&&
		(strSrcTemplate!=L"kosmosnimki"))
		if (!StandardTileName::validateTemplate(strSrcTemplate))
		{
			wcout<<L"Error: can't validate src template: "<<strSrcTemplate<<endl;
			return 0;
		}

	TileType tileType;
	MercatorProjType mercType;

	if (IsDirectory(srcPath))
	{
		if (strTileType == L"")
		{
			strTileType = ((strSrcTemplate!=L"") && (strSrcTemplate!=L"kosmosnimki") && (strSrcTemplate!=L"standard")) ?
						strSrcTemplate.substr(strSrcTemplate.rfind(L".")+1,strSrcTemplate.length()-strSrcTemplate.rfind(L".")-1) :
						L"jpg";
		}
		tileType = ((strTileType == L"") ||  (strTileType == L"jpg") || (strTileType == L"jpeg") || (strTileType == L".jpg")) ?
					JPEG_TILE : ((strTileType == L"png") || (strTileType == L".png")) ? PNG_TILE : TIFF_TILE;
		mercType = ((strProjType == L"") || (strProjType == L"0") || (strProjType == L"world_mercator")|| (strProjType == L"epsg:3395")) ?
					WORLD_MERCATOR : WEB_MERCATOR;
	}
	else
	{
		TileContainer *poSrcContainer = GMTOpenTileContainerForReading(srcPath);
		if (! poSrcContainer)
		{
			wcout<<L"Error: can't init. tile container: "<<srcPath<<endl;
			return 0;
		}

		tileType = poSrcContainer->getTileType();
		mercType	= poSrcContainer->getProjType();
		if ((MakeLower(srcPath).find(L".mbtiles") != wstring::npos) && strProjType!=L"")
			mercType = ((strProjType == L"") || (strProjType == L"0") || (strProjType == L"world_mercator")|| (strProjType == L"epsg:3395")) ?
					WORLD_MERCATOR : WEB_MERCATOR;

		wcout<<L"Input container info: tileType="<<TileName::tileExtension(poSrcContainer->getTileType());
		wcout<<L", proj="<<(poSrcContainer->getProjType()==WEB_MERCATOR)<<endl;
		delete(poSrcContainer);		
	}


	
	TileName		*poSrcTileName = NULL;
	if (IsDirectory(srcPath))
	{
		if ((strSrcTemplate==L"") || (strSrcTemplate==L"kosmosnimki"))
			poSrcTileName = new KosmosnimkiTileName(srcPath,tileType);
		else if (strSrcTemplate==L"standard") 
			poSrcTileName = new StandardTileName(srcPath,(L"{z}\\{x}\\{z}_{x}_{y}."+TileName::tileExtension(tileType)));
		else 
			poSrcTileName = new StandardTileName(srcPath,strSrcTemplate);
	}

	TilePyramid	*poSrcTilePyramid = NULL;
	if (IsDirectory(srcPath))	poSrcTilePyramid = new TileFolder(poSrcTileName, FALSE);	
	else						poSrcTilePyramid = GMTOpenTileContainerForReading(srcPath);
	

	TileName		*poDestTileName		= NULL;
	TilePyramid		*poDestTilePyramid	= NULL;
	BOOL			bDestFolder			= (	(MakeLower(destPath).find(L".mbtiles") == wstring::npos) && 
											(MakeLower(destPath).find(L".tiles") == wstring::npos)) ? TRUE : FALSE;
	if (bDestFolder)
	{
		if (!FileExists(destPath))
		{
			if (!CreateDirectory(destPath.c_str(),NULL))
			{
				wcout<<L"Error: can't create folder: "<<destPath<<endl;
				return 0;
			}
		}

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
		OGREnvelope mercEnvelope;
		if (borderFilePath!=L"")
		{
			VectorBorder *pVB = VectorBorder::createFromVectorFile(borderFilePath,mercType);
			if (!pVB) return 0;
			mercEnvelope = pVB->getEnvelope();
			delete(pVB);
		}
		else mercEnvelope = poSrcTilePyramid->getMercatorEnvelope();
		
		if (MakeLower(destPath).find(L".mbtiles") != wstring::npos) 
			poDestTilePyramid = new MBTileContainer(destPath,tileType,mercType,mercEnvelope); 
		else
			poDestTilePyramid = new GMTileContainer(destPath,
													tileType,
													mercType,
													mercEnvelope,
													(nMaxZoom == -1) ? poSrcTilePyramid->getMaxZoom() : nMaxZoom,
													FALSE);
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
			GMTPrintTilingProgress(oTileList.size(),tilesCopied);
		}
	}
	if (logFile) fclose(logFile);
	if (poDestTilePyramid) poDestTilePyramid->close();
	delete(poSrcTilePyramid);
	delete(poSrcTileName);
	delete(poDestTileName);
	delete(poDestTilePyramid);


	return 0;
}

