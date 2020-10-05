#include "stdafx.h"

const list<MPLOptionDescriptor> listDescriptors = {
  { "-i", 0, 0, "input folder or file" },
  { "-o", 0, 0, "output folder or file" },
  { "-of", 0, 0, "output tile container type" },
  { "-b", 0, 0, "vector clip mask: shp,tab,mif,kml,geojson" },
  { "-tt", 0, 0, "tile type: jpg,png" },
  { "-z", 0, 0, "min-max zoom" },
  { "-co", 0, 2, "creation options" },
  { "-tsrs", 0, 0, "tiling srs" },
  { "-i_tnt", 0, 0, "input tile name template" },
  { "-o_tnt", 0, 0, "output tile name template" }
};

const list<string> listUsageExamples = {
  "copytiles -i tiles -i_tnt standard -tt png -of MBTiles -o tiles.mbtiles -z 10-15",
  "copytiles -i tiles -i_tnt {z}/{x}/{y}.png -of MBTiles -o tiles.mbtiles -b zone.shp",
  "copytiles -i tiles -i_tnt {z}/{x}/{y}.png -o tiles_new -o_tnt {z}_{x}_{y}.png"
};


#ifdef WIN32
int _tmain(int nArgs, wchar_t* pastrArgsW[])
{

	std::vector<string> vecArgs;
	for (int i = 0; i < nArgs; i++)
	{
		string strBuff;
		MPLString::wstrToUtf8(strBuff, pastrArgsW[i]);
		vecArgs.push_back(MPLString::ReplaceAll(strBuff, "\\", "/"));
	}

	if (!MPLGDALDelayLoader::Load(MPLFileSys::GetPath(vecArgs[0]) + "/tilingtools.config"))
	{
		cout << "ERROR: can't load GDAL" << endl;
		return 1;
	}

	cout << endl;
#else
int main(int nArgs, char* argv[])
{
	std::vector<string> vecArgs;
	for (int i = 0; i < nArgs; i++)
		vecArgs.push_back(argv[i]);
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

  int             nMinZoom = 0;
  int             nMaxZoom;
  string strSrcPath = oOptionParser.GetOptionValue("-i");
  string strDestPath = oOptionParser.GetOptionValue("-o");
  string strBorderFilePath = oOptionParser.GetOptionValue("-b");
  string strZooms = oOptionParser.GetOptionValue("-z");
  string strTileType = oOptionParser.GetOptionValue("-tt");
  string strProjType = oOptionParser.GetOptionValue("-tsrs");
  string strSrcTemplate = oOptionParser.GetOptionValue("-i_tnt");
  string strDestTemplate = oOptionParser.GetOptionValue("-o_tnt");
  string strOutputFormat = oOptionParser.GetOptionValue("-of");
  map<string, string> strCreationOptions = oOptionParser.GetKeyValueCollection("-co");

  FILE *logFile = NULL;

  if (strSrcPath == "")
  {
    cout << "ERROR: missing \"-i\" parameter" << endl;
    return 1;
  }

  if (strDestPath == "")
  {
    cout << "ERROR: missing \"-o\" parameter" << endl;
    return 1;
  }

  if (!MPLFileSys::FileExists(strSrcPath))
  {
    cout << "ERROR: can't find input path: " << strSrcPath << endl;
    return 1;
  }

  if (MPLFileSys::FileExists(strDestPath))
  {
    if (!MPLFileSys::IsDirectory(strDestPath))
    {
      if (!MPLFileSys::FileDelete(strDestPath))
      {
        cout << "ERROR: can't delete file: " << strDestPath << endl;
        return 1;
      }
    }
  }

  if ((strBorderFilePath != "") && !MPLFileSys::FileExists(strBorderFilePath))
  {
    cout << "ERROR: can't open file: " << strBorderFilePath << endl;
    return 1;
  }

  if (strZooms == "")
  {
    nMinZoom = (nMaxZoom = -1);
  }
  else
  {
    std::regex oZoomsTemplate("\\d+-\\d+");
    if (!regex_match(strZooms, oZoomsTemplate))
    {
      cout << "ERROR: not valid value of \"-z\" parameter: " << strZooms << endl;
      return 1;
    }
    nMinZoom = atoi(strZooms.substr(0, strZooms.find("-")).data());
    nMaxZoom = atoi(strZooms.substr(strZooms.find("-") + 1, strZooms.size() - strZooms.find("-") - 1).data());
    if (nMinZoom>nMaxZoom) { int t; t = nMaxZoom; nMaxZoom = nMinZoom; nMinZoom = t; }

  }



  ttx::ITileContainer *poSrcTC = 0;
  ttx::TileType tile_type;
  ttx::MercatorProjType eMercType;
  ttx::Metadata *p_metadata = NULL;
  ttx::TileName         *poSrcTileName = NULL;
  if (!MPLFileSys::IsDirectory(strSrcPath))
  {
    poSrcTC = ttx::TileContainerFactory::OpenForReading(strSrcPath);

    if (!poSrcTC)
    {
      cout << "ERROR: can't open tile container: " << strSrcPath << endl;
      return 2;
    }

    tile_type = poSrcTC->GetTileType();
    eMercType = poSrcTC->GetProjType();
    p_metadata = poSrcTC->GetMetadata();

    cout << "Input container info: tile type=" << ttx::TileName::TileInfoByTileType(poSrcTC->GetTileType());
    cout << ", tiling srs=" << (poSrcTC->GetProjType() == ttx::WEB_MERCATOR) << endl;
  }
  else
  {
    if ((strTileType == "") && (strSrcTemplate != ""))
    {
      strTileType = ((strSrcTemplate != "") && (strSrcTemplate != "kosmosnimki") && (strSrcTemplate != "standard")) ?
        strSrcTemplate.substr(strSrcTemplate.rfind(".") + 1,
        strSrcTemplate.length() - strSrcTemplate.rfind(".") - 1) : "jpg";
    }
    if (strTileType == "")
    {
      cout << "ERROR: missing parameter \"-tt\"" << endl;
      return 1;
    }
    else if (!ttx::TileName::TileTypeByExtension(strTileType, tile_type))
    {
      cout << "ERROR: not valid tile type: " << strTileType << endl;
      return 1;
    }

    eMercType = ((strProjType == "0") || (strProjType == "world_mercator") || (strProjType == "epsg:3395")) ?
      ttx::WORLD_MERCATOR : ttx::WEB_MERCATOR;


    if (strSrcTemplate == "kosmosnimki")
      poSrcTileName = new ttx::KosmosnimkiTileName(strSrcPath, tile_type);
    else if ((strSrcTemplate == "standard") || (strSrcTemplate == ""))
      poSrcTileName = new ttx::StandardTileName(strSrcPath, ("{z}/{x}/{y}." + ttx::TileName::ExtensionByTileType(tile_type)));
    else if (ttx::ESRITileName::ValidateTemplate(strSrcTemplate))
      poSrcTileName = new ttx::ESRITileName(strSrcPath, strSrcTemplate);
    else if (ttx::StandardTileName::ValidateTemplate(strSrcTemplate))
      poSrcTileName = new ttx::StandardTileName(strSrcPath, strSrcTemplate);
    else
    {
      cout << "ERROR: not valid value of \"-i_tnt\" parameter: " << strSrcTemplate << endl;
      return 1;
    }
    poSrcTC = new ttx::TileFolder(poSrcTileName, eMercType, FALSE);

  }



  ttx::TileName           *poDestTileName = NULL;
  ttx::ITileContainer     *poDestTC = NULL;
  ttx::TileContainerType eDestTCType;

  list<pair<int, pair<int, int>>> tile_list;
  cout << "calculating number of tiles: ";
  fflush(stdout);
  cout << poSrcTC->GetTileList(tile_list, nMinZoom, nMaxZoom, strBorderFilePath) << endl;

  if (tile_list.size()==0) return 0;

 
  if (!ttx::TileContainerFactory::GetTileContainerType(strOutputFormat, eDestTCType))
  {
    cout << "ERROR: not valid value of \"-of\" parameter: " << strOutputFormat << endl;
    return 1;
  }
  if (eDestTCType == ttx::TileContainerType::TILEFOLDER) //TODO: should refactor
  {
    if (!MPLFileSys::FileExists(strDestPath))
    {
      if (!MPLFileSys::CreateDir(strDestPath.c_str()))
      {
        cout << "ERROR: can't create folder: " << strDestPath << endl;
        return 1;
      }
    }

    if (strDestTemplate == "kosmosnimki")
      poDestTileName = new ttx::KosmosnimkiTileName(strDestPath, tile_type);
    else if (ttx::ESRITileName::ValidateTemplate(strDestTemplate))
      poDestTileName = new ttx::ESRITileName(strDestPath, strDestTemplate);
    else if ((strDestTemplate == "standard") || ((strDestTemplate == "")))
      poDestTileName = new ttx::StandardTileName(strDestPath, ("{z}/{x}/{y}." + ttx::TileName::ExtensionByTileType(tile_type)));
    else if (ttx::StandardTileName::ValidateTemplate(strDestTemplate))
      poDestTileName = new ttx::StandardTileName(strDestPath, strDestTemplate);
    else
    {
      cout << "ERROR: not valid value of \"-o_tnt\" parameter: " << strDestTemplate << endl;
      return 1;
    }
    poDestTC = new ttx::TileFolder(poDestTileName, eMercType, FALSE);
  }
  else
  {
    ttx::TileContainerOptions oTCOptions;
    oTCOptions.path_ = strDestPath;
    oTCOptions.extra_options_ = strCreationOptions;
    oTCOptions.tile_type_ = poSrcTC->GetTileType();
    ttx::MercatorTileMatrixSet oMercTMS(eMercType);
    oTCOptions.p_matrix_set_ = &oMercTMS;
    int *panTileBounds = ttx::ITileContainer::GetTileBounds(&tile_list);
    int nSrcTilesMaxZoom = 0;
    for (int z = 0; z<32; z++)
    {
      if (panTileBounds[4 * z] != -1)
      {
        oTCOptions.tiling_srs_envp_ = oMercTMS.CalcEnvelopeByTileRange(z, panTileBounds[4 * z],
        panTileBounds[4 * z + 1],
        panTileBounds[4 * z + 2],
        panTileBounds[4 * z + 3]);
        nSrcTilesMaxZoom = z;
      }
    }
    oTCOptions.p_tile_bounds_=panTileBounds;

    oTCOptions.max_zoom_ = nMaxZoom != -1 ? nMaxZoom : nSrcTilesMaxZoom;

    if (!(poDestTC = ttx::TileContainerFactory::OpenForWriting(eDestTCType, &oTCOptions)))
    {
      cout << "ERROR: can't create output tile container: " << strDestPath << endl;
      return 2;
    }
  }


  cout << "coping tiles: 0% ";
  int tilesCopied = 0;
  for (auto iter : tile_list)
  {
    int z = iter.first;
    int x = iter.second.first;
    int y = iter.second.second;

    unsigned char* tileData = 0;
    unsigned int            tileSize = 0;
    if (poSrcTC->GetTile(z, x, y, tileData, tileSize))
    {
      if (poDestTC->AddTile(z, x, y, tileData, tileSize)) tilesCopied++;
    }
    delete[]tileData;

    TTXPrintTilingProgress(tile_list.size(), tilesCopied);
  }
  cout << " done." << endl;

  if (logFile) fclose(logFile);
  if (poDestTC) poDestTC->Close();
  delete(poSrcTC);
  delete(poSrcTileName);
  delete(poDestTileName);
  delete(poDestTC);


  return 0;
}