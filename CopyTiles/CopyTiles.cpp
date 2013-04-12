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
		GMX::wstrToUtf8(argv[i],argvW[i]);
		GMX::ReplaceAll(argv[i],"\\","/");
	}
	
	if (!GMX::LoadGDAL(argc,argv)) return -1;
	GDALAllRegister();
	OGRRegisterAll();

	int		nMinZoom = 0;
	int		nMaxZoom;
	string srcPath			= GMX::ReadConsoleParameter("-from",argc,argv);
	string destPath			= GMX::ReadConsoleParameter("-to",argc,argv);
	string borderFilePath	= GMX::ReadConsoleParameter("-border",argc,argv);
	string strZooms			= GMX::ReadConsoleParameter("-zooms",argc,argv);
	string strTileType		= GMX::MakeLower( GMX::ReadConsoleParameter("-tile_type",argc,argv));
	string strProjType		= GMX::MakeLower( GMX::ReadConsoleParameter("-proj",argc,argv));
	string strLogFile		= GMX::ReadConsoleParameter("-log_file",argc,argv);
	string strSrcTemplate	= GMX::MakeLower( GMX::ReadConsoleParameter("-src_template",argc,argv));
	string strDestTemplate	= GMX::MakeLower( GMX::ReadConsoleParameter("-dest_template",argc,argv));


 	FILE *logFile = NULL;
	/*
	if (strLogFile!="")
	{
		if((logFile = GMX::OpenFile _wfreopen(strLogFile.c_str(), "w", stdout)) == NULL)
		{
			cout<<"ERROR: can't open log file: "<<strLogFile<<endl;
			exit(-1);
		}
	}
	*/
	//srcPath			= "C:\\Users\\mpotanin\\Downloads\\Layers.mbtiles";
	//destPath		= "C:\\Users\\mpotanin\\Downloads\\Layers.tiles";



	if (srcPath == "")
	{
		cout<<"ERROR: missing \"-from\" parameter"<<endl;
		return -1;
	}

	if (destPath == "")
	{
		cout<<"ERROR: missing \"-to\" parameter"<<endl;
		return -1;
	}

	/*
	if (borderFilePath == "")
	{
		cout<<"ERROR: missing \"-border\" parameter"<<endl;
		return -1;
	}
	*/

	if (!GMX::FileExists(srcPath))
	{
		cout<<"ERROR: can't find input folder or file: "<<srcPath<<endl;
		return -1;
	}

	//bSrcContainerFile = IsDirectory(srcPath) ? FALSE : TRUE;
	
	if (GMX::FileExists(destPath))
	{
		if (!GMX::IsDirectory(destPath))
		{
			if (!GMX::DeleteFile(destPath))
			{
				cout<<"ERROR: can't delete existing file: "<<destPath<<endl;
				return -1;
			}
		}
	}


	if ((borderFilePath != "") && !GMX::FileExists(borderFilePath))
	{
		cout<<"ERROR: can't open file: "<<borderFilePath<<endl;
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
		if (!GMX::StandardTileName::validateTemplate(strDestTemplate)) return FALSE;

	
	if ((GMX::IsDirectory(srcPath)) &&
		(strSrcTemplate!="") && 
		(strSrcTemplate!="standard")&&
		(strSrcTemplate!="kosmosnimki"))
		if (!GMX::StandardTileName::validateTemplate(strSrcTemplate))
		{
			cout<<"ERROR: can't validate src template: "<<strSrcTemplate<<endl;
			return -1;
		}

	GMX::TileType tileType;
	GMX::MercatorProjType mercType;

	if (GMX::IsDirectory(srcPath))
	{
		if (strTileType == "")
		{
			strTileType = ((strSrcTemplate!="") && (strSrcTemplate!="kosmosnimki") && (strSrcTemplate!="standard")) ?
						strSrcTemplate.substr(	strSrcTemplate.rfind(".") + 1,
												strSrcTemplate.length()-strSrcTemplate.rfind(".") - 1) :
						"jpg";
		}
		tileType = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					GMX::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? GMX::PNG_TILE : GMX::TIFF_TILE;
		mercType = ((strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")|| 
					(strProjType == "epsg:3395")) ? GMX::WORLD_MERCATOR : GMX::WEB_MERCATOR;
	}
	else
	{
		//ToDo
		GMX::ITileContainer *poSrcContainer = GMX::OpenITileContainerForReading(srcPath);
		if (! poSrcContainer)
		{
			cout<<"ERROR: can't init. tile container: "<<srcPath<<endl;
			return -1;
		}

		tileType = poSrcContainer->getTileType();
		mercType	= poSrcContainer->getProjType();
		if ((GMX::MakeLower(srcPath).find(".mbtiles") != string::npos))
		{
			if (strProjType!="")
				mercType = ((strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")|| 
							(strProjType == "epsg:3395")) ? GMX::WORLD_MERCATOR : GMX::WEB_MERCATOR;
			if (strTileType!="")
				tileType = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					GMX::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? GMX::PNG_TILE : GMX::TIFF_TILE;
		}

		cout<<"Input container info: tileType="<<GMX::TileName::tileExtension(poSrcContainer->getTileType());
		cout<<", proj="<<(poSrcContainer->getProjType()==GMX::WEB_MERCATOR)<<endl;
		delete(poSrcContainer);		
	}


	
	GMX::TileName		*poSrcTileName = NULL;
	if (GMX::IsDirectory(srcPath))
	{
		if ((strSrcTemplate=="") || (strSrcTemplate=="kosmosnimki"))
			poSrcTileName = new GMX::KosmosnimkiTileName(srcPath,tileType);
		else if (strSrcTemplate=="standard") 
			poSrcTileName = new GMX::StandardTileName(srcPath,("{z}/{x}/{z}_{x}_{y}."+ GMX::TileName::tileExtension(tileType)));
		else 
			poSrcTileName = new GMX::StandardTileName(srcPath,strSrcTemplate);
	}

	GMX::ITilePyramid	*poSrcITilePyramid = NULL;
	if (GMX::IsDirectory(srcPath))	poSrcITilePyramid = new GMX::TileFolder(poSrcTileName, FALSE);	
	else						poSrcITilePyramid = GMX::OpenITileContainerForReading(srcPath);
	

	GMX::TileName		*poDestTileName		= NULL;
	GMX::ITilePyramid	*poDestITilePyramid	= NULL;
	BOOL			bDestFolder			= (	(GMX::MakeLower(destPath).find(".mbtiles") == string::npos) && 
											(GMX::MakeLower(destPath).find(".tiles") == string::npos)) ? TRUE : FALSE;
	if (bDestFolder)
	{
		if (!GMX::FileExists(destPath))
		{
			if (!GMX::CreateDirectory(destPath.c_str()))
			{
				cout<<"ERROR: can't create folder: "<<destPath<<endl;
				return -1;
			}
		}

		if ((strDestTemplate=="") || (strDestTemplate=="kosmosnimki"))
			poDestTileName = new GMX::KosmosnimkiTileName(destPath,tileType);
		else if (strDestTemplate=="standard") 
			poDestTileName = new GMX::StandardTileName(destPath,("{z}/{x}/{z}_{x}_{y}."+GMX::TileName::tileExtension(tileType)));
		else 
			poDestTileName = new GMX::StandardTileName(destPath,strDestTemplate);

		poDestITilePyramid = new GMX::TileFolder(poDestTileName,FALSE);
	}
	else
	{
		OGREnvelope mercEnvelope;
		int tileBounds[92];

		if (borderFilePath!="")
		{
			GMX::VectorBorder *pVB = GMX::VectorBorder::createFromVectorFile(borderFilePath,mercType);
			if (!pVB)
			{
				cout<<"ERROR: reading vector file: "<<borderFilePath<<endl;
				return -1;
			}
			mercEnvelope = pVB->getEnvelope();
			delete(pVB);
		}
		else
		{
			if (!poSrcITilePyramid->getTileBounds(tileBounds))
			{
				cout<<"ERROR: reading tile bounds from source: "<<srcPath<<endl;
				return -1;
			}
		}
		if (GMX::MakeLower(destPath).find(".mbtiles") != string::npos) 
			poDestITilePyramid = new GMX::MBTileContainer(destPath,tileType,mercType,mercEnvelope); 
		else
		{
			poDestITilePyramid =  (borderFilePath!="") ?  new GMX::GMXTileContainer(destPath,
																				tileType,
																				mercType,
																				mercEnvelope,
																				(nMaxZoom == -1) ? poSrcITilePyramid->getMaxZoom() : nMaxZoom,
																				FALSE)
													: new GMX::GMXTileContainer(destPath,
																				tileType,
																				mercType,
																				tileBounds,
																				FALSE);
		}
	}
	
	list<__int64> oTileList;
	cout<<"calculating number of tiles: ";
	cout<<poSrcITilePyramid->getTileList(oTileList,nMinZoom,nMaxZoom,borderFilePath)<<endl;
	
	if (oTileList.size()>0)
	{
		cout<<"coping tiles: 0% ";
		int tilesCopied = 0;
		for (list<__int64>::iterator iter = oTileList.begin(); iter!=oTileList.end();iter++)
		{
			int x,y,z;
			BYTE	*tileData = NULL;
			unsigned int		tileSize = 0;
			if (poSrcITilePyramid->tileXYZ((*iter),z,x,y))
			{
				poSrcITilePyramid->getTile(z,x,y,tileData,tileSize);
				if (tileSize>0)
				{
					if(poDestITilePyramid->addTile(z,x,y,tileData,tileSize)) tilesCopied++;
				}
				delete[]tileData;
			}
			GMXPrintTilingProgress(oTileList.size(),tilesCopied);
		}
	}
	if (logFile) fclose(logFile);
	if (poDestITilePyramid) poDestITilePyramid->close();
	delete(poSrcITilePyramid);
	delete(poSrcTileName);
	delete(poDestTileName);
	delete(poDestITilePyramid);


	return 0;
}

