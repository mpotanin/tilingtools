#include "stdafx.h"

void Exit()
{
  cout << "Tiling finished - press any key" << endl;
  char c;
  cin >> c;
}


//
//cases:
//1. one vector for bundle
//2. single explicitly defined vector for each raster
//3. single vector defined by name for each raster - BundleTiler.Init (raster_file_list, vector_file_list)


int ParseCmdLineAndCallTiling(GMXOptionParser &oOptionParser)
{
  gmx::TilingParameters oTilingParams;

  if (oOptionParser.GetValueList("-i").size() == 0)
  {
    cout << "ERROR: missing \"-i\" parameter" << endl;
    return 1;
  }

  gmx::BundleInputData oBundleInputData;
  if (!oBundleInputData.InitByConsoleParams(oOptionParser.GetValueList("-i"),
    oOptionParser.GetValueList("-b"),
    oOptionParser.GetValueList("-bnd")))
    return 1;

  oTilingParams.p_bundle_input_ = &oBundleInputData;


  if (!gmx::TileContainerFactory::GetTileContainerType(oOptionParser.GetOptionValue("-of"), oTilingParams.container_type_))
  {
    cout << "ERROR: not valid value of \"-of\" parameter: " << oOptionParser.GetOptionValue("-of") << endl;
    return 1;
  }


  string strProjType = oOptionParser.GetOptionValue("-tsrs");
  oTilingParams.merc_type_ = ((strProjType == "0") || (strProjType == "world_mercator") || (strProjType == "epsg:3395")) ?
    gmx::WORLD_MERCATOR : gmx::WEB_MERCATOR;

  list<string> listRasters = oBundleInputData.GetRasterFiles();

  if (oOptionParser.GetOptionValue("-pseudo_png") != "")
    oTilingParams.tile_type_ = gmx::TileType::PSEUDO_PNG_TILE; //ToDo - move to creation options
  else if (oOptionParser.GetOptionValue("-tt") != "")
  {
    if (!gmx::TileName::TileTypeByExtension(oOptionParser.GetOptionValue("-tt"),
      oTilingParams.tile_type_))
    {
      cout << "ERROR: not valid value of \"-tt\" parameter: " << oOptionParser.GetOptionValue("-tt") << endl;
      return 1;
    }
  }
  else if (GMXFileSys::GetExtension(oOptionParser.GetOptionValue("-tnt")) != "")
  {
    if (!gmx::TileName::TileTypeByExtension(GMXFileSys::GetExtension(oOptionParser.GetOptionValue("-tnt")),
      oTilingParams.tile_type_))
    {
      cout << "ERROR: not valid value of \"-tnt\" parameter: " << GMXFileSys::GetExtension(oOptionParser.GetOptionValue("-tnt")) << endl;
      return 1;
    }
  }
  else
  {
    oTilingParams.tile_type_ = GMXString::MakeLower(GMXFileSys::GetExtension(*listRasters.begin())) == "png" ?
      gmx::TileType::PNG_TILE : gmx::TileType::JPEG_TILE;
  }


  //ToDo - should refactor move to this logic inside ITileContainer subclasses
  if (oOptionParser.GetOptionValue("-o") != "")
    oTilingParams.output_path_ = oOptionParser.GetOptionValue("-o");
  else
    oTilingParams.output_path_ = oTilingParams.container_type_ == gmx::TileContainerType::TILEFOLDER ?
    GMXFileSys::RemoveExtension(*listRasters.begin()) + "_tiles" :
    oTilingParams.output_path_ = GMXFileSys::RemoveExtension(*listRasters.begin()) + "." +
    gmx::TileContainerFactory::GetExtensionByTileContainerType(oTilingParams.container_type_);
  if ((oTilingParams.container_type_ == gmx::TileContainerType::TILEFOLDER) &&
    (!GMXFileSys::FileExists(oTilingParams.output_path_)))
  {
    if (!GMXFileSys::CreateDir(oTilingParams.output_path_))
    {
      cout << "ERROR: can't create folder: " << oTilingParams.output_path_ << endl;
      return 1;
    }
  }

  //ToDo - should refactor move to this logic inside ITileContainer::TileFolder subclasses
  if (oTilingParams.container_type_ == gmx::TileContainerType::TILEFOLDER)
  {
    if ((oOptionParser.GetOptionValue("-tnt") == "standard") ||
      (oOptionParser.GetOptionValue("-tnt") == ""))
    {
      oTilingParams.p_tile_name_ = new gmx::StandardTileName(oTilingParams.output_path_,
        "{z}/{x}/{y}." + gmx::TileName::ExtensionByTileType(oTilingParams.tile_type_));
    }
    else if (oOptionParser.GetOptionValue("-tnt") == "kosmosnimki")
    {
      oTilingParams.p_tile_name_ = new gmx::KosmosnimkiTileName(oTilingParams.output_path_,
        oTilingParams.tile_type_);
    }
    else
      oTilingParams.p_tile_name_ = new gmx::StandardTileName(oTilingParams.output_path_,
      oOptionParser.GetOptionValue("-tnt"));
  }


  if (oOptionParser.GetOptionValue("-z") != "")
    oTilingParams.base_zoom_ = atoi(oOptionParser.GetOptionValue("-z").c_str());


  if (oOptionParser.GetOptionValue("-minz") != "")
    oTilingParams.min_zoom_ = atoi(oOptionParser.GetOptionValue("-minz").c_str());


  if (oOptionParser.GetOptionValue("-b") != "")
    oTilingParams.vector_file_ = oOptionParser.GetOptionValue("-b");


  if (oOptionParser.GetOptionValue("-q") != "")
    oTilingParams.jpeg_quality_ = atoi(oOptionParser.GetOptionValue("-q").c_str());


  if (oOptionParser.GetOptionValue("-nd") != "")
  {
    unsigned char pabyRGB[3];
    if (!GMXString::ConvertStringToRGB(oOptionParser.GetOptionValue("-nd"), pabyRGB))
    {
      cout << "ERROR: not valid value of \"-nd\" parameter: " << oOptionParser.GetOptionValue("-nd") << endl;
      return FALSE;
    }
    oTilingParams.p_transparent_color_ = new unsigned char[3];
    memcpy(oTilingParams.p_transparent_color_, pabyRGB, 3);
  }


  if (oOptionParser.GetOptionValue("-ndt") != "")
    oTilingParams.nodata_tolerance_ = atoi(oOptionParser.GetOptionValue("-ndt").c_str());


  if (oOptionParser.GetOptionValue("-bgc") != "")
  {
    unsigned char        pabyRGB[3];
    if (!GMXString::ConvertStringToRGB(oOptionParser.GetOptionValue("-bgc"), pabyRGB))
    {
      cout << "ERROR: not valid value of \"-bgc\" parameter: " << oOptionParser.GetOptionValue("-bgc") << endl;
      return FALSE;
    }
    oTilingParams.p_background_color_ = new unsigned char[3];
    memcpy(oTilingParams.p_background_color_, pabyRGB, 3);
  }


  if (oOptionParser.GetOptionValue("-r") == "")
  {
    gmx::RasterFile oRF;
    if (!oRF.Init(*listRasters.begin()))
    {
      cout << "ERROR: can't open file: " << (*listRasters.begin()) << endl;
      return FALSE;
    }
    oTilingParams.gdal_resampling_ = oRF.get_gdal_ds_ref()->GetRasterBand(1)->GetColorTable() ? GRA_NearestNeighbour
      : GRA_Cubic;
    oRF.Close();
  }
  else
  {
    string strResampling = oOptionParser.GetOptionValue("-r");
    if (strResampling == "near" || strResampling == "nearest")
      oTilingParams.gdal_resampling_ = GRA_NearestNeighbour;
    else if (strResampling == "bilinear")
      oTilingParams.gdal_resampling_ = GRA_Bilinear;
    else if (strResampling == "cubic")
      oTilingParams.gdal_resampling_ = GRA_Cubic;
    else if (strResampling == "lanczos")
      oTilingParams.gdal_resampling_ = GRA_Lanczos;
    else if (strResampling == "cubicspline")
      oTilingParams.gdal_resampling_ = GRA_CubicSpline;
    else
      oTilingParams.gdal_resampling_ = GRA_Cubic;
  }


  if (oOptionParser.GetOptionValue("-wt") != "")
    oTilingParams.max_work_threads_ = atoi(oOptionParser.GetOptionValue("-wt").c_str());


  if (oOptionParser.GetKeyValueCollection("-co").size() != 0)
    oTilingParams.options_ = oOptionParser.GetKeyValueCollection("-co");


  oTilingParams.auto_stretching_ = true;
  oTilingParams.calculate_histogram_ = true;   //TODO - replace by default value in GMXTileContainer init


  return GMXMakeTiling(&oTilingParams) ? 0 : 2;
}


int nDescriptors = 18;
const GMXOptionDescriptor asDescriptors[] =
{
  { "-i", 0, 1, "input path" },
  { "-ap", 1, 0, "tiling input files apart" },
  { "-o", 0, 0, "output path" },
  { "-z", 0, 0, "max zoom" },
  { "-b", 0, 1, "vector clip mask" },
  { "-minz", 0, 0, "min zoom" },
  { "-tt", 0, 0, "tile type: png, jpg, jp2, tif" },
  { "-r", 0, 0, "resampling method" },
  { "-q", 0, 0, "compression quality" },
  { "-of", 0, 0, "tile container format" },
  { "-co", 0, 2, "creation options" },
  { "-tsrs", 0, 0, "tiling srs" },
  { "-tnt", 0, 0, "tile name template" },
  { "-nd", 0, 0, "nodata value" },
  { "-ndt", 0, 0, "nodata tolerance" },
  { "-bgc", 0, 0, "background color" },
  { "-bnd", 0, 1, "raster band list" },
  { "-wt", 0, 0, "work threads num." },
  { "-pseudo_png", 1, 0, "" }
};

int nExamples = 4;
const string astrUsageExamples[] =
{
  "imagetiling -i image.tif -of mbtiles -o image.mbtiles -tt png",
  "imagetiling -i image1.tif -i image2.tif -o image1-2_tiles -tnt standard -tt jpg -z 18 -minz 10",
  "imagetiling -i images/*.tif -of mbtiles -o images_tiles -tnt {z}_{x}_{y}.png",
  "imagetiling -i image.tif -b zone.shp -nd 0 -of mbtiles -o image.mbtiles -tt png"
};



int _tmain(int nArgs, wchar_t* pastrArgsW[])
{
  string* pastrArgs = new string[nArgs];
  for (int i = 0; i<nArgs; i++)
  {
    GMXString::wstrToUtf8(pastrArgs[i], pastrArgsW[i]);
    GMXString::ReplaceAll(pastrArgs[i], "\\", "/");
  }

  if (!GMXGDALLoader::Load(GMXFileSys::GetPath(pastrArgs[0])))
  {
    cout << "ERROR: can't load GDAL" << endl;
    delete[]pastrArgs;
    return 1;
  }

  //debug
  //GMXOptionParser::InitCmdLineArgsFromFile("../autotest/debug_input.txt",nArgs,pastrArgs);
  //for (int i=0;i<nArgs;i++) GMXString::ReplaceAll(pastrArgs[i],"\\","/");
  //end-debug


  if (nArgs == 1)
  {
    cout << "version: " << GMXFileSys::ReadTextFile(GMXFileSys::GetAbsolutePath(
      GMXFileSys::GetPath(pastrArgs[0]),
      "version.txt")
      ) << endl;
    cout << "build date: " << __DATE__ << endl;
    GMXOptionParser::PrintUsage(asDescriptors, nDescriptors, astrUsageExamples, nExamples);
    delete[]pastrArgs;
    return 0;
  }

  GMXOptionParser oOptionParser;
  if (!oOptionParser.Init(asDescriptors, nDescriptors, pastrArgs, nArgs))
  {
    cout << "ERROR: input cmd line is not valid" << endl;
    delete[]pastrArgs;
    return 1;
  }
  delete[]pastrArgs;

  try
  {
    wcout << endl;
    if (oOptionParser.GetOptionValue("-ap") != "") //ToDo - file_list.size>1;
    {
      /*
      gmx::BundleInputData oBundleInputData;
      if (!oBundleInputData.InitByConsoleParams(mapConsoleParams.at("-file"),
      mapConsoleParams.at("-bands")))
      {
      //ToDo -
      cout<<"Error: can't parse \"-file\" parameter"<<endl;
      return 1;
      }

      std::list<string> lstInputFiles = oBundleInputData.GetFileList();

      bool bUseContainer = (mapConsoleParams.at("-gmxtiles")!="" || mapConsoleParams.at("-mbtiles")!="");

      for (std::list<string>::iterator iter = lstInputFiles.begin(); iter!=lstInputFiles.end();iter++)
      {
      cout<<"Tiling file: "<<(*iter)<<endl;
      map<string,string> mapConsoleParamsFix = mapConsoleParams;
      mapConsoleParamsFix.at("-file") = (*iter);

      int nBands = 0;
      int *panBands = 0;
      if (!oBundleInputData.GetBands(*iter,nBands,panBands))
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
      if (!gmx::GMXFileSys::FileExists(mapConsoleParams.at("-tiles")))
      {
      if (!gmx::GMXFileSys::CreateDir(mapConsoleParams.at("-tiles").c_str()))
      {
      cout<<"Error: can't create directory: "<<mapConsoleParams.at("-tiles")<<endl;
      return 1;
      }
      }

      if (bUseContainer)
      {
      mapConsoleParamsFix.at("-tiles") = (mapConsoleParams.at("-mbtiles") != "") ?
      gmx::GMXFileSys::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + GMXFileSys::RemovePath(gmx::GMXFileSys::RemoveExtension(*iter)) +".mbtiles" :
      gmx::GMXFileSys::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + GMXFileSys::RemovePath(gmx::GMXFileSys::RemoveExtension(*iter)) +".tiles";
      }
      else
      {
      mapConsoleParamsFix.at("-tiles") = gmx::GMXFileSys::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + GMXFileSys::RemovePath(gmx::GMXFileSys::RemoveExtension(*iter)) +"_tiles";
      }
      }

      if ((!ParseCmdLineAndCallTiling(mapConsoleParamsFix)) && lstInputFiles.size()==1)
      return 2;
      wcout<<endl;
      }
      */
    }
    else return ParseCmdLineAndCallTiling(oOptionParser);
  }
  catch (...)
  {
    cout << "ERROR: unknown error occured" << endl;
    return 101;
  }

}

