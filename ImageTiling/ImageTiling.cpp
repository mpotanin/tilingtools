// ImageTiling.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


void PrintHelp ()
{

	cout<<"Usage:\n";
	cout<<"ImageTiling [-file input path] [-tiles output path] [-border vector border] [-zoom tiling zoom] [-minZoom pyramid min tiling zoom]"; 
	cout<<" [-container write to geomixer container file] [-mbtiles write to MBTiles file] [-proj tiles projection (0 - World_Mercator, 1 - Web_Mercator)] [-tile_type jpg|png|tif] [-template tile name template] [-no_data_rgb transparent color for png tiles]\n";
		
	cout<<"\nEx.1 - image to simple tiles:\n";
	cout<<"ImageTiling -file c:\\image.tif"<<endl;
	cout<<"(default values: -tiles=c:\\image_tiles -minZoom=1 -proj=0 -template=kosmosnimki -tile_type=jpg)"<<endl;
	
	cout<<"\nEx.2 - image to tiles packed into geomixer container:\n";
	cout<<"ImageTiling -file c:\\image.tif -container"<<endl;
	cout<<"(default values: -tiles=c:\\image.tiles -minZoom=1 -proj=0 -tile_type=jpg)"<<endl;

	cout<<"\nEx.3 - tiling folder of images:\n";
	cout<<"ImageTiling -file c:\\images\\*.tif -container -template {z}/{x}/{z}_{x}_{y}.png  -tile_type png -no_data_rgb \"0 0 0\""<<endl;
	cout<<"(default values: -minZoom=1 -proj=0)"<<endl;

}

void Exit()
{
	cout<<"Tiling finished - press any key"<<endl;
	char c;
	cin>>c;
}


