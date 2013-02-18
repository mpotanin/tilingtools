// CopyTiles.cpp : Defines the entry point for the console application.
//

// CopyTiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int _tmain(int argc, wchar_t* argvW[])
{
	if (argc == 1)
	{
		cout<<"Usage: "<<endl;
		cout<<"CopyTiles [-from input folder or container] [-to destination folder or container] [-border vector border] [-type input tiles type (jpg|png|tif)] [-zooms MinZoom-MaxZoom] [-proj input tiles projection (0 - World_Mercator, 1 - Web_Mercator)] [-src_template source tile name template] [-dest_template destination tile name template]\n"<<endl;
		cout<<"Example  (copy tiles, default: type=jpg, proj=0 ):"<<endl;
		cout<<"CopyTiles -from c:\\all_tiles -to c:\\moscow_reg_tiles -border c:\\moscow_reg.shp -zooms 6-14"<<endl;
		return 0;
	}

	string *argv = new string[argc];
	for (int i=0;i<argc;i++)
	{
		GMT::wstrToUtf8(argv[i],argvW[i]);
		GMT::ReplaceAll(argv[i],"\\","/");
	}
	
	if (!GMT::LoadGDAL(argc,argv)) return -1;
	GDALAllRegister();
	OGRRegisterAll();

	int		nMinZoom = 0;
	int		nMaxZoom;
	string srcPath			= GMT::ReadConsoleParameter("-from",argc,argv);
	string destPath			= GMT::ReadConsoleParameter("-to",argc,argv);
	string borderFilePath	= GMT::ReadConsoleParameter("-border",argc,argv);
	string strZooms			= GMT::ReadConsoleParameter("-zooms",argc,argv);
	string strTileType		= GMT::MakeLower( GMT::ReadConsoleParameter("-tile_type",argc,argv));
	string strProjType		= GMT::MakeLower( GMT::ReadConsoleParameter("-proj",argc,argv));
	string strLogFile		= GMT::ReadConsoleParameter("-log_file",argc,argv);
	string strSrcTemplate	= GMT::MakeLower( GMT::ReadConsoleParameter("-src_template",argc,argv));
	string strDestTemplate	= GMT::MakeLower( GMT::ReadConsoleParameter("-dest_template",argc,argv));


 	FILE *logFile = NULL;
	/*
	if (strLogFile!="")
	{
		if((logFile = GMT::OpenFile _wfreopen(strLogFile.c_str(), "w", stdout)) == NULL)
		{
			cout<<"Error: can't open log file: "<<strLogFile<<endl;
			exit(-1);
		}
	}
	*/

	//srcPath		= "C:\\TileMill\\export\\export\\VHR_test_spot5_cover.mbtiles";
	//destPath	= "C:\\TileMill\\export\\export\\VHR_test_spot5_cover_tiles";



	if (srcPath == "")
	{
		cout<<"Error: missing \"-from\" parameter"<<endl;
		return -1;
	}

	if (destPath == "")
	{
		cout<<"Error: missing \"-to\" parameter"<<endl;
		return -1;
	}

	/*
	if (borderFilePath == "")
	{
		cout<<"Error: missing \"-border\" parameter"<<endl;
		return -1;
	}
	*/

	if (!GMT::FileExists(srcPath))
	{
		cout<<"Error: can't find input folder or file: "<<srcPath<<endl;
		return -1;
	}

	//bSrcContainerFile = IsDirectory(srcPath) ? FALSE : TRUE;
	
	if (GMT::FileExists(destPath))
	{
		if (!GMT::IsDirectory(destPath))
		{
			if (!GMT::DeleteFile(destPath))
			{
				cout<<"Error: can't delete existing file: "<<destPath<<endl;
				return -1;
			}
		}
	}


	if ((borderFilePath != "") && !GMT::FileExists(borderFilePath))
	{
		cout<<"Error: can't open file: "<<borderFilePath<<endl;
		return -1;
	}
	
	if (strZooms=="")
	{
		nMinZoom=(nMaxZoom=-1);
	}
	else
	{
		if (strZooms.find("_")>=0)
		{
			nMinZoom = (int)atof(strZooms.substr(0,strZooms.find("-")).data());
			nMaxZoom = (int)atof(strZooms.substr(strZooms.find("-")+1,strZooms.size()-strZooms.find("-")-1).data());
			if (nMinZoom>nMaxZoom) {int t; t=nMaxZoom; nMaxZoom = nMinZoom; nMinZoom = t;}
		}
	}
	

	if ((strDestTemplate!="") &&  
		(strDestTemplate!="standard")&&
		(strDestTemplate!="kosmosnimki"))
		if (!GMT::StandardTileName::validateTemplate(strDestTemplate)) return FALSE;

	
	if ((GMT::IsDirectory(srcPath)) &&
		(strSrcTemplate!="") && 
		(strSrcTemplate!="standard")&&
		(strSrcTemplate!="kosmosnimki"))
		if (!GMT::StandardTileName::validateTemplate(strSrcTemplate))
		{
			cout<<"Error: can't validate src template: "<<strSrcTemplate<<endl;
			return -1;
		}

	GMT::TileType tileType;
	GMT::MercatorProjType mercType;

	if (GMT::IsDirectory(srcPath))
	{
		if (strTileType == "")
		{
			strTileType = ((strSrcTemplate!="") && (strSrcTemplate!="kosmosnimki") && (strSrcTemplate!="standard")) ?
						strSrcTemplate.substr(	strSrcTemplate.rfind(".") + 1,
												strSrcTemplate.length()-strSrcTemplate.rfind(".") - 1) :
						"jpg";
		}
		tileType = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					GMT::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? GMT::PNG_TILE : GMT::TIFF_TILE;
		mercType = ((strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")|| 
					(strProjType == "epsg:3395")) ? GMT::WORLD_MERCATOR : GMT::WEB_MERCATOR;
	}
	else
	{
		//ToDo
		GMT::TileContainer *poSrcContainer = GMT::OpenTileContainerForReading(srcPath);
		if (! poSrcContainer)
		{
			cout<<"Error: can't init. tile container: "<<srcPath<<endl;
			return -1;
		}

		tileType = poSrcContainer->getTileType();
		mercType	= poSrcContainer->getProjType();
		if ((GMT::MakeLower(srcPath).find(".mbtiles") != string::npos))
		{
			if (strProjType!="")
				mercType = ((strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")|| 
							(strProjType == "epsg:3395")) ? GMT::WORLD_MERCATOR : GMT::WEB_MERCATOR;
			if (strTileType!="")
				tileType = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					GMT::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? GMT::PNG_TILE : GMT::TIFF_TILE;
		}

		cout<<"Input container info: tileType="<<GMT::TileName::tileExtension(poSrcContainer->getTileType());
		cout<<", proj="<<(poSrcContainer->getProjType()==GMT::WEB_MERCATOR)<<endl;
		delete(poSrcContainer);		
	}


	
	GMT::TileName		*poSrcTileName = NULL;
	if (GMT::IsDirectory(srcPath))
	{
		if ((strSrcTemplate=="") || (strSrcTemplate=="kosmosnimki"))
			poSrcTileName = new GMT::KosmosnimkiTileName(srcPath,tileType);
		else if (strSrcTemplate=="standard") 
			poSrcTileName = new GMT::StandardTileName(srcPath,("{z}/{x}/{z}_{x}_{y}."+ GMT::TileName::tileExtension(tileType)));
		else 
			poSrcTileName = new GMT::StandardTileName(srcPath,strSrcTemplate);
	}

	GMT::TilePyramid	*poSrcTilePyramid = NULL;
	if (GMT::IsDirectory(srcPath))	poSrcTilePyramid = new GMT::TileFolder(poSrcTileName, FALSE);	
	else						poSrcTilePyramid = GMT::OpenTileContainerForReading(srcPath);
	

	GMT::TileName		*poDestTileName		= NULL;
	GMT::TilePyramid		*poDestTilePyramid	= NULL;
	BOOL			bDestFolder			= (	(GMT::MakeLower(destPath).find(".mbtiles") == string::npos) && 
											(GMT::MakeLower(destPath).find(".tiles") == string::npos)) ? TRUE : FALSE;
	if (bDestFolder)
	{
		if (!GMT::FileExists(destPath))
		{
			if (!GMT::CreateDirectory(destPath.c_str()))
			{
				cout<<"Error: can't create folder: "<<destPath<<endl;
				return -1;
			}
		}

		if ((strDestTemplate=="") || (strDestTemplate=="kosmosnimki"))
			poDestTileName = new GMT::KosmosnimkiTileName(destPath,tileType);
		else if (strDestTemplate=="standard") 
			poDestTileName = new GMT::StandardTileName(destPath,("{z}/{x}/{z}_{x}_{y}."+GMT::TileName::tileExtension(tileType)));
		else 
			poDestTileName = new GMT::StandardTileName(destPath,strDestTemplate);

		poDestTilePyramid = new GMT::TileFolder(poDestTileName,FALSE);
	}
	else
	{
		OGREnvelope mercEnvelope;
		if (borderFilePath!="")
		{
			GMT::VectorBorder *pVB = GMT::VectorBorder::createFromVectorFile(borderFilePath,mercType);
			if (!pVB) return -1;
			mercEnvelope = pVB->getEnvelope();
			delete(pVB);
		}
		else mercEnvelope = poSrcTilePyramid->getMercatorEnvelope();
		
		if (GMT::MakeLower(destPath).find(".mbtiles") != string::npos) 
			poDestTilePyramid = new GMT::MBTileContainer(destPath,tileType,mercType,mercEnvelope); 
		else
			poDestTilePyramid = new GMT::GMTileContainer(destPath,
														tileType,
														mercType,
														mercEnvelope,
														(nMaxZoom == -1) ? poSrcTilePyramid->getMaxZoom() : nMaxZoom,
														FALSE);
	}
	
	list<__int64> oTileList;
	cout<<"calculating number of tiles: ";
	cout<<poSrcTilePyramid->getTileList(oTileList,nMinZoom,nMaxZoom,borderFilePath)<<endl;
	
	if (oTileList.size()>0)
	{
		cout<<"coping tiles: 0% ";
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

