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


int ParseCmdLineAndCallTiling(MPLOptionParser &oOptionParser)
{
  ttx::TilingParameters oTilingParams;

  

  if (oOptionParser.GetValueList("-i").size() == 0)
  {
    cout << "ERROR: missing \"-i\" parameter" << endl;
    return 1;
  }

  ttx::BundleConsoleInput oBundleConsoleInput;
  if (!oBundleConsoleInput.InitByConsoleParams(oOptionParser.GetValueList("-i"),
    oOptionParser.GetValueList("-b"),
    oOptionParser.GetValueList("-bnd")))
    return 1;

  oTilingParams.p_bundle_input_ = &oBundleConsoleInput;


  if (!ttx::TileContainerFactory::GetTileContainerType(oOptionParser.GetOptionValue("-of"), oTilingParams.container_type_))
  {
    cout << "ERROR: not valid value of \"-of\" parameter: " << oOptionParser.GetOptionValue("-of") << endl;
    return 1;
  }


  string strProjType = oOptionParser.GetOptionValue("-tsrs");
  oTilingParams.merc_type_ = ((strProjType == "0") || (strProjType == "world_mercator") || (strProjType == "epsg:3395")) ?
    ttx::WORLD_MERCATOR : ttx::WEB_MERCATOR;

  list<string> listRasters = oBundleConsoleInput.GetRasterFiles();


  if (oOptionParser.GetOptionValue("-pseudo_png") != "")
    oTilingParams.tile_type_ = ttx::TileType::PSEUDO_PNG_TILE; //ToDo - move to creation options
  else if (oOptionParser.GetOptionValue("-tt") != "")
  {
    if (!ttx::TileName::TileTypeByExtension(oOptionParser.GetOptionValue("-tt"),
      oTilingParams.tile_type_))
    {
      cout << "ERROR: not valid value of \"-tt\" parameter: " << oOptionParser.GetOptionValue("-tt") << endl;
      return 1;
    }
  }
  else if (MPLFileSys::GetExtension(oOptionParser.GetOptionValue("-tnt")) != "")
  {
    if (!ttx::TileName::TileTypeByExtension(MPLFileSys::GetExtension(oOptionParser.GetOptionValue("-tnt")),
      oTilingParams.tile_type_))
    {
      cout << "ERROR: not valid value of \"-tnt\" parameter: " << MPLFileSys::GetExtension(oOptionParser.GetOptionValue("-tnt")) << endl;
      return 1;
    }
  }
  else
  {
    oTilingParams.tile_type_ = MPLString::MakeLower(MPLFileSys::GetExtension(*listRasters.begin())) == "png" ?
      ttx::TileType::PNG_TILE : ttx::TileType::JPEG_TILE;
  }


  //ToDo - should refactor move to this logic inside ITileContainer subclasses
  if (oOptionParser.GetOptionValue("-o") != "")
    oTilingParams.output_path_ = oOptionParser.GetOptionValue("-o");
  else
    oTilingParams.output_path_ = oTilingParams.container_type_ == ttx::TileContainerType::TILEFOLDER ?
    MPLFileSys::RemoveExtension(*listRasters.begin()) + "_tiles" :
    oTilingParams.output_path_ = MPLFileSys::RemoveExtension(*listRasters.begin()) + "." +
    ttx::TileContainerFactory::GetExtensionByTileContainerType(oTilingParams.container_type_);
  if ((oTilingParams.container_type_ == ttx::TileContainerType::TILEFOLDER) &&
    (!MPLFileSys::FileExists(oTilingParams.output_path_)))
  {
    if (!MPLFileSys::CreateDir(oTilingParams.output_path_))
    {
      cout << "ERROR: can't create folder: " << oTilingParams.output_path_ << endl;
      return 1;
    }
  }

  //ToDo - should refactor move to this logic inside ITileContainer::TileFolder subclasses
  if (oTilingParams.container_type_ == ttx::TileContainerType::TILEFOLDER)
  {
    if ((oOptionParser.GetOptionValue("-tnt") == "standard") ||
      (oOptionParser.GetOptionValue("-tnt") == ""))
    {
      oTilingParams.p_tile_name_ = new ttx::StandardTileName(oTilingParams.output_path_,
        "{z}/{x}/{y}." + ttx::TileName::ExtensionByTileType(oTilingParams.tile_type_));
    }
    else if (oOptionParser.GetOptionValue("-tnt") == "kosmosnimki")
    {
      oTilingParams.p_tile_name_ = new ttx::KosmosnimkiTileName(oTilingParams.output_path_,
        oTilingParams.tile_type_);
    }
    else
      oTilingParams.p_tile_name_ = new ttx::StandardTileName(oTilingParams.output_path_,
      oOptionParser.GetOptionValue("-tnt"));
  }


  if (oOptionParser.GetOptionValue("-z") != "")
    oTilingParams.base_zoom_ = atoi(oOptionParser.GetOptionValue("-z").c_str());


  if (oOptionParser.GetOptionValue("-minz") != "")
    oTilingParams.min_zoom_ = atoi(oOptionParser.GetOptionValue("-minz").c_str());


  if (oOptionParser.GetOptionValue("-b") != "")
    oTilingParams.vector_file_ = oOptionParser.GetOptionValue("-b");


  if (oOptionParser.GetOptionValue("-q") != "")
    oTilingParams.quality_ = atoi(oOptionParser.GetOptionValue("-q").c_str());


  if (oOptionParser.GetValueList("-nd").size() != 0)
  {
    list<string> listNodataValues = oOptionParser.GetValueList("-nd");
    oTilingParams.nd_num_ = listNodataValues.size();
    oTilingParams.p_nd_rgbcolors_ = new unsigned char*[oTilingParams.nd_num_];
    for (int i = 0; i<oTilingParams.nd_num_;i ++)
      oTilingParams.p_nd_rgbcolors_[i] = 0;
    
    int i=0;
    for (string strNodata : listNodataValues)
    {
      oTilingParams.p_nd_rgbcolors_[i] = new unsigned char[3];
      if (!MPLString::ConvertStringToRGB(strNodata, oTilingParams.p_nd_rgbcolors_[i]))
      {
        cout << "ERROR: not valid value of \"-nd\" parameter: " << strNodata << endl;
        return false;
      }
      i++;
    }
  }


  if (oOptionParser.GetOptionValue("-ndt") != "")
    oTilingParams.nodata_tolerance_ = atoi(oOptionParser.GetOptionValue("-ndt").c_str());


  if (oOptionParser.GetOptionValue("-bgc") != "")
  {
    unsigned char        pabyRGB[3];
    if (!MPLString::ConvertStringToRGB(oOptionParser.GetOptionValue("-bgc"), pabyRGB))
    {
      cout << "ERROR: not valid value of \"-bgc\" parameter: " << oOptionParser.GetOptionValue("-bgc") << endl;
      return false;
    }
    oTilingParams.p_background_color_ = new unsigned char[3];
    memcpy(oTilingParams.p_background_color_, pabyRGB, 3);
  }


  if (oOptionParser.GetOptionValue("-r") == "")
  {
    ttx::RasterFile oRF;
    if (!oRF.Init(*listRasters.begin()))
    {
      cout << "ERROR: can't open file: " << (*listRasters.begin()) << endl;
      return false;
    }
    oTilingParams.gdal_resampling_ = ( oRF.get_gdal_ds_ref()->GetRasterBand(1)->GetColorTable() ||
                                      oOptionParser.GetValueList("-nd").size()) 
                                      ? GRA_NearestNeighbour : GRA_Cubic;
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

  if (oOptionParser.GetOptionValue("-tsz") != "")
	  oTilingParams.tile_px_size_ = atoi(oOptionParser.GetOptionValue("-tsz").c_str());

  if (oOptionParser.GetOptionValue("-wt") != "")
    oTilingParams.max_work_threads_ = atoi(oOptionParser.GetOptionValue("-wt").c_str());

  if (oOptionParser.GetOptionValue("-bmarg") != "")
    oTilingParams.clip_offset_ = atof(oOptionParser.GetOptionValue("-bmarg").c_str());

  if (oOptionParser.GetKeyValueCollection("-co").size() != 0)
    oTilingParams.options_ = oOptionParser.GetKeyValueCollection("-co");


  if (oOptionParser.GetOptionValue("-isrs") != "")
    oTilingParams.user_input_srs_ = oOptionParser.GetOptionValue("-isrs");

  if (oOptionParser.GetOptionValue("-wc") != "")
	  oTilingParams.tile_chunk_size_ = atoi(oOptionParser.GetOptionValue("-wc").c_str());

  oTilingParams.calculate_histogram_ = true;   //TODO - replace by default value in GMXTileContainer init

 

  return TTXMakeTiling(&oTilingParams) ? 0 : 2;
}



const list<MPLOptionDescriptor> listDescriptors = {
  { "-i", 0, 1, 1, "input path" },
  { "-ap", 1, 0, 0, "tiling input files apart" },
  { "-o", 0, 0, 0, "output path" },
  { "-z", 0, 0, 0, "max zoom" },
  { "-b", 0, 1, 0, "vector clip mask" },
  { "-minz", 0, 0, 0, "min zoom" },
  { "-tt", 0, 0, 0, "tile type: png, jpg, jp2, tif" },
  { "-r", 0, 0, 0, "resampling method" },
  { "-q", 0, 0, 0, "compression quality" },
  { "-of", 0, 0, 0, "tile container format" },
  { "-co", 0, 2, 0, "creation options" },
  { "-isrs", 0, 0, 0, "input files srs WKT or PROJ.4 format" },
  { "-tsrs", 0, 0, 0, "tiling srs" },
  { "-tnt", 0, 0, 0, "tile name template" },
  { "-nd", 0, 1, 0, "nodata value/rgb color" },
  { "-ndt", 0, 0, 0, "nodata tolerance" },
  { "-bgc", 0, 0, 0, "background color" },
  { "-bnd", 0, 1, 0, "raster band list" },
  { "-wt", 0, 0, 0, "work threads num." },
  { "-tsz", 0, 0, 0, "tile seize (default 256)" },
  { "-pseudo_png", 1, 0, 0, "" },
  {"-bmarg", 0, 0, 0, "border margin in tiling srs units"},
  {"-wc",0, 0, 0, "warp chunk size in tiles"}
};


const list<string> listUsageExamples = {
  "imagetiling -i image.tif -of mbtiles -o image.mbtiles -tt png",
  "imagetiling -i image1.tif -i image2.tif -o image1-2_tiles -tnt standard -tt jpg -z 18 -minz 10",
  "imagetiling -i images/*.tif -of mbtiles -o images_tiles -tnt {z}_{x}_{y}.png",
  "imagetiling -i image.tif -b zone.shp -nd 0 -of mbtiles -o image.mbtiles -tt png",
  "imagetiling -i image.jpg -isrs \"+proj=longlat +datum=WGS84\""
};







#ifdef WIN32
int _tmain(int nArgs, wchar_t *pastrArgsW[])
{
	std::vector<string> vecArgs;
	for (int i = 0; i<nArgs; i++)
	{
		string strBuff;
		MPLString::wstrToUtf8(strBuff, pastrArgsW[i]);
		vecArgs.push_back(MPLString::ReplaceAll(strBuff, "\\", "/"));
	}
	if (!MPLGDALDelayLoader::Load(MPLFileSys::GetPath(vecArgs[0]) + "tilingtools.config"))
	{
		cout << "ERROR: can't load GDAL" << endl;
		return 1;
	}

	cout << endl;

#else
int main(int nArgs, char* argv[])
{
	std::cout << " ";

	std::vector<string> vecArgs;
	for (int i = 0; i<nArgs; i++)
		vecArgs.push_back(argv[i]);

	GDALAllRegister();
	OGRRegisterAll();
	CPLSetConfigOption("OGR_ENABLE_PARTIAL_REPROJECTION", "YES");
#endif

	if (nArgs == 1)
	{
		cout << "version: " << MPLFileSys::ReadTextFile(MPLFileSys::GetAbsolutePath(
			MPLFileSys::GetPath(vecArgs[0]),
			"version.txt")
			) << endl;
		cout << "build date: " << __DATE__ << endl;
		MPLOptionParser::PrintUsage(listDescriptors, listUsageExamples);
		return 0;
	}


	MPLOptionParser oOptionParser;
	if (!oOptionParser.Init(listDescriptors, vecArgs))
	{
		cout << "ERROR: input cmd line is not valid" << endl;
		return 1;
	}
  

	try
	{
		wcout << endl;
		if (oOptionParser.GetOptionValue("-ap") != "") //ToDo - file_list.size>1;
		{
      /*
      ttx::BundleConsoleInput oBundleConsoleInput;
      if (!oBundleConsoleInput.InitByConsoleParams(mapConsoleParams.at("-file"),
      mapConsoleParams.at("-bands")))
      {
      //ToDo -
      cout<<"Error: can't parse \"-file\" parameter"<<endl;
      return 1;
      }

      std::list<string> lstInputFiles = oBundleConsoleInput.GetFileList();

      bool bUseContainer = (mapConsoleParams.at("-gmxtiles")!="" || mapConsoleParams.at("-mbtiles")!="");

      for (std::list<string>::iterator iter = lstInputFiles.begin(); iter!=lstInputFiles.end();iter++)
      {
      cout<<"Tiling file: "<<(*iter)<<endl;
      map<string,string> mapConsoleParamsFix = mapConsoleParams;
      mapConsoleParamsFix.at("-file") = (*iter);

      int nBands = 0;
      int *panBands = 0;
      if (!oBundleConsoleInput.GetBands(*iter,nBands,panBands))
      {
      //ToDo - Error
      }
      else if (panBands!=0)
      {
      string strBands = ttx::ConvertIntToString(panBands[0]);
      for (int i=1;i<nBands;i++)
      {
      strBands+=",";
      strBands+=ttx::ConvertIntToString(panBands[i]);
      }
      mapConsoleParamsFix.at("-bands") = strBands;
      delete[]panBands;
      }

      if ((lstInputFiles.size()>1) && (mapConsoleParams.at("-border")==""))
      mapConsoleParamsFix.at("-border") = ttx::VectorOperations::GetVectorFileNameByRasterFileName(*iter);

      if ((lstInputFiles.size()>1)&&(mapConsoleParams.at("-tiles")!=""))
      {
      if (!ttx::MPLFileSys::FileExists(mapConsoleParams.at("-tiles")))
      {
      if (!ttx::MPLFileSys::CreateDir(mapConsoleParams.at("-tiles").c_str()))
      {
      cout<<"Error: can't create directory: "<<mapConsoleParams.at("-tiles")<<endl;
      return 1;
      }
      }

      if (bUseContainer)
      {
      mapConsoleParamsFix.at("-tiles") = (mapConsoleParams.at("-mbtiles") != "") ?
      ttx::MPLFileSys::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + MPLFileSys::RemovePath(ttx::MPLFileSys::RemoveExtension(*iter)) +".mbtiles" :
      ttx::MPLFileSys::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + MPLFileSys::RemovePath(ttx::MPLFileSys::RemoveExtension(*iter)) +".tiles";
      }
      else
      {
      mapConsoleParamsFix.at("-tiles") = ttx::MPLFileSys::RemoveEndingSlash(mapConsoleParams.at("-tiles")) + "/" + MPLFileSys::RemovePath(ttx::MPLFileSys::RemoveExtension(*iter)) +"_tiles";
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

