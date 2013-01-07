// ImageTiling.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
using namespace GMT;


void PrintHelp ()
{

	cout<<"Usage:\n";
	cout<<"ImageTiling [-file input path] [-tiles output path] [-border vector border] [-zoom tiling zoom] [-minZoom pyramid min zoom]"; 
	cout<<" [-container write to geomixer container file] [-mbtiles write to MBTiles file] [-proj tiles projection (0 - World_Mercator, 1 - Web_Mercator)] [-tile_type jpg|png|tif] [-template tile name template]\n";
		
	cout<<"\nEx.1 - image to simple tiles:\n";
	cout<<"ImageTiling -file c:\\image.tif"<<endl;
	cout<<"(default values: -tiles=c:\\image_tiles -minZoom=1 -proj=0 -template=kosmosnimki -tile_type=jpg)"<<endl;
	
	cout<<"\nEx.2 - image to tiles packed into geomixer container:\n";
	cout<<"ImageTiling -file c:\\image.tif -container"<<endl;
	cout<<"(default values: -tiles=c:\\image.tiles -minZoom=1 -proj=0 -tile_type=jpg)"<<endl;

	cout<<"\nEx.3 - tiling folder of images:\n";
	cout<<"ImageTiling -file c:\\images\\*.tif -container -template {z}\\{x}\\{z}_{x}_{y}.jpg"<<endl;
	cout<<"(default values: -minZoom=1 -proj=0 -tile_type=jpg)"<<endl;

}

void Exit()
{
	cout<<"Tiling finished - press any key"<<endl;
	char c;
	cin>>c;
}


