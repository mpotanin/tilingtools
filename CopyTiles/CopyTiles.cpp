// CopyTiles.cpp : Defines the entry point for the console application.
//

// CopyTiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int _tmain(int argc, wchar_t* argvW[])
{
  //std::regex rx;
  //rx = "(L[A-Fa-f0-9]{1,2})";//_(R\\x{8,8})_(C\\x{8,8}).jpg";



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
		gmx::wstrToUtf8(argv[i],argvW[i]);
		gmx::ReplaceAll(argv[i],"\\","/");
	}
	
	if (!gmx::LoadGDAL(argc,argv)) return 1;
	GDALAllRegister();
	OGRRegisterAll();

	int		min_zoom = 0;
	int		nMaxZoom;
	string srcPath			= gmx::ReadConsoleParameter("-from",argc,argv);
	string destPath			= gmx::ReadConsoleParameter("-to",argc,argv);
	string borderFilePath	= gmx::ReadConsoleParameter("-border",argc,argv);
	string strZooms			= gmx::ReadConsoleParameter("-zooms",argc,argv);
	string strTileType		= gmx::MakeLower( gmx::ReadConsoleParameter("-tile_type",argc,argv));
	string strProjType		= gmx::MakeLower( gmx::ReadConsoleParameter("-proj",argc,argv));
	string strLogFile		= gmx::ReadConsoleParameter("-log_file",argc,argv);
	string strSrcTemplate	= gmx::MakeLower( gmx::ReadConsoleParameter("-src_template",argc,argv));
	string strDestTemplate	= gmx::MakeLower( gmx::ReadConsoleParameter("-dest_template",argc,argv));


 	FILE *logFile = NULL;
	/*
	if (strLogFile!="")
	{
		if((logFile = gmx::OpenFile _wfreopen(strLogFile.c_str(), "w", stdout)) == NULL)
		{
			cout<<"ERROR: can't open log file: "<<strLogFile<<endl;
			exit(-1);
		}
	}
	*/
	//srcPath			= "e:\\test_images\\po_731070_0000004_merc.tiles";
	//destPath		= "e:\\test_images\\po_731070_0000004_merc_esri_tiles";

  //strDestTemplate	= "{l}/{r}/{c}.jpg";

	//srcPath			= "C:\\Work\\Projects\\TilingTools\\autotest\\new_rast.mbtiles";
	//strInput		= "C:\\Work\\Projects\\TilingTools\\autotest\\scn_120719_Vrangel_island_SWA.tif";
	//strZoom			= "9";

	//strContainer	= "-container";
	//strTemplate		= "standard";


	//destPath		= "C:\\Work\\Projects\\TilingTools\\autotest\\new_rast_mbtiles";



	if (srcPath == "")
	{
		cout<<"ERROR: missing \"-from\" parameter"<<endl;
		return 1;
	}

	if (destPath == "")
	{
		cout<<"ERROR: missing \"-to\" parameter"<<endl;
		return 1;
	}

	/*
	if (borderFilePath == "")
	{
		cout<<"ERROR: missing \"-border\" parameter"<<endl;
		return 1;
	}
	*/

	if (!gmx::FileExists(srcPath))
	{
		cout<<"ERROR: can't find input folder or file: "<<srcPath<<endl;
		return 1;
	}

	//bSrcContainerFile = IsDirectory(srcPath) ? FALSE : TRUE;
	
	if (gmx::FileExists(destPath))
	{
		if (!gmx::IsDirectory(destPath))
		{
			if (!gmx::DeleteFile(destPath))
			{
				cout<<"ERROR: can't delete existing file: "<<destPath<<endl;
				return 1;
			}
		}
	}


	if ((borderFilePath != "") && !gmx::FileExists(borderFilePath))
	{
		cout<<"ERROR: can't open file: "<<borderFilePath<<endl;
		return 1;
	}
	
	if (strZooms=="")
	{
		min_zoom=(nMaxZoom=-1);
	}
	else
	{
		if (strZooms.find("_")>=0)
		{
			min_zoom = (int)atof(strZooms.substr(0,strZooms.find("-")).data());
			nMaxZoom = (int)atof(strZooms.substr(strZooms.find("-")+1,strZooms.size()-strZooms.find("-")-1).data());
			if (min_zoom>nMaxZoom) {int t; t=nMaxZoom; nMaxZoom = min_zoom; min_zoom = t;}
		}
	}
	

	gmx::TileType tile_type;
	gmx::MercatorProjType merc_type;

	if (gmx::IsDirectory(srcPath))
	{
		if (strTileType == "")
		{
			strTileType = ((strSrcTemplate!="") && (strSrcTemplate!="kosmosnimki") && (strSrcTemplate!="standard")) ?
						strSrcTemplate.substr(	strSrcTemplate.rfind(".") + 1,
												strSrcTemplate.length()-strSrcTemplate.rfind(".") - 1) :
						"jpg";
		}
		tile_type = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					gmx::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? gmx::PNG_TILE : gmx::TIFF_TILE;
		merc_type = ((strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")|| 
					(strProjType == "epsg:3395")) ? gmx::WORLD_MERCATOR : gmx::WEB_MERCATOR;
	}
	else
	{
		//ToDo
		gmx::ITileContainer *poSrcContainer = gmx::OpenTileContainerForReading(srcPath);
		if (! poSrcContainer)
		{
			cout<<"ERROR: can't init. tile container: "<<srcPath<<endl;
			return 1;
		}

		tile_type = poSrcContainer->GetTileType();
		merc_type	= poSrcContainer->GetProjType();
		if ((gmx::MakeLower(srcPath).find(".mbtiles") != string::npos))
		{
			if (strProjType!="")
				merc_type = ((strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")|| 
							(strProjType == "epsg:3395")) ? gmx::WORLD_MERCATOR : gmx::WEB_MERCATOR;
			if (strTileType!="")
				tile_type = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					gmx::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? gmx::PNG_TILE : gmx::TIFF_TILE;
		}

		cout<<"Input container info: tile_type="<<gmx::TileName::ExtensionByTileType(poSrcContainer->GetTileType());
		cout<<", proj="<<(poSrcContainer->GetProjType()==gmx::WEB_MERCATOR)<<endl;
		delete(poSrcContainer);		
	}


	
	gmx::TileName		*poSrcTileName = NULL;
	if (gmx::IsDirectory(srcPath))
	{
		if ((strSrcTemplate=="") || (strSrcTemplate=="kosmosnimki"))
			poSrcTileName = new gmx::KosmosnimkiTileName(srcPath,tile_type);
		else if (strSrcTemplate=="standard") 
			poSrcTileName = new gmx::StandardTileName(srcPath,("{z}/{x}/{y}."+ gmx::TileName::ExtensionByTileType(tile_type)));
    else if (gmx::ESRITileName::ValidateTemplate(strDestTemplate))
      poSrcTileName = new gmx::ESRITileName(srcPath,strSrcTemplate);
    else if (gmx::StandardTileName::ValidateTemplate(strSrcTemplate))
			poSrcTileName = new gmx::StandardTileName(srcPath,strSrcTemplate);
    else
    {
      cout<<"ERROR: can't validate src_template: "<<strSrcTemplate<<endl;
      return 1;
    }
	}

	gmx::ITileContainer	*poSrcITileContainer = NULL;
	if (gmx::IsDirectory(srcPath))	poSrcITileContainer = new gmx::TileFolder(poSrcTileName, FALSE);	
	else						poSrcITileContainer = gmx::OpenTileContainerForReading(srcPath);
	

	gmx::TileName		*poDestTileName		= NULL;
	gmx::ITileContainer	*poDestITileContainer	= NULL;
	BOOL			bDestFolder			= (	(gmx::MakeLower(destPath).find(".mbtiles") == string::npos) && 
											(gmx::MakeLower(destPath).find(".tiles") == string::npos)) ? TRUE : FALSE;
	if (bDestFolder)
	{
		if (!gmx::FileExists(destPath))
		{
			if (!gmx::CreateDirectory(destPath.c_str()))
			{
				cout<<"ERROR: can't create folder: "<<destPath<<endl;
				return 1;
			}
		}
    
    if ((strDestTemplate=="") || (strDestTemplate=="kosmosnimki"))
			poDestTileName = new gmx::KosmosnimkiTileName(destPath,tile_type);
		else if (gmx::ESRITileName::ValidateTemplate(strDestTemplate))
      poDestTileName = new gmx::ESRITileName(destPath,strDestTemplate);
    else if (strDestTemplate=="standard") 
			poDestTileName = new gmx::StandardTileName(destPath,("{z}/{x}/{y}."+gmx::TileName::ExtensionByTileType(tile_type)));
    else if (gmx::StandardTileName::ValidateTemplate(strDestTemplate))
			poDestTileName = new gmx::StandardTileName(destPath,strDestTemplate);
    else
    {
      cout<<"ERROR: can't validate dest_template: "<<strDestTemplate<<endl;
      return 1;
    }
		poDestITileContainer = new gmx::TileFolder(poDestTileName,FALSE);
	}
	else
	{
		OGREnvelope merc_envp;
		int tile_bounds[128];

		if (borderFilePath!="")
		{
			gmx::VectorBorder *pVB = gmx::VectorBorder::CreateFromVectorFile(borderFilePath,merc_type);
			if (!pVB)
			{
				cout<<"ERROR: reading vector file: "<<borderFilePath<<endl;
				return 1;
			}
			merc_envp = pVB->GetEnvelope();
			delete(pVB);
		}
		else
		{
			if (!poSrcITileContainer->GetTileBounds(tile_bounds))
			{
				cout<<"ERROR: reading tile bounds from source: "<<srcPath<<endl;
				return 1;
			}
		}
		if (gmx::MakeLower(destPath).find(".mbtiles") != string::npos) 
			poDestITileContainer = new gmx::MBTileContainer(destPath,tile_type,merc_type,merc_envp); 
		else
		{
			poDestITileContainer =  (borderFilePath!="") ?  new gmx::GMXTileContainer(destPath,
																				tile_type,
																				merc_type,
																				merc_envp,
																				(nMaxZoom == -1) ? poSrcITileContainer->GetMaxZoom() : nMaxZoom,
																				FALSE)
													: new gmx::GMXTileContainer(destPath,
																				tile_type,
																				merc_type,
																				tile_bounds,
																				FALSE);
		}
	}
	
	list<pair<int, pair<int,int>>> tile_list;
	cout<<"calculating number of tiles: ";
	cout<<poSrcITileContainer->GetTileList(tile_list,min_zoom,nMaxZoom,borderFilePath)<<endl;
	
  if (tile_list.size()>0)
	{
		cout<<"coping tiles: 0% ";
		int tilesCopied = 0;
		for (list<pair<int, pair<int,int>>>::iterator iter = tile_list.begin(); iter!=tile_list.end();iter++)
		{
			int z = (*iter).first;
      int x = (*iter).second.first;
      int y = (*iter).second.second;

			BYTE	*tileData = NULL;
			unsigned int		tileSize = 0;
			if (poSrcITileContainer->GetTile(z,x,y,tileData,tileSize))
			{
				if(poDestITileContainer->AddTile(z,x,y,tileData,tileSize)) tilesCopied++;
      }
      delete[]tileData;
			
			GMXPrintTilingProgress(tile_list.size(),tilesCopied);
		}
	}
	if (logFile) fclose(logFile);
	if (poDestITileContainer) poDestITileContainer->Close();
	delete(poSrcITileContainer);
	delete(poSrcTileName);
	delete(poDestTileName);
	delete(poDestITileContainer);


	return 0;
}

