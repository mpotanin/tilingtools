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
	GMX::FindFilesInFolderByPattern(inputFiles,strInput);
	if (inputFiles.size()==0)
	{
		cout<<"Error: can't find input files by path: "<<strInput<<endl;
		return -1;
	}

	//синициализируем обязательные параметры для тайлинга
	GMX::MercatorProjType mercType =	(	(strProjType == "") || (strProjType == "0") || (strProjType == "world_mercator")
											|| (strProjType == "epsg:3395")
										) ? GMX::WORLD_MERCATOR : GMX::WEB_MERCATOR;


	if ( (strContainer=="") && 
		 (strTemplate!="") && 
		 (strTemplate!="kosmosnimki") && 
		 (strTemplate!="standard") 
		)
	{
		if (!GMX::StandardTileName::validateTemplate(strTemplate)) return FALSE;
	}

	
	GMX::TileType tileType;
	if (strTileType=="")
	{
		if ((strTemplate!="") && (strTemplate!="kosmosnimki") && (strTemplate!="standard"))
			strTileType = strTemplate.substr(strTemplate.rfind(".")+1,strTemplate.length()-strTemplate.rfind(".")-1);
		else
			strTileType = "jpg";
	}
	
	tileType = ((strTileType == "") ||  (strTileType == "jpg") || (strTileType == "jpeg") || (strTileType == ".jpg")) ?
					GMX::JPEG_TILE : ((strTileType == "png") || (strTileType == ".png")) ? GMX::PNG_TILE : GMX::TIFF_TILE;
	
	GMXilingParameters oParams(strInput,mercType,tileType);

	if (strZoom != "")		oParams.baseZoom = (int)atof(strZoom.c_str());
	if (strMinZoom != "")	oParams.minZoom = (int)atof(strMinZoom.c_str());
	if (strVectorFile!="") oParams.vectorFile = strVectorFile;
	
	if (strContainer=="")
	{
		oParams.useContainer = FALSE;
		strOutput = (strOutput == "") ? (GMX::RemoveExtension(strInput)+"_tiles") : strOutput;
		if (!GMX::IsDirectory(strOutput))
		{
			if (!GMX::CreateDirectory(strOutput.c_str()))
			{
				cout<<"Error: can't create folder: "<<strOutput<<endl;
				return FALSE;
			}
		}
		if ((strTemplate=="") || (strTemplate=="kosmosnimki"))
			oParams.poTileName = new GMX::KosmosnimkiTileName(strOutput,tileType);
		else if (strTemplate=="standard") 
			oParams.poTileName = new GMX::StandardTileName(	strOutput,("{z}/{x}/{z}_{x}_{y}." + 
															GMX::TileName::tileExtension(tileType)));
		else 
			oParams.poTileName = new GMX::StandardTileName(strOutput,strTemplate);
	}
	else
	{
		oParams.useContainer	= TRUE;
		string containerExt	= (strContainer == "-container") ? "tiles" : "mbtiles"; 
		oParams.containerFile = (strOutput == "")	?
									GMX::RemoveExtension((*inputFiles.begin())) + "." + containerExt :
								(GMX::IsDirectory(strOutput)) ?
									GMX::RemoveEndingSlash(strOutput) + "/" + 
									GMX::RemoveExtension(GMX::RemovePath((*inputFiles.begin()))) + "." + containerExt :
								(GMX::GetExtension(strOutput) == containerExt) ?
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
		if (!GMX::ConvertStringToRGB(strTranspColor,rgb))
		{
			cout<<"Error: bad value of parameter: \"-no_data_rgb\""<<endl;
			return FALSE;
		}
		oParams.pTransparentColor = new BYTE[3];
		memcpy(oParams.pTransparentColor,rgb,3);
	}


	GMXMakeTiling(&oParams);
	//if (logFile) fclose(logFile);
	
	return 1;
}



int _tmain(int argc, wchar_t* argvW[])
{
	//cout.imbue(std::locale("rus_rus.866"));
	//locale myloc("");
	

	string *argv = new string[argc];
	for (int i=0;i<argc;i++)
	{
		GMX::wstrToUtf8(argv[i],argvW[i]);
		GMX::ReplaceAll(argv[i],"\\","/");
	}
	
	if (!GMX::LoadGDAL(argc,argv)) return -1;
	GDALAllRegister();
	OGRRegisterAll();

	if (argc == 1)
	{
		PrintHelp();
		return 0;
	}

  	//обязательный параметр
	string strInput				=  GMX::ReadConsoleParameter("-file",argc,argv);

	//дополнительные параметры
	string strMosaic			=  GMX::ReadConsoleParameter("-mosaic",argc,argv,TRUE);
	string	strContainer		=  (GMX::ReadConsoleParameter("-container",argc,argv,TRUE) != "") ? 
											GMX::ReadConsoleParameter("-container",argc,argv,TRUE) : 
											GMX::ReadConsoleParameter("-mbtiles",argc,argv,TRUE);
	string strZoom				=  GMX::ReadConsoleParameter("-zoom",argc,argv);
	string strMinZoom			=  GMX::ReadConsoleParameter("-minZoom",argc,argv);	
	string strVectorFile		=  GMX::ReadConsoleParameter("-border",argc,argv);
	string strOutput		=  GMX::ReadConsoleParameter("-tiles",argc,argv);
	string strTileType			=  GMX::ReadConsoleParameter("-tile_type",argc,argv);
	string strProjType			=  GMX::ReadConsoleParameter("-proj",argc,argv);
	string strTemplate			=  GMX::ReadConsoleParameter("-template",argc,argv);
	string strNoData			=  GMX::ReadConsoleParameter("-no_data",argc,argv);
	string strTranspColor		=  GMX::ReadConsoleParameter("-no_data_rgb",argc,argv);

	//скрытые параметры
	string strEdges				=  GMX::ReadConsoleParameter("-edges",argc,argv);
	string strShiftX			=  GMX::ReadConsoleParameter("-shiftX",argc,argv);
	string strShiftY			=  GMX::ReadConsoleParameter("-shiftY",argc,argv);
	string strPixelTiling		=  GMX::ReadConsoleParameter("-pixel_tiling",argc,argv,TRUE);
	string strBackground		=  GMX::ReadConsoleParameter("-background",argc,argv);
	string strLogFile			=  GMX::ReadConsoleParameter("-log_file",argc,argv);
	string strCache				=  GMX::ReadConsoleParameter("-cache",argc,argv);
	//string	strN

	if (argc == 2)				strInput	= argv[1];
	wcout<<endl;

	//проверяем входной файл(ы)

	//strInput		= "C:\\Work\\Projects\\TilingTools\\autotest\\scn_120719_Vrangel_island_SWA.tif";
	//strOutput		= "C:\\Work\\Projects\\TilingTools\\autotest\\result\\scn_120719_Vrangel_island_SWA_tiles";
	//strZoom			= "8";
	//strProjType		= "1";
	//strTemplate		= "standard";
	//strVectorFile	= "C:\\Work\\Projects\\TilingTools\\autotest\\border\\markers.tab";

	//-no_data_rgb "0 0 0" -tile_type png -border C:\Work\Projects\TilingTools\autotest\border\markers.tab


	//strInput		= "C:\\Work\\Projects\\TilingTools\\autotest\\dnb_land_ocean_ice.2012.54000x27000_geo_cut3.tif";
	//strOutput		= "C:\\Work\\Projects\\TilingTools\\autotest\\result\\scn_120719_Vrangel_island_SWA.tiles";
	//strZoom			= "6";
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


	if (strTileType	== "" ) strTileType = GMX::MakeLower( GMX::ReadConsoleParameter("-tileType",argc,argv));

	if (strMosaic=="")
	{
		std::list<string> input_files;
		if (!GMX::FindFilesInFolderByPattern (input_files,strInput))
		{
			cout<<"Can't find input files by pattern: "<<strInput<<endl;
			return 1;
		}


		for (std::list<string>::iterator iter = input_files.begin(); iter!=input_files.end();iter++)
		{
			cout<<"Tiling file: "<<(*iter)<<endl;
			if (input_files.size()>1)
				strVectorFile = GMX::VectorBorder::getVectorFileNameByRasterFileName(*iter);
			string strOutput_fix = strOutput;
			if ((input_files.size()>1)&&(strOutput!=""))
			{
				if (!GMX::FileExists(strOutput)) 
				{
					if (!GMX::CreateDirectory(strOutput.c_str()))
					{
						cout<<"Error: can't create directory: "<<strOutput<<endl;
						return -1;
					}
				}
				strOutput_fix =	GMX::RemoveEndingSlash(strOutput) + "/" + 
										GMX::RemovePath(GMX::RemoveExtension(*iter)) + "_tiles";
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