int CheckArgsAndCallTiling (	wstring strInput,
								wstring	strContainer,		
								wstring strZoom,
								wstring strMinZoom,
								wstring strVectorFile,
								wstring strTilesFolder,
								wstring strTileType,
								wstring strProjType,
								wstring strTemplate,
								wstring strNoData,
								wstring strTranspColor,
								wstring strEdges,
								wstring strShiftX,
								wstring strShiftY,
								wstring strPixelTiling,
								wstring strBackground,
								wstring strLogFile,
								wstring strCache)
{


	FILE *logFile = NULL;
	if (strLogFile!=L"")
	{
		if((logFile = _wfreopen(strLogFile.c_str(), L"w", stdout)) == NULL)
		{
			wcout<<L"Error: can't open log file: "<<strLogFile<<endl;
			exit(-1);
		}
	}

	//проверяем входной файл или директорию
	if (strInput == L"")
	{
		cout<<L"Error: missing \"-file\" parameter"<<endl;
		return 0;
	}
	

	list<wstring> inputFiles;
	FindFilesInFolderByPattern(inputFiles,strInput);
	if (inputFiles.size()==0)
	{
		cout<<L"Error: can't find input files by path: "<<strInput<<endl;
		return 0;
	}

	//синициализируем обязательные параметры для тайлинга
	MercatorProjType mercType = ((strProjType == L"") || (strProjType == L"0") || (strProjType == L"world_mercator")|| (strProjType == L"epsg:3395")) ?
								WORLD_MERCATOR : WEB_MERCATOR;


	if ( (strContainer==L"") && 
		 (strTemplate!=L"") && 
		 (strTemplate!=L"kosmosnimki") && 
		 (strTemplate!=L"standard") 
		)
	{
		if (!StandardTileName::validateTemplate(strTemplate)) return FALSE;
	}

	
	TileType tileType;
	if (strTileType==L"")
	{
		if ((strTemplate!=L"") && (strTemplate!=L"kosmosnimki") && (strTemplate!=L"standard"))
			strTileType = strTemplate.substr(strTemplate.rfind(L".")+1,strTemplate.length()-strTemplate.rfind(L".")-1);
		else
			strTileType = L"jpg";
	}
	
	tileType = ((strTileType == L"") ||  (strTileType == L"jpg") || (strTileType == L"jpeg") || (strTileType == L".jpg")) ?
					JPEG_TILE : ((strTileType == L"png") || (strTileType == L".png")) ? PNG_TILE : TIFF_TILE;
	
	GMTilingParameters oParams(strInput,mercType,tileType);

	if (strZoom != L"")		oParams.baseZoom = (int)_wtof(strZoom.c_str());
	if (strMinZoom != L"")	oParams.minZoom = (int)_wtof(strMinZoom.c_str());
	if (strVectorFile!=L"") oParams.vectorFile = strVectorFile;
	
	if (strContainer==L"")
	{
		oParams.useContainer = FALSE;
		strTilesFolder = (strTilesFolder == L"") ? (RemoveExtension(strInput)+L"_tiles") : strTilesFolder;
		if (!IsDirectory(strTilesFolder))
		{
			if (!CreateDirectory(strTilesFolder.c_str(),NULL))
			{
				wcout<<L"Error: can't create folder: "<<strTilesFolder<<endl;
				return FALSE;
			}
		}
		if ((strTemplate==L"") || (strTemplate==L"kosmosnimki"))
			oParams.poTileName = new KosmosnimkiTileName(strTilesFolder,tileType);
		else if (strTemplate==L"standard") 
			oParams.poTileName = new StandardTileName(strTilesFolder,(L"{z}\\{x}\\{z}_{x}_{y}."+TileName::tileExtension(tileType)));
		else 
			oParams.poTileName = new StandardTileName(strTilesFolder,strTemplate);
	}
	else
	{
		oParams.useContainer	= TRUE;
		wstring containerExt	= (strContainer == L"-container") ? L"tiles" : L"mbtiles"; 
		oParams.containerFile = (strTilesFolder == L"")	?
									RemoveExtension((*inputFiles.begin())) + L"." + containerExt :
								(IsDirectory(strTilesFolder)) ?
									RemoveEndingSlash(strTilesFolder) + L"\\" + 
									RemoveExtension(RemovePath((*inputFiles.begin()))) + L"." + containerExt :
								(GetExtension(strTilesFolder) == containerExt) ?
									strTilesFolder :
									strTilesFolder + L"." + containerExt;
	}

	if ((strShiftX!=L"")) oParams.dShiftX=_wtof(strShiftX.c_str());
	if ((strShiftY!=L"")) oParams.dShiftY=_wtof(strShiftY.c_str());

	if (strCache!=L"") oParams.maxTilesInCache = _wtof(strCache.c_str());

	if (strNoData!=L"")
	{
		oParams.pNoDataValue = new int[1];
		oParams.pNoDataValue[0] = _wtof(strNoData.c_str());
	}

	if (strTranspColor!=L"")
	{
		BYTE	rgb[3];
		if (!ConvertStringToRGB(strTranspColor,rgb))
		{
			wcout<<L"Error: bad value of parameter: \"-no_data_rgb\""<<endl;
			return FALSE;
		}
		oParams.pTransparentColor = new BYTE[3];
		memcpy(oParams.pTransparentColor,rgb,3);
	}


	GMTMakeTiling(&oParams);
	if (logFile) fclose(logFile);
	
	return 1;
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (!LoadGDAL(argc,argv)) return 0;
	GDALAllRegister();
	OGRRegisterAll();

	if (argc == 1)
	{
		PrintHelp();
		return 0;
	}

  	//обязательный параметр
	wstring strInput			=  ReadConsoleParameter(L"-file",argc,argv);

	//дополнительные параметры
	wstring strMosaic			=  ReadConsoleParameter(L"-mosaic",argc,argv,TRUE);
	wstring	strContainer		=  (ReadConsoleParameter(L"-container",argc,argv,TRUE) != L"") ? 
											ReadConsoleParameter(L"-container",argc,argv,TRUE) : 
											ReadConsoleParameter(L"-mbtiles",argc,argv,TRUE);
	wstring strZoom				=  ReadConsoleParameter(L"-zoom",argc,argv);
	wstring strMinZoom			=  ReadConsoleParameter(L"-minZoom",argc,argv);	
	wstring strVectorFile		=  ReadConsoleParameter(L"-border",argc,argv);
	wstring strTilesFolder		=  ReadConsoleParameter(L"-tiles",argc,argv);
	wstring strTileType			=  ReadConsoleParameter(L"-tile_type",argc,argv);
	wstring strProjType			=  ReadConsoleParameter(L"-proj",argc,argv);
	wstring strTemplate			=  ReadConsoleParameter(L"-template",argc,argv);
	wstring strNoData			=  ReadConsoleParameter(L"-no_data",argc,argv);
	wstring strTranspColor		=  ReadConsoleParameter(L"-no_data_rgb",argc,argv);

	//скрытые параметры
	wstring strEdges			=  ReadConsoleParameter(L"-edges",argc,argv);
	wstring strShiftX			=  ReadConsoleParameter(L"-shiftX",argc,argv);
	wstring strShiftY			=  ReadConsoleParameter(L"-shiftY",argc,argv);
	wstring strPixelTiling		=  ReadConsoleParameter(L"-pixel_tiling",argc,argv,TRUE);
	wstring strBackground		=  ReadConsoleParameter(L"-background",argc,argv);
	wstring strLogFile			=  ReadConsoleParameter(L"-log_file",argc,argv);
	wstring strCache			=  ReadConsoleParameter(L"-cache",argc,argv);
	//wstring	strN

	if (argc == 2)				strInput	= argv[1];
	wcout<<endl;

	//проверяем входной файл(ы)

	/*
	strInput		= L"C:\\Work\\Projects\\TilingTools\\autotest\\o42073g8.tif";
	strVectorFile	= L"C:\\Work\\Projects\\TilingTools\\autotest\\border_o42073g8\\border.shp";
	strMinZoom		= L"8";
	strTileType	= L"png";
	*/
	
	if (strInput == L"")
	{
		cout<<L"Error: missing \"-file\" parameter"<<endl;
		return 0;
	}


	if (strTileType	== L"" ) strTileType = MakeLower( ReadConsoleParameter(L"-tileType",argc,argv));

	if (strMosaic==L"")
	{
		std::list<wstring> input_files;
		if (!FindFilesInFolderByPattern (input_files,strInput))
		{
			wcout<<L"Can't find input files by pattern: "<<strInput<<endl;
			return 1;
		}


		for (std::list<wstring>::iterator iter = input_files.begin(); iter!=input_files.end();iter++)
		{
			wcout<<L"Tiling file: "<<(*iter)<<endl;
			if (input_files.size()>1)
				strVectorFile = VectorBorder::getVectorFileNameByRasterFileName(*iter);
		
			CheckArgsAndCallTiling (	(*iter),
										strContainer,		
										strZoom,
										strMinZoom,
										strVectorFile,
										strTilesFolder,
										strTileType,
										strProjType,
										strTemplate,
										strNoData,
										strTranspColor,
										strEdges,
										strShiftX,
										strShiftY,
										strPixelTiling,
										strBackground,
										strLogFile,
										strCache);
			wcout<<endl;
		}
	}
	else
	{
		CheckArgsAndCallTiling (	strInput,
									strContainer,		
									strZoom,
									strMinZoom,
									strVectorFile,
									strTilesFolder,
									strTileType,
									strProjType,
									strTemplate,
									strNoData,
									strTranspColor,
									strEdges,
									strShiftX,
									strShiftY,
									strPixelTiling,
									strBackground,
									strLogFile,
									strCache);
	}

	return 0;
	
}