int CheckArgsAndCallTiling (	string strInput,
								string	strContainer,		
								string strZoom,
								string strMinZoom,
								string strVectorFile,
								string strOutput,
								string strTileType,
								string strProjType,
								string strTemplate,
								string strNoData,
								string strTranspColor,
								string strEdges,
								string strShiftX,
								string strShiftY,
								string strPixelTiling,
								string strBackground,
								string strLogFile,
								string strCache)
{


	/*
	FILE *logFile = NULL;
	if (strLogFile!="")
	{
		if((logFile = _wfreopen(strLogFile.c_str(), "w", stdout)) == NULL)
		{
			wcout<<"Error: can't open log file: "<<strLogFile<<endl;
			exit(-1);
		}
	}
	*/
	//проверяем входной файл или директорию
	if (strInput == "")
	{
		cout<<"Error: missing \"-file\" parameter"<<endl;
		return -1;
	}
	

	list<string> inputFiles;
	GMT::FindFilesInFolderByPattern(inputFiles,strInput);
	if (inputFiles.size()==0)
	{
		cout<<"Error: can't find input files by path: "<<strInput<<endl;
		return -1;
	}

	//синициализируем обязательные параметры для тайлинга
	GMT::MercatorProjType mercType =	(	(strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")
											|| (strProjType == "epsg:3395")
										) ? GMT::WORLD_MERCATOR : GMT::WEB_MERCATOR;


	if ( (strContainer=="") && 
		 (strTemplate!="") && 
		 (strTemplate!="kosmosnimki") && 
		 (strTemplate!="standard") 
		)
	{
		if (!GMT::StandardTileName::validateTemplate(strTemplate)) return FALSE;
	}

	
	GMT::TileType tileType;
	if (strTileType=="")
	{
		if ((strTemplate!="") && (strTemplate!="kosmosnimki") && (strTemplate!="standard"))
			strTileType = strTemplate.substr(strTemplate.rfind(".")+1,strTemplate.length()-strTemplate.rfind(".")-1);
		else
			strTileType = "jpg";
	}
	
	tileType = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					GMT::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? GMT::PNG_TILE : GMT::TIFF_TILE;
	
	GMTilingParameters oParams(strInput,mercType,tileType);

	if (strZoom != "")		oParams.baseZoom = (int)atof(strZoom.c_str());
	if (strMinZoom != "")	oParams.minZoom = (int)atof(strMinZoom.c_str());
	if (strVectorFile!="") oParams.vectorFile = strVectorFile;
	
	if (strContainer=="")
	{
		oParams.useContainer = FALSE;
		strOutput = (strOutput == "") ? (GMT::RemoveExtension(strInput)+"_tiles") : strOutput;
		if (!GMT::IsDirectory(strOutput))
		{
			if (!GMT::CreateDirectory(strOutput.c_str()))
			{
				cout<<"Error: can't create folder: "<<strOutput<<endl;
				return FALSE;
			}
		}
		if ((strTemplate=="") || (strTemplate=="kosmosnimki"))
			oParams.poTileName = new GMT::KosmosnimkiTileName(strOutput,tileType);
		else if (strTemplate=="standard") 
			oParams.poTileName = new GMT::StandardTileName(	strOutput,("{z}/{x}/{z}_{x}_{y}." + 
															GMT::TileName::tileExtension(tileType)));
		else 
			oParams.poTileName = new GMT::StandardTileName(strOutput,strTemplate);
	}
	else
	{
		oParams.useContainer	= TRUE;
		string containerExt	= (strContainer == "-container") ? "tiles" : "mbtiles"; 
		oParams.containerFile = (strOutput == "")	?
									GMT::RemoveExtension((*inputFiles.begin())) + "." + containerExt :
								(GMT::IsDirectory(strOutput)) ?
									GMT::RemoveEndingSlash(strOutput) + "/" + 
									GMT::RemoveExtension(GMT::RemovePath((*inputFiles.begin()))) + "." + containerExt :
								(GMT::GetExtension(strOutput) == containerExt) ?
									strOutput :
									strOutput + "." + containerExt;
	}

	if ((strShiftX!="")) oParams.dShiftX=atof(strShiftX.c_str());
	if ((strShiftY!="")) oParams.dShiftY=atof(strShiftY.c_str());

	if (strCache!="") oParams.maxTilesInCache = atof(strCache.c_str());

	if (strNoData!="")
	{
		oParams.pNoDataValue = new int[1];
		oParams.pNoDataValue[0] = atof(strNoData.c_str());
	}

	if (strTranspColor!="")
	{
		BYTE	rgb[3];
		if (!GMT::ConvertStringToRGB(strTranspColor,rgb))
		{
			cout<<"Error: bad value of parameter: \"-no_data_rgb\""<<endl;
			return FALSE;
		}
		oParams.pTransparentColor = new BYTE[3];
		memcpy(oParams.pTransparentColor,rgb,3);
	}


	GMTMakeTiling(&oParams);
	//if (logFile) fclose(logFile);
	
	return 1;
}



int _tmain(int argc, wchar_t* argvW[])
{
	cout.imbue(std::locale("rus_rus.866"));
	string *argv = new string[argc];
	for (int i=0;i<argc;i++)
	{
		GMT::wstrToUtf8(argv[i],argvW[i]);
		GMT::ReplaceAll(argv[i],"\\","/");
	}
	
	if (!GMT::LoadGDAL(argc,argv)) return -1;
	GDALAllRegister();
	OGRRegisterAll();

	if (argc == 1)
	{
		PrintHelp();
		return 0;
	}

  	//обязательный параметр
	string strInput				=  GMT::ReadConsoleParameter("-file",argc,argv);

	//дополнительные параметры
	string strMosaic			=  GMT::ReadConsoleParameter("-mosaic",argc,argv,TRUE);
	string	strContainer		=  (GMT::ReadConsoleParameter("-container",argc,argv,TRUE) != "") ? 
											GMT::ReadConsoleParameter("-container",argc,argv,TRUE) : 
											GMT::ReadConsoleParameter("-mbtiles",argc,argv,TRUE);
	string strZoom				=  GMT::ReadConsoleParameter("-zoom",argc,argv);
	string strMinZoom			=  GMT::ReadConsoleParameter("-minZoom",argc,argv);	
	string strVectorFile		=  GMT::ReadConsoleParameter("-border",argc,argv);
	string strOutput		=  GMT::ReadConsoleParameter("-tiles",argc,argv);
	string strTileType			=  GMT::ReadConsoleParameter("-tile_type",argc,argv);
	string strProjType			=  GMT::ReadConsoleParameter("-proj",argc,argv);
	string strTemplate			=  GMT::ReadConsoleParameter("-template",argc,argv);
	string strNoData			=  GMT::ReadConsoleParameter("-no_data",argc,argv);
	string strTranspColor		=  GMT::ReadConsoleParameter("-no_data_rgb",argc,argv);

	//скрытые параметры
	string strEdges				=  GMT::ReadConsoleParameter("-edges",argc,argv);
	string strShiftX			=  GMT::ReadConsoleParameter("-shiftX",argc,argv);
	string strShiftY			=  GMT::ReadConsoleParameter("-shiftY",argc,argv);
	string strPixelTiling		=  GMT::ReadConsoleParameter("-pixel_tiling",argc,argv,TRUE);
	string strBackground		=  GMT::ReadConsoleParameter("-background",argc,argv);
	string strLogFile			=  GMT::ReadConsoleParameter("-log_file",argc,argv);
	string strCache				=  GMT::ReadConsoleParameter("-cache",argc,argv);
	//string	strN

	if (argc == 2)				strInput	= argv[1];
	wcout<<endl;

	//проверяем входной файл(ы)


	//strInput		= "C:\\Work\\Projects\\TilingTools\\autotest\\scn_120719_Vrangel_island_SWA.tif";
	//strOutput		= "C:\\Work\\Projects\\TilingTools\\autotest\\result\\scn_120719_Vrangel_island_SWA.tiles";
	//strZoom			= "8";
	//strContainer	= "-container";



	/*
	strInput		= "C:\\Work\\Projects\\TilingTools\\autotest\\o42073g8.tif";
	strVectorFile	= "C:\\Work\\Projects\\TilingTools\\autotest\\border_o42073g8\\border.shp";
	strMinZoom		= "8";
	strTileType	= "png";
	*/
	
	if (strInput == "")
	{
		cout<<"Error: missing \"-file\" parameter"<<endl;
		return -1;
	}


	if (strTileType	== "" ) strTileType = GMT::MakeLower( GMT::ReadConsoleParameter("-tileType",argc,argv));

	if (strMosaic=="")
	{
		std::list<string> input_files;
		if (!GMT::FindFilesInFolderByPattern (input_files,strInput))
		{
			cout<<"Can't find input files by pattern: "<<strInput<<endl;
			return 1;
		}


		for (std::list<string>::iterator iter = input_files.begin(); iter!=input_files.end();iter++)
		{
			cout<<"Tiling file: "<<(*iter)<<endl;
			if (input_files.size()>1)
				strVectorFile = GMT::VectorBorder::getVectorFileNameByRasterFileName(*iter);
			string strOutput_fix = strOutput;
			if ((input_files.size()>1)&&(strOutput!=""))
			{
				if (!GMT::FileExists(strOutput)) 
				{
					if (!GMT::CreateDirectory(strOutput.c_str()))
					{
						cout<<"Error: can't create directory: "<<strOutput<<endl;
						return -1;
					}
				}
				strOutput_fix =	GMT::RemoveEndingSlash(strOutput) + "/" + 
										GMT::RemovePath(GMT::RemoveExtension(*iter)) + "_tiles";
			}

			CheckArgsAndCallTiling (	(*iter),
										strContainer,		
										strZoom,
										strMinZoom,
										strVectorFile,
										strOutput_fix,
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
									strOutput,
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

