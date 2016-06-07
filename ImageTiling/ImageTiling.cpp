// ImageTiling.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void PrintHelp ()
{

  cout<<"Usage: ImageTiling [-file input path] [-tiles output path]"<<endl;
  cout<<"       [-border vector border] [-zoom tiling zoom]"<<endl; 
  cout<<"       [-min_zoom min tiling zoom] [-container write to geomixer container]"<<endl;
  cout<<"       [-quality jpeg/jpeg2000 quality 0-100] [-mbtiles write to MBTiles]"<<endl;
  cout<<"       [-proj tiles projection (0 - EPSG:3395, 1 - EPSG:3857)]"<<endl;
  cout<<"       [-tile_type jpg|png|jp2] [-template tile name template]"<<endl;
  cout<<"       [-nodata transparent color for png tiles]"<<endl;
  cout<<"       [-background rgb-color for tiles backgroud]"<<endl;
  cout<<"       [-nodata_tolerance tolerance value for \"no_data\" parameter]"<<endl;

  cout<<"\nExample 1 - image to simple tiles:"<<endl;
  cout<<"ImageTiling -file c:\\image.tif"<<endl;
  cout<<"(default values: -tiles=c:\\image_tiles -min_zoom=1 -proj=0 -template=kosmosnimki -tile_type=jpg)"<<endl;
  
  cout<<"\nExample 2 - image to tiles packed into geomixer container:"<<endl;
  cout<<"ImageTiling -file c:\\image.tif -container -quality 90"<<endl;
  cout<<"(default values: -tiles=c:\\image.tiles -min_zoom=1 -proj=0 -tile_type=jpg)"<<endl;

  cout<<"\nExample 3 - tiling folder of images:"<<endl;
  cout<<"ImageTiling -file c:\\images\\*.tif -container -template {z}/{x}/{z}_{x}_{y}.png  -tile_type png -nodata \"0 0 0\""<<endl;
  cout<<"(default values: -min_zoom=1 -proj=0)"<<endl;

}

void Exit()
{
  cout<<"Tiling finished - press any key"<<endl;
  char c;
  cin>>c;
}


