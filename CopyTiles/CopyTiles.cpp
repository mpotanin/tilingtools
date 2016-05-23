// CopyTiles.cpp : Defines the entry point for the console application.
//

// CopyTiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int _tmain(int argc, wchar_t* argvW[])
{
  if (argc == 1)
	{
		cout<<"Usage: CopyTiles [-from input folder or container]"<<endl;
    cout<<"       [-to output] [-border vector border]"<<endl;
    cout<<"       [-tile_type input tiles type (jpg|png|jp2)]"<<endl;
    cout<<"       [-zooms min-max zooms]"<<endl;
    cout<<"       [-proj input proj. (0 - World_Mercator, 1 - Web_Mercator)]"<<endl;
    cout<<"       [-src_template source tile template]"<<endl;
    cout<<"       [-dest_template destination tile template]"<<endl;
		
    cout<<"\nExample 1:"<<endl;
		cout<<"CopyTiles -from c:\\all_tiles -to c:\\moscow_reg_tiles -border c:\\moscow_reg.shp -zooms 6-14"<<endl;

    cout<<"\nExample 2:"<<endl;
		cout<<"CopyTiles -from c:\\all_tiles -to c:\\moscow_reg.mbtiles -src_template standard -border c:\\moscow_reg.shp -zooms 6-14"<<endl;
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
  string metadata_file = gmx::MakeLower( gmx::ReadConsoleParameter("-metadata",argc,argv)); 

  /*
  srcPath = "C:\\Work\\Projects\\TilingTools\\autotest\\result\\scn_120719_Vrangel_island_SWA_tiles";
  destPath = "C:\\Work\\Projects\\TilingTools\\autotest\\result\\copy2";
  strSrcTemplate = "standard";
  strZooms = "1-5";
  strProjType = "1";
  strDestTemplate="{z}_{x}_{y}.png";
  strTileType= "png";
  borderFilePath="C:\\Work\\Projects\\TilingTools\\autotest\\border\\markers.tab";
  */

 	FILE *logFile = NULL;
	
	if (srcPath == "")
	{
		cout<<"Error: missing \"-from\" parameter"<<endl;
		return 1;
	}

	if (destPath == "")
	{
		cout<<"Error: missing \"-to\" parameter"<<endl;
		return 1;
	}

	/*
	if (borderFilePath == "")
	{
		cout<<"Error: missing \"-border\" parameter"<<endl;
		return 1;
	}
	*/

	if (!gmx::FileExists(srcPath))
	{
		cout<<"Error: can't find input folder or file: "<<srcPath<<endl;
		return 1;
	}

	//bSrcContainerFile = IsDirectory(srcPath) ? FALSE : TRUE;
	
	if (gmx::FileExists(destPath))
	{
		if (!gmx::IsDirectory(destPath))
		{
			if (!gmx::GMXDeleteFile(destPath))
			{
				cout<<"Error: can't delete existing file: "<<destPath<<endl;
				return 1;
			}
		}
	}


	if ((borderFilePath != "") && !gmx::FileExists(borderFilePath))
	{
		cout<<"Error: can't open file: "<<borderFilePath<<endl;
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
  gmx::Metadata *p_metadata = NULL;

	if (gmx::IsDirectory(srcPath))
	{
		if (strTileType == "")
		{
			strTileType = ((strSrcTemplate!="") && (strSrcTemplate!="kosmosnimki") && (strSrcTemplate!="standard")) ?
						strSrcTemplate.substr(	strSrcTemplate.rfind(".") + 1,
												strSrcTemplate.length()-strSrcTemplate.rfind(".") - 1) :
						"jpg";
		}
		if (!gmx::TileName::TileTypeByExtension(strTileType,tile_type))
    {
      cout<<"Error: unknown input tile type: "<<strTileType<<endl;
		  return 1;
    }
		merc_type = ((strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")|| 
					(strProjType == "epsg:3395")) ? gmx::WORLD_MERCATOR : gmx::WEB_MERCATOR;
	}
	else
	{
		//ToDo
		gmx::ITileContainer *poSrcContainer = gmx::ITileContainer::OpenTileContainerForReading(srcPath);
		if (! poSrcContainer)
		{
			cout<<"Error: can't init. tile container: "<<srcPath<<endl;
			return 1;
		}

		tile_type = poSrcContainer->GetTileType();
		merc_type	= poSrcContainer->GetProjType();
    p_metadata = poSrcContainer->GetMetadata();

		if ((gmx::MakeLower(srcPath).find(".mbtiles") != string::npos))
		{
			if (strProjType!="")
				merc_type = ((strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")|| 
							(strProjType == "epsg:3395")) ? gmx::WORLD_MERCATOR : gmx::WEB_MERCATOR;
			if (strTileType!="")
				tile_type = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					gmx::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? gmx::PNG_TILE : gmx::TIFF_TILE;
		}

		cout<<"Input container info: tile_type="<<gmx::TileName::TileInfoByTileType(poSrcContainer->GetTileType());
		cout<<", proj="<<(poSrcContainer->GetProjType()==gmx::WEB_MERCATOR);
		if (p_metadata)
    {
      cout<<", metadata_tags="<<p_metadata->TagCount();
      if (metadata_file!="") p_metadata->SaveToTextFile(metadata_file);
      p_metadata->DeleteAll();
    }
    cout<<endl;
    delete(poSrcContainer);		
	}

  if (p_metadata) delete(p_metadata);
	
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
      cout<<"Error: can't validate src_template: "<<strSrcTemplate<<endl;
      return 1;
    }
	}

	gmx::ITileContainer	*poSrcITileContainer = NULL;
	if (gmx::IsDirectory(srcPath))	poSrcITileContainer = new gmx::TileFolder(poSrcTileName, FALSE);	
	else						poSrcITileContainer = gmx::ITileContainer::OpenTileContainerForReading(srcPath);
	

	gmx::TileName		*poDestTileName		= NULL;
	gmx::ITileContainer	*poDestITileContainer	= NULL;
	bool			is_dest_tile_folder			= (	(gmx::MakeLower(destPath).find(".mbtiles") == string::npos) && 
											(gmx::MakeLower(destPath).find(".tiles") == string::npos)) ? TRUE : FALSE;
	if (is_dest_tile_folder)
	{
		if (!gmx::FileExists(destPath))
		{
			if (!gmx::GMXCreateDirectory(destPath.c_str()))
			{
				cout<<"Error: can't create folder: "<<destPath<<endl;
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
      cout<<"Error: can't validate dest_template: "<<strDestTemplate<<endl;
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
      gmx::MercatorTileGrid merc_grid(merc_type);

      OGRGeometry* poBorder = gmx::VectorOperations::ReadAndTransformGeometry(borderFilePath,merc_grid.GetTilingSRS());

			if (!poBorder)
			{
				cout<<"Error: reading vector file: "<<borderFilePath<<endl;
				return 1;
			}
      poBorder->getEnvelope(&merc_envp);
			delete(poBorder);
		}
		else
		{
			if (!poSrcITileContainer->GetTileBounds(tile_bounds))
			{
				cout<<"Error: reading tile bounds from source: "<<srcPath<<endl;
				return 1;
			}
		}

		if (gmx::MakeLower(destPath).find(".mbtiles") != string::npos) 
			poDestITileContainer = new gmx::MBTileContainer(destPath,tile_type,merc_type,merc_envp); 
		else
		{
      poDestITileContainer = new gmx::GMXTileContainer();
      bool opened = (borderFilePath!="") ? ((gmx::GMXTileContainer*)poDestITileContainer)->OpenForWriting(destPath,
																				                                    tile_type,
																				                                    merc_type,
																				                                    merc_envp,
																				                                    (nMaxZoom == -1) ? poSrcITileContainer->GetMaxZoom() : nMaxZoom,
																				                                    FALSE)
                                                                            :
                                           ((gmx::GMXTileContainer*)poDestITileContainer)->OpenForWriting(destPath,
																			                                      tile_type,
																			                                      merc_type,
																			                                      tile_bounds,
			      													                                      FALSE);
      if (!opened)
      {
        cout<<"Error: can't open gmx-container: "<<destPath<<" for writing"<<endl;
        return 1;
      }
		}
	}
	
	list<pair<int, pair<int,int>>> tile_list;
	cout<<"calculating number of tiles: ";
	cout<<poSrcITileContainer->GetTileList(tile_list,min_zoom,nMaxZoom,borderFilePath,merc_type)<<endl;
	
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