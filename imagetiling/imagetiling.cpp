#include "stdafx.h"



void Exit()
{
    cout << "Tiling finished - press any key" << endl;
    char c;
    cin >> c;
}




/*
* Parses command line arguments, inits TilingParameters instance and launches tiling
*/
int ParseCmdLineAndCallTiling(MPLOptionParser &oOptionParser)
{
    ttx::TilingParameters oTilingParams;
      
    //Parse main input parameters:
    //  -i - raster file(s)
    //  -b - vector cutline(s)
    //  -bnd - band order
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


    //Define output tile container type: gmxtiles, mbtiles, tile folder, etc
    if (!ttx::TileContainerFactory::GetType(oOptionParser.GetOptionValue("-of"), 
                                                         oTilingParams.container_type_))
    {
        cout << "ERROR: not valid value of \"-of\" parameter: " << oOptionParser.GetOptionValue("-of") << endl;
        return 1;
    }

    //Define output SRS from of two possible: EPSG:3857 (WEB_MERCATOR), EPSG:3395 (WORLD_MERCATOR)
    string strProjType = oOptionParser.GetOptionValue("-tsrs");
    oTilingParams.merc_type_ = ((strProjType == "0") || (strProjType == "world_mercator") || (strProjType == "epsg:3395")) 
                            ? ttx::WORLD_MERCATOR : ttx::WEB_MERCATOR;


    //Get one of input rasters to extract some needed parameters further
    string strSampledRaster = *oBundleConsoleInput.GetRasterFiles().begin();

    //Define tile type: png, jpeg, jp2, tif, pseudo png. Some special cases are considered...
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
            cout << "ERROR: not valid value of \"-tnt\" parameter: " 
                << MPLFileSys::GetExtension(oOptionParser.GetOptionValue("-tnt")) << endl;
            return 1;
        }
    }
    else
    {
        oTilingParams.tile_type_ = MPLString::MakeLower(MPLFileSys::GetExtension(strSampledRaster)) == "png" ?
            ttx::TileType::PNG_TILE : ttx::TileType::JPEG_TILE;
    }


    //Define output path: take it from "-o" option if it was specified 
    // or set by default depending on output tile container type
    //ToDo - should refactor move to this logic inside ITileContainer subclasses
    if (oOptionParser.GetOptionValue("-o") != "")
        oTilingParams.output_path_ = oOptionParser.GetOptionValue("-o");
    else
        oTilingParams.output_path_ = oTilingParams.container_type_ == ttx::TileContainerType::TILEFOLDER 
            ? MPLFileSys::RemoveExtension(strSampledRaster) + "_tiles"
            : MPLFileSys::RemoveExtension(strSampledRaster) + "." 
                + ttx::TileContainerFactory::GetExtensionByType(oTilingParams.container_type_);
    
    
    //Create folder if output tile container type is TILEFOLDER
    if ((oTilingParams.container_type_ == ttx::TileContainerType::TILEFOLDER) 
        && (!MPLFileSys::FileExists(oTilingParams.output_path_)))
    {
        if (!MPLFileSys::CreateDir(oTilingParams.output_path_))
        {
            cout << "ERROR: can't create folder: " << oTilingParams.output_path_ << endl;
            return 1;
        }
    }


    //Define tile name template if output tile container type is TILEFOLDER
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


    //Base zoom  parameter (=max zoom)
    if (oOptionParser.GetOptionValue("-z") != "")
        oTilingParams.base_zoom_ = atoi(oOptionParser.GetOptionValue("-z").c_str());


    //min z
    if (oOptionParser.GetOptionValue("-minz") != "")
        oTilingParams.min_zoom_ = atoi(oOptionParser.GetOptionValue("-minz").c_str());


    //vector border or cutline 
    if (oOptionParser.GetOptionValue("-b") != "")
        oTilingParams.vector_file_ = oOptionParser.GetOptionValue("-b");

    //quality parameter for tile formats with compression: jpg, jp2
    if (oOptionParser.GetOptionValue("-q") != "")
        oTilingParams.quality_ = atoi(oOptionParser.GetOptionValue("-q").c_str());


    //nodata value parameter
    if (oOptionParser.GetOptionValue("-nd").size() != 0)
    {
        oTilingParams.m_bNDVDefined = true;
        oTilingParams.m_fNDV = stof(oOptionParser.GetOptionValue("-nd"));
        
    }
    

    //Resampling algorithm is defined. It will be applied for
    //warping from input raster SRS to output tiling SRS (EPSG:3857 or EPSG:3395)
    if (oOptionParser.GetOptionValue("-r") == "")
    {
        ttx::RasterFile oRF;
        if (!oRF.Init(strSampledRaster))
        {
            cout << "ERROR: can't open file: " << (strSampledRaster) << endl;
            return false;
        }
        oTilingParams.gdal_resampling_ = ( oRF.get_gdal_ds_ref()->GetRasterBand(1)->GetColorTable() ||
                                            oOptionParser.GetOptionValue("-nd").size()) 
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


    //Maximum working threads is set
    if (oOptionParser.GetOptionValue("-wt") != "")
    oTilingParams.max_work_threads_ = atoi(oOptionParser.GetOptionValue("-wt").c_str());



    if (oOptionParser.GetOptionValue("-bmarg") != "")
    oTilingParams.clip_offset_ = atof(oOptionParser.GetOptionValue("-bmarg").c_str());


    if (oOptionParser.GetKeyValueCollection("-co").size() != 0)
    oTilingParams.options_ = oOptionParser.GetKeyValueCollection("-co");


    //Input rasters SRS can be set manually in case it isn't defined 
    if (oOptionParser.GetOptionValue("-isrs") != "")
    oTilingParams.user_input_srs_ = oOptionParser.GetOptionValue("-isrs");


    if (oOptionParser.GetOptionValue("-wc") != "")
	    oTilingParams.tile_chunk_size_ = atoi(oOptionParser.GetOptionValue("-wc").c_str());

  
    oTilingParams.calculate_histogram_ = false;   //TODO - replace by default value in GMXTileContainer init

 

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
  { "-nd", 0, 0, 0, "nodata value" },
  { "-ndt", 0, 0, 0, "nodata tolerance" },
  { "-bnd", 0, 1, 0, "raster band list" },
  { "-wt", 0, 0, 0, "work threads num." },
  { "-tsz", 0, 0, 0, "tile size (default 256)" },
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
      
		}
		else return ParseCmdLineAndCallTiling(oOptionParser);
	}
	catch (...)
	{
	    cout << "ERROR: unknown error occured" << endl;
	    return 101;
	}

}