bool InitParamsAndCallTiling (map<string,string> mapConsoleParams)
{
  string strInputPath = mapConsoleParams.at("-file");
  string strUseContainer = mapConsoleParams.at("-gmxtiles") != "" ? mapConsoleParams.at("-gmxtiles") : mapConsoleParams.at("-mbtiles");		
  string strMaxZoom = mapConsoleParams.at("-zoom");
  string strMinZoom = mapConsoleParams.at("-min_zoom");
  string strVectorFile = mapConsoleParams.at("-border");
  string strOutputPath = mapConsoleParams.at("-tiles");
  string strTileType = mapConsoleParams.at("-tile_type");
  string strProjType = mapConsoleParams.at("-proj");
  string strQuality = mapConsoleParams.at("-quality");
  string strTileNameTemplate = mapConsoleParams.at("-template");
  string strNodata = mapConsoleParams.at("-nodata");
  string strNodataTolerance = mapConsoleParams.at("-nodata_tolerance");
  string strUsePixelTiling = mapConsoleParams.at("-pixel_tiling");
  string strBackground = mapConsoleParams.at("-background");
  string strLogFile = mapConsoleParams.at("-log_file");
  string strCacheSize = mapConsoleParams.at("-cache_size");
  string strGMXVolumeSize = mapConsoleParams.at("-gmx_volume_size");
  string strGdalResampling = mapConsoleParams.at("-resampling");
  string strMaxWorkThreads = mapConsoleParams.at("-work_threads");
  string strMaxWarpThreads = mapConsoleParams.at("-warp_threads");
  string strBands = mapConsoleParams.at("-bands");
  string strPseudoPNG = mapConsoleParams.at("-pseudo_png");

 
  gmx::BandMapping oBandMapping;

  if (!oBandMapping.InitByConsoleParams(strInputPath,strBands))
  {
    //ToDo - 
    return FALSE;
  }
  list<string> lstInputFiles = oBandMapping.GetFileList();

  //синициализируем обязательные параметры для тайлинга
  gmx::MercatorProjType eMercType =	(	(strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")
                      || (strProjType == "epsg:3395")
                    ) ? gmx::WORLD_MERCATOR : gmx::WEB_MERCATOR;

  gmx::TileType eTileType;
  if (strPseudoPNG!="") 
    eTileType = gmx::TileType::PSEUDO_PNG_TILE;
  else 
  {
    if ((strTileType == ""))
    {
      if ((strTileNameTemplate!="") && (strTileNameTemplate!="kosmosnimki") && (strTileNameTemplate!="standard"))
        strTileType = strTileNameTemplate.substr(strTileNameTemplate.rfind(".")+1,
                                                  strTileNameTemplate.length()-strTileNameTemplate.rfind(".")-1
                                                  );
      else
        strTileType =  (gmx::MakeLower(gmx::GetExtension((*lstInputFiles.begin())))== "png") ? "png" : "jpg";
    }
    if (!gmx::TileName::TileTypeByExtension(strTileType,eTileType))
    {
      cout<<"Error: not valid tile type: "<<strTileType;
      return FALSE;
    }
  }  

  gmx::TilingParameters oTilingParams(lstInputFiles,eMercType,eTileType);

  if (oBandMapping.GetBandsNum() != 0) 
    oTilingParams.p_band_mapping_ = &oBandMapping;
  
  if (strMaxZoom != "")		oTilingParams.base_zoom_ = (int)atof(strMaxZoom.c_str());
  if (strMinZoom != "")	oTilingParams.min_zoom_ = (int)atof(strMinZoom.c_str());
  if (strVectorFile!="") oTilingParams.vector_file_ = strVectorFile;
  
  oTilingParams.use_container_	= !(strUseContainer=="");
  
  if (!oTilingParams.use_container_)
  {
    if (strOutputPath == "")
      strOutputPath = (lstInputFiles.size()>1) ?  gmx::RemoveEndingSlash(gmx::GetPath((*lstInputFiles.begin())))+"_tiles" :
                                                  gmx::RemoveExtension((*lstInputFiles.begin()))+"_tiles";
    
    if (!gmx::IsDirectory(strOutputPath))
    {
      if (!gmx::GMXCreateDirectory(strOutputPath.c_str()))
      {
        cout<<"Error: can't create folder: "<<strOutputPath<<endl;
        return FALSE;
      }
    }
    
  }
  else
  {
    string strContainerExt	= (strUseContainer == "-container") ? "tiles" : "mbtiles"; 
    if (gmx::IsDirectory(strOutputPath))
    {
      oTilingParams.container_file_ =  gmx::GetAbsolutePath(strOutputPath,gmx::RemoveExtension(gmx::RemovePath((*lstInputFiles.begin())))+"."+strContainerExt);
    }
    else
    {
      if (strOutputPath == "")
       oTilingParams.container_file_ = gmx::RemoveExtension((*lstInputFiles.begin())) + "." + strContainerExt;
      else
       oTilingParams.container_file_ =  (gmx::GetExtension(strOutputPath) == strContainerExt) ? strOutputPath : strOutputPath+"."+strContainerExt;
    }
  }


  if (!oTilingParams.use_container_ )
  {
    if ((strTileNameTemplate=="") || (strTileNameTemplate=="kosmosnimki"))
        oTilingParams.p_tile_name_ = new gmx::KosmosnimkiTileName(strOutputPath,eTileType);
    else if (strTileNameTemplate=="standard") 
        oTilingParams.p_tile_name_ = new gmx::StandardTileName(	strOutputPath,("{z}/{x}/{y}." + 
                                gmx::TileName::ExtensionByTileType(eTileType)));
    else if (gmx::ESRITileName::ValidateTemplate(strTileNameTemplate))
        oTilingParams.p_tile_name_ = new gmx::ESRITileName(strOutputPath,strTileNameTemplate);
    else if (gmx::StandardTileName::ValidateTemplate(strTileNameTemplate))
        oTilingParams.p_tile_name_ = new gmx::StandardTileName(strOutputPath,strTileNameTemplate);
    else
    {
      cout<<"Error: can't validate \"-template\" parameter: "<<strTileNameTemplate<<endl;
      return FALSE;
    }
  }


  if (strCacheSize!="") 
    oTilingParams.max_cache_size_ = atof(strCacheSize.c_str());
  
  if (strGMXVolumeSize!="") 
    oTilingParams.max_gmx_volume_size_ = atof(strGMXVolumeSize.c_str());


  if (strQuality!="")
    oTilingParams.jpeg_quality_ = (int)atof(strQuality.c_str());
  
  
  if (strNodata!="")
  {
    BYTE	pabyRGB[3];
    if (!gmx::ConvertStringToRGB(strNodata,pabyRGB))
    {
      cout<<"Error: bad value of parameter: \"-nodata\""<<endl;
      return FALSE;
    }
    oTilingParams.p_transparent_color_ = new BYTE[3];
    memcpy(oTilingParams.p_transparent_color_,pabyRGB,3);
  }

  if (strNodataTolerance != "")
  {
    if (((int)atof(strNodataTolerance.c_str()))>=0 && ((int)atof(strNodataTolerance.c_str()))<=100)
      oTilingParams.nodata_tolerance_ = (int)atof(strNodataTolerance.c_str());
  }

  if (strBackground!="")
  {
    BYTE	pabyRGB[3];
    if (!gmx::ConvertStringToRGB(strBackground,pabyRGB))
    {
      cout<<"Error: bad value of parameter: \"-background\""<<endl;
      return FALSE;
    }
    oTilingParams.p_background_color_ = new BYTE[3];
    memcpy(oTilingParams.p_background_color_,pabyRGB,3);
  }
  
  gmx::RasterFile oRF;
  if (!oRF.Init(*oTilingParams.file_list_.begin()))
  { 
    cout<<"Error: can't open file: "<<(*oTilingParams.file_list_.begin())<<endl;
    return FALSE;
  }
  oTilingParams.gdal_resampling_ = (strGdalResampling=="near" ||
                                    strGdalResampling=="nearest" ||
                                    oRF.get_gdal_ds_ref()->GetRasterBand(1)->GetColorTable()
                                   ) ? GRA_NearestNeighbour: GRA_Cubic;
  oRF.Close();

  oTilingParams.auto_stretching_ = true;
  oTilingParams.calculate_histogram_ = true; 
 
  if (strMaxWorkThreads != "")
    oTilingParams.max_work_threads_ = ((int)atof(strMaxWorkThreads.c_str())>0) ? (int)atof(strMaxWorkThreads.c_str()) : 0;

  if (strMaxWarpThreads != "")
    oTilingParams.max_warp_threads_ = ((int)atof(strMaxWarpThreads.c_str())>0) ? (int)atof(strMaxWarpThreads.c_str()) : 0;

  return GMXMakeTiling(&oTilingParams);
  //if (logFile) fclose(logFile);
}


int _tmain(int nArgs, wchar_t* pastrArgsW[])
{
  
  string* pastrArgs = new string[nArgs];
  for (int i=0;i<nArgs;i++)
  {
    gmx::wstrToUtf8(pastrArgs[i],pastrArgsW[i]);
    gmx::ReplaceAll(pastrArgs[i],"\\","/");
  }
  
  if (!gmx::LoadGDAL(nArgs,pastrArgs))
  {
    cout<<"Error: can't load GDAL"<<endl;
    return 1;
  }
  GDALAllRegister();
  OGRRegisterAll();

  if (nArgs == 1)
  {
    PrintHelp();
    //return 0;
  }

  try 
  {

    map<string,string> mapConsoleParams;
  
    mapConsoleParams.insert(pair<string,string>("-file",gmx::ReadConsoleParameter("-file",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-mosaic",gmx::ReadConsoleParameter("-mosaic",nArgs,pastrArgs,TRUE)));
    mapConsoleParams.insert(pair<string,string>("-gmxtiles",gmx::ReadConsoleParameter("-container",nArgs,pastrArgs,TRUE)));
    mapConsoleParams.insert(pair<string,string>("-mbtiles",gmx::ReadConsoleParameter("-mbtiles",nArgs,pastrArgs,TRUE)));
    mapConsoleParams.insert(pair<string,string>("-zoom",gmx::ReadConsoleParameter("-zoom",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-min_zoom",gmx::ReadConsoleParameter("-min_zoom",nArgs,pastrArgs)));

    mapConsoleParams.insert(pair<string,string>("-border",gmx::ReadConsoleParameter("-border",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-tiles",gmx::ReadConsoleParameter("-tiles",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-tile_type",gmx::ReadConsoleParameter("-tile_type",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-proj",gmx::ReadConsoleParameter("-proj",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-bands",gmx::ReadConsoleParameter("-bands",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-template",gmx::ReadConsoleParameter("-template",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-nodata",gmx::ReadConsoleParameter("-nodata",nArgs,pastrArgs) != "" ?
                                                            gmx::ReadConsoleParameter("-nodata",nArgs,pastrArgs) :
                                                            gmx::ReadConsoleParameter("-nodata_rgb",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-nodata_tolerance",gmx::ReadConsoleParameter("-nodata_tolerance",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-quality",gmx::ReadConsoleParameter("-quality",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-resampling",gmx::ReadConsoleParameter("-resampling",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-background",gmx::ReadConsoleParameter("-background",nArgs,pastrArgs)));
  
    mapConsoleParams.insert(pair<string,string>("-cache_size",gmx::ReadConsoleParameter("-cache_size",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-gmx_volume_size",gmx::ReadConsoleParameter("-gmx_volume_size",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-log_file",gmx::ReadConsoleParameter("-log_file",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-pixel_tiling",gmx::ReadConsoleParameter("-pixel_tiling",nArgs,pastrArgs,TRUE)));
    mapConsoleParams.insert(pair<string,string>("-work_threads",gmx::ReadConsoleParameter("-work_threads",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-warp_threads",gmx::ReadConsoleParameter("-warp_threads",nArgs,pastrArgs)));
    mapConsoleParams.insert(pair<string,string>("-pseudo_png",gmx::ReadConsoleParameter("-pseudo_png",nArgs,pastrArgs,TRUE)));


    if (nArgs == 2) mapConsoleParams.at("-file") = pastrArgs[1];
  


    //-file scn_120719_Vrangel_island_SWA.tif -container -tiles result\scn_120719_Vrangel_island_SWA.tiles -zoom 8 -gmx_volume_size 1000000 -cache_size 1000000 -resampling nearest

    //mapConsoleParams.at("-file") = "E:\\test_images\\L8\\LC81740202015127LGN00\\LC81740202015127LGN00_B6.TIF?1,,|E:\\test_images\\L8\\LC81740202015127LGN00\\LC81740202015127LGN00_B5.TIF?,1,|E:\\test_images\\L8\\LC81740202015127LGN00\\LC81740202015127LGN00_B4.TIF?,,1";
    
    mapConsoleParams.at("-file") = "C:\\Users\\mpotanin\\Downloads\\753_2\\*.jpg";
    mapConsoleParams.at("-mosaic") = "-mosaic";
    mapConsoleParams.at("-gmxtiles")="-container";
    mapConsoleParams.at("-tiles")="C:\\Users\\mpotanin\\Downloads\\753_2\\Catalog_Landsat-8_753.tiles";

    //mapConsoleParams.at("-file") = "C:\\Users\\mpotanin\\Downloads\\1\\*.tif";
    //mapConsoleParams.at("-mosaic") = "-mosaic";
    //mapConsoleParams.at("-mosaic") = "-container";


    //mapConsoleParams.at("-file")="C:\\Work\\Projects\\TilingTools\\autotest\\o42073g8.tif";
    //mapConsoleParams.at("-zoom") = "8";
    //mapConsoleParams.at("-tiles")="C:\\Work\\Projects\\TilingTools\\autotest\\scn_120719_Vrangel_island_SWA_proj1_tiles";
    //mapConsoleParams.at("-nodata")="0 0 0";
    //mapConsoleParams.at("-tile_type")="png";
        //mapConsoleParams.at("-proj")="1";

    //mapConsoleParams.at("-border")="C:\\Work\\Projects\\TilingTools\\autotest\\border\\markers.tab";
    //mapConsoleParams.at("-background")="255 255 255";
    //mapConsoleParams.at("-resampling")="nearest";
    //mapConsoleParams.at("-gmxtiles")="-container";


    //..\x64\release\imagetiling -file scn_120719_Vrangel_island_SWA.tif -tiles result\scn_120719_Vrangel_island_SWA_tiles -zoom 8 -proj 1 -template standard -nodata_rgb "0 0 0" -tile_type png -border border\markers.tab -background "127 0 0"

  

    //-gmx_volume_size 1000000 -cache_size 1000000 -resampling nearest
        //dem_cs_zl11_094_165.tif";
    //mapConsoleParams.at("-pseudo_png") = "pseudo_png";
    //mapConsoleParams.at("-tile_type") = "tif";
    //mapConsoleParams.at("-work_threads") = "1";

    //mapConsoleParams.at("-zoom") = "8";
    //mapConsoleParams.at("-background") = "255 255 255";

    //mapConsoleParams.at("-gmxtiles")="-container";
    //mapConsoleParams.at("-tiles")="E:\\test_images\\L8\\LC81740202015127LGN00\\LC81740202015127LGN00_debug.tiles";
    //mapConsoleParams.at("-resampling")="nearest";
    //mapConsoleParams.at("-tiles")="e:\\test_tiles";


    //mapConsoleParams.at("-gmxtiles")="-container";
    //mapConsoleParams.at("-border") = "\\\\192.168.4.43\\share\\spot6\\1\\Krasnodar_SP6.X08.Y13.mif";
    //mapConsoleParams.at("-tile_type") = "png";
    //mapConsoleParams.at("-nodata") = "0";
    //mapConsoleParams.at("-template")="standard";
    //mapConsoleParams.at("-file") = "E:\\test_images\\L8\\for_test\\pan8\\LC81120742014154LGN00.TIF";

  
    if (mapConsoleParams.at("-file") == "")
    {
      cout<<"Error: missing \"-file\" parameter"<<endl;
      return 1;
    }


    wcout<<endl;
    if (mapConsoleParams.at("-mosaic")== "") //ToDo - file_list.size>1;
    {
      gmx::BandMapping oBandMapping;
      if (!oBandMapping.InitByConsoleParams(mapConsoleParams.at("-file"),
                                            mapConsoleParams.at("-bands")))
      {
        //ToDo - 
        cout<<"Error: can't parse \"-file\" parameter"<<endl;
        return 1;
      }

      std::list<string> lstInputFiles = oBandMapping.GetFileList();

      bool bUseContainer = (mapConsoleParams.at("-gmxtiles")!="" || mapConsoleParams.at("-mbtiles")!="");

      for (std::list<string>::iterator iter = lstInputFiles.begin(); iter!=lstInputFiles.end();iter++)
      {
        cout<<"Tiling file: "<<(*iter)<<endl;
        map<string,string> mapConsoleParamsFix = mapConsoleParams;
        mapConsoleParamsFix.at("-file") = (*iter);
      
        int nBands = 0;
        int *panBands = 0;
        if (!oBandMapping.GetBands(*iter,nBands,panBands))
        {
          //ToDo - Error
        }
        else if (panBands!=0)
        {
          string strBands = gmx::ConvertIntToString(panBands[0]);
          for (int i=1;i<nBands;i++)
          {
            strBands+=",";
            strBands+=gmx::ConvertIntToString(panBands[i]);
          }
          mapConsoleParamsFix.at("-bands") = strBands;
          delete[]panBands;
        }

        if ((lstInputFiles.size()>1) && (mapConsoleParams.at("-border")=="")) 
          mapConsoleParamsFix.at("-border") = gmx::VectorOperations::GetVectorFileNameByRasterFileName(*iter);
      
        if ((lstInputFiles.size()>1)&&(mapConsoleParams.at("-tiles")!=""))
        {
          if (!gmx::FileExists(mapConsoleParams.at("-tiles"))) 
          {
            if (!gmx::GMXCreateDirectory(mapConsoleParams.at("-tiles").c_str()))
            {
              cout<<"Error: can't create directory: "<<mapConsoleParams.at("-tiles")<<endl;
              return 1;
            }
          }        

          if (bUseContainer)
          {
            mapConsoleParamsFix.at("-tiles") = (mapConsoleParams.at("-mbtiles") != "") ? 
                                          gmx::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +".mbtiles" :
                                          gmx::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +".tiles"; 
          }
          else
          {
            mapConsoleParamsFix.at("-tiles") = gmx::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +"_tiles"; 
          }
        }
        
        if ((!InitParamsAndCallTiling(mapConsoleParamsFix)) && lstInputFiles.size()==1)
          return 2;
        wcout<<endl;
      }
    }
    else
    {
      if (! InitParamsAndCallTiling (mapConsoleParams))
       return 2;
    }

    return 0;
  }
  catch (...)
  {
    cout<<"Error: unknown error occured"<<endl;
    return 101;
  }
  
}