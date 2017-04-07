#include "stdafx.h"

int nDescriptors = 10;
const GMXOptionDescriptor asDescriptors[] =
{
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

int nExamples = 3;
const string astrUsageExamples[] =
{
  "copytiles -i tiles -i_tnt standard -tt png -of MBTiles -o tiles.mbtiles -z 10-15",
  "copytiles -i tiles -i_tnt {z}/{x}/{y}.png -of MBTiles -o tiles.mbtiles -b zone.shp",
  "copytiles -i tiles -i_tnt {z}/{x}/{y}.png -o tiles_new -o_tnt {z}_{x}_{y}.png"
};


int _tmain(int nArgs, wchar_t* argvW[])
{
  string *pastrArgs = new string[nArgs];
  for (int i = 0; i<nArgs; i++)
  {
    GMXString::wstrToUtf8(pastrArgs[i], argvW[i]);
    GMXString::ReplaceAll(pastrArgs[i], "\\", "/");
  }

  if (!GMXGDALLoader::Load(GMXFileSys::GetPath(pastrArgs[0]))) return 1;
  GDALAllRegister();
  OGRRegisterAll();

  //debug
  //GMXOptionParser::InitCmdLineArgsFromFile("../autotest/debug_input.txt", nArgs, pastrArgs);
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

  if (!GMXFileSys::FileExists(strSrcPath))
  {
    cout << "ERROR: can't find input path: " << strSrcPath << endl;
    return 1;
  }

  if (GMXFileSys::FileExists(strDestPath))
  {
    if (!GMXFileSys::IsDirectory(strDestPath))
    {
      if (!GMXFileSys::FileDelete(strDestPath))
      {
        cout << "ERROR: can't delete file: " << strDestPath << endl;
        return 1;
      }
    }
  }

  if ((strBorderFilePath != "") && !GMXFileSys::FileExists(strBorderFilePath))
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



  gmx::ITileContainer *poSrcTC = 0;
  gmx::TileType tile_type;
  gmx::MercatorProjType eMercType;
  gmx::Metadata *p_metadata = NULL;
  gmx::TileName         *poSrcTileName = NULL;
  if (!GMXFileSys::IsDirectory(strSrcPath))
  {
    poSrcTC = gmx::TileContainerFactory::OpenForReading(strSrcPath);

    if (!poSrcTC)
    {
      cout << "ERROR: can't open tile container: " << strSrcPath << endl;
      return 2;
    }

    tile_type = poSrcTC->GetTileType();
    eMercType = poSrcTC->GetProjType();
    p_metadata = poSrcTC->GetMetadata();

    cout << "Input container info: tile type=" << gmx::TileName::TileInfoByTileType(poSrcTC->GetTileType());
    cout << ", tiling srs=" << (poSrcTC->GetProjType() == gmx::WEB_MERCATOR) << endl;
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
    else if (!gmx::TileName::TileTypeByExtension(strTileType, tile_type))
    {
      cout << "ERROR: not valid tile type: " << strTileType << endl;
      return 1;
    }

    eMercType = ((strProjType == "0") || (strProjType == "world_mercator") || (strProjType == "epsg:3395")) ?
      gmx::WORLD_MERCATOR : gmx::WEB_MERCATOR;


    if (strSrcTemplate == "kosmosnimki")
      poSrcTileName = new gmx::KosmosnimkiTileName(strSrcPath, tile_type);
    else if ((strSrcTemplate == "standard") || (strSrcTemplate == ""))
      poSrcTileName = new gmx::StandardTileName(strSrcPath, ("{z}/{x}/{y}." + gmx::TileName::ExtensionByTileType(tile_type)));
    else if (gmx::ESRITileName::ValidateTemplate(strSrcTemplate))
      poSrcTileName = new gmx::ESRITileName(strSrcPath, strSrcTemplate);
    else if (gmx::StandardTileName::ValidateTemplate(strSrcTemplate))
      poSrcTileName = new gmx::StandardTileName(strSrcPath, strSrcTemplate);
    else
    {
      cout << "ERROR: not valid value of \"-i_tnt\" parameter: " << strSrcTemplate << endl;
      return 1;
    }
    poSrcTC = new gmx::TileFolder(poSrcTileName, eMercType, FALSE);

  }



  gmx::TileName           *poDestTileName = NULL;
  gmx::ITileContainer     *poDestTC = NULL;
  gmx::TileContainerType eDestTCType;
  if (!gmx::TileContainerFactory::GetTileContainerType(strOutputFormat, eDestTCType))
  {
    cout << "ERROR: not valid value of \"-of\" parameter: " << strOutputFormat << endl;
    return 1;
  }
  if (eDestTCType == gmx::TileContainerType::TILEFOLDER) //TODO: should refactor
  {
    if (!GMXFileSys::FileExists(strDestPath))
    {
      if (!GMXFileSys::CreateDir(strDestPath.c_str()))
      {
        cout << "ERROR: can't create folder: " << strDestPath << endl;
        return 1;
      }
    }

    if (strDestTemplate == "kosmosnimki")
      poDestTileName = new gmx::KosmosnimkiTileName(strDestPath, tile_type);
    else if (gmx::ESRITileName::ValidateTemplate(strDestTemplate))
      poDestTileName = new gmx::ESRITileName(strDestPath, strDestTemplate);
    else if ((strDestTemplate == "standard") || ((strDestTemplate == "")))
      poDestTileName = new gmx::StandardTileName(strDestPath, ("{z}/{x}/{y}." + gmx::TileName::ExtensionByTileType(tile_type)));
    else if (gmx::StandardTileName::ValidateTemplate(strDestTemplate))
      poDestTileName = new gmx::StandardTileName(strDestPath, strDestTemplate);
    else
    {
      cout << "ERROR: not valid value of \"-o_tnt\" parameter: " << strDestTemplate << endl;
      return 1;
    }
    poDestTC = new gmx::TileFolder(poDestTileName, eMercType, FALSE);
  }
  else
  {
    gmx::TileContainerOptions oTCOptions;
    oTCOptions.path_ = strDestPath;
    oTCOptions.max_zoom_ = nMaxZoom != -1 ? nMaxZoom : poSrcTC->GetMaxZoom();
    oTCOptions.extra_options_ = strCreationOptions;
    oTCOptions.tile_type_ = poSrcTC->GetTileType();
    gmx::MercatorTileMatrixSet oMercTMS(eMercType);
    oTCOptions.p_matrix_set_ = &oMercTMS;
    int panTileBounds[128];
    poSrcTC->GetTileBounds(panTileBounds);
    for (int z = 0; z<32; z++)
    {
      if (panTileBounds[4 * z] != -1)
        oTCOptions.tiling_srs_envp_ = oMercTMS.CalcEnvelopeByTileRange(z, panTileBounds[4 * z],
        panTileBounds[4 * z + 1],
        panTileBounds[4 * z + 2],
        panTileBounds[4 * z + 3]);
    }

    if (!(poDestTC = gmx::TileContainerFactory::OpenForWriting(eDestTCType, &oTCOptions)))
    {
      cout << "ERROR: can't create output tile container: " << strDestPath << endl;
      return 2;
    }
  }

  list<pair<int, pair<int, int>>> tile_list;
  cout << "calculating number of tiles: ";
  cout << poSrcTC->GetTileList(tile_list, nMinZoom, nMaxZoom, strBorderFilePath) << endl;

  if (tile_list.size()>0)
  {
    cout << "coping tiles: 0% ";
    int tilesCopied = 0;
    for (list<pair<int, pair<int, int>>>::iterator iter = tile_list.begin(); iter != tile_list.end(); iter++)
    {
      int z = (*iter).first;
      int x = (*iter).second.first;
      int y = (*iter).second.second;

      unsigned char* tileData = 0;
      unsigned int            tileSize = 0;
      if (poSrcTC->GetTile(z, x, y, tileData, tileSize))
      {
        if (poDestTC->AddTile(z, x, y, tileData, tileSize)) tilesCopied++;
      }
      delete[]tileData;

      GMXPrintTilingProgress(tile_list.size(), tilesCopied);
    }
  }
  if (logFile) fclose(logFile);
  if (poDestTC) poDestTC->Close();
  delete(poSrcTC);
  delete(poSrcTileName);
  delete(poDestTileName);
  delete(poDestTC);


  return 0;
}