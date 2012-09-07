// ImageTiling.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void PrintHelp ()
{

	cout<<"Usage:\n";
	cout<<"ImageTiling [-file input path] [-tiles output path] [-border vector border] [-zoom tiling zoom] [-minZoom pyramid min zoom]"; 
	cout<<" [-container write to container file] [-proj tiles projection (0 - World_Mercator, 1 - Web_Mercator)] [-tileType jpg|png|tif] [-template tile name template]\n";
		
	cout<<"\nEx.1 - image to simple tiles:\n";
	cout<<"ImageTiling -file c:\\image.tif"<<endl;
	cout<<"(default values: -tiles=c:\\image_tiles -minZoom=1 -proj=0 -template=kosmosnimki -tileType=jpg)"<<endl;
	
	cout<<"\nEx.2 - image to tiles packed in container:\n";
	cout<<"ImageTiling -file c:\\image.tif -container"<<endl;
	cout<<"(default values: -tiles=c:\\image.tiles -minZoom=1 -proj=0 -tileType=jpg)"<<endl;

	cout<<"\nEx.3 - image to tiles to simple tiles:\n";
	cout<<"ImageTiling -file c:\\image.tif -container -template {z}\\{x}\\{z}_{x}_{y}.jpg"<<endl;
	cout<<"(default values: -tiles=c:\\image.tiles -minZoom=1 -proj=0 -tileType=jpg)"<<endl;

}

void Exit()
{
	cout<<"Tiling finished - press any key"<<endl;
	char c;
	cin>>c;
}



int _tmain(int argc, _TCHAR* argv[])
{


	if (!LoadGdal(argc,argv)) return 0;
	GDALAllRegister();
	OGRRegisterAll();

	if (argc == 1)
	{
		PrintHelp();
		return 0;
	}

  
  	//обязательный параметр
	wstring strInput			= ReadParameter(L"-file",argc,argv);

	//дополнительные параметры
	wstring strBundle			= ReadParameter(L"-bundle",argc,argv,TRUE);
	wstring	strContainer		= ReadParameter(L"-container",argc,argv,TRUE);
	wstring strZoom				= ReadParameter(L"-zoom",argc,argv);
	wstring strMinZoom			= ReadParameter(L"-minZoom",argc,argv);	
	wstring strVectorFile		= ReadParameter(L"-border",argc,argv);
	wstring strTilesFolder		= ReadParameter(L"-tiles",argc,argv);
	wstring strTileType			= MakeLower(ReadParameter(L"-tileType",argc,argv));
	wstring strProjType			= MakeLower(ReadParameter(L"-proj",argc,argv));
	wstring strTemplate			= MakeLower(ReadParameter(L"-template",argc,argv));



	//скрытые параметры
	wstring strEdges			= ReadParameter(L"-edges",argc,argv);
	wstring strShiftX			= ReadParameter(L"-shiftX",argc,argv);
	wstring strShiftY			= ReadParameter(L"-shiftY",argc,argv);
	wstring strPixelTiling		= ReadParameter(L"-pixel_tiling",argc,argv,TRUE);
	wstring strBackground		= ReadParameter(L"-background",argc,argv);
	wstring strLogFile			= ReadParameter(L"-log_file",argc,argv);
	wstring strCache			= ReadParameter(L"-cache",argc,argv);

	if (argc == 2)				strInput	= argv[1];
	


	 
	FILE *logFile = NULL;
	if (strLogFile!=L"")
	{
		if((logFile = _wfreopen(strLogFile.c_str(), L"w", stdout)) == NULL)
		{
			wcout<<L"Error: can't open log file: "<<strLogFile<<endl;
			exit(-1);
		}
	}



	//strZoom		= L"14";
	//strMinZoom	= L"11";
	//strInput		= L"C:\\Work\\Projects\\TilingTools\\autotest\\scn_120719_Vrangel_island_SWA.tif";
	//strTilesFolder	= L"C:\\Work\\Projects\\TilingTools\\autotest\\result\\scn_120719_Vrangel_island_SWA_tiles";
	//strZoom			= L"8";
	//strProjType		= L"1";
	//strTemplate		= L"standard";
	//strContainer	= L"-container";
	//strInput	= L"C:\\ik_po_426174_0050002_ch1-3_8bit_utm43_x4.tif";
	//strInput	= L"C:\\ik_po_426174_0050002_ch1-3_8bit_merc.tif";
	//strInput	= L"C:\\Work\\users\\Anton\\scn_120719_Vrangel_island_SWA.tif";
	//strInput	= L"C:\\Users\\mpotanin\\Downloads\\Arctic_r06c03.2012193.terra.250m\\Arctic_r06c03.2012193.terra.250m.jpg";


	//strInput		= L"C:\\Work\\Projects\\AllRelease\\AutoTest\\ik_po_426174_0050002_ch1-3_8bit_utm43.tif";
	//strVectorFile	= L"C:\\Work\\Projects\\AllRelease\\AutoTest\\ik_po_426174_0050002_ch1-3_8bit_utm43_cut.shp";
	//strContainer	= L"container";
	//strTileType	= L"png";
	//strProjType		= L"1";


	//проверяем входной файл(ы)
	if (strInput == L"")
	{
		cout<<L"Error: missing \"-file\" parameter"<<endl;
		return 0;
	}
	list<wstring> inputFiles;
	int n = strInput.find(L'*');
	if (n <0) 
	{
		if (!FileExists(strInput))
		{
			cout<<L"Error: can't open input file: "<<strInput<<endl;
			return 0;
		}
	}
	else
	{
		FindFilesInFolderByPattern(inputFiles,strInput);
		if (inputFiles.size()==0)
		{
			cout<<L"Error: can't find input files by pattern: "<<strInput<<endl;
			return 0;
		}
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
	
	TilingParameters oParams(strInput,mercType,tileType);

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
		oParams.useContainer = TRUE;
		oParams.containerFile = (strTilesFolder==L"") ? (RemoveExtension(strInput)+L".tiles") : strTilesFolder;
	}

	if ((strShiftX!=L"")) oParams.dShiftX=_wtof(strShiftX.c_str());
	if ((strShiftY!=L"")) oParams.dShiftY=_wtof(strShiftY.c_str());

	if (strCache!=L"") oParams.maxTilesInCache = _wtof(strCache.c_str());


	MakeTiling(oParams);

	if (logFile) fclose(logFile);
	
	//*/


	///*
	//strImageFile = "I:\\GeoMixerTools\\test_data\\image_cut_merc.img";
	//strBundle = "-bundle";
	//strImagesFolder = "I:\\GeoMixerTools\\test_data\\Spot";
	//strVectorFile	= "\\\\192.168.4.28\\spot10-khabar_evr\\SPOT10\\VECTOR\\XABAROVSK.mif";
	//strImagesType	= "jpg";
	//strTilesFolder = "I:\\temp\\khabarovsk_tiles";
	//strVectorFile = "I:\\GeoMixerTools\\test_data\\image_cut.mif";
	//*/

	/*
	if ((strBundle!=L"") || (strImagesFolder==L""))
	{
		CheckParamsAndMakeTiling(strImagesFolder,strImageFile,strVectorFile,strContainer,strTilesFolder,strZooms,strMinZoom,strEdges,strShiftX,strShiftY,strImagesType,strBackground);
	}
	else
	{
		if (!FileExists(strImagesFolder))
		{
			wcout<<L"Error: folder: "<<strImagesFolder<<L" doesn't exist"<<endl;
			return 0;
		}
				
		if (strImagesType==L"") strImagesType = L".tif";
		list<wstring> strFilesList;

		if (!FindAllFilesInFolderByExtension(strFilesList,strImagesFolder,strImagesType))
		{
			wcout<<L"Error: no images of type "<<strImagesType<<L" in the folder "<<strImagesFolder<<endl;
			return 0;
		}
		
		if (strFilesList.size()==0)
		{
			wcout<<L"Error: no images of type "<<strImagesType<<L" in the folder "<<strImagesFolder<<endl;
			return 0;
		}
		
		for (list<wstring>::iterator iter = strFilesList.begin();iter!=strFilesList.end();iter++)
		{
			wcout<<(*iter)<<endl;
			CheckParamsAndMakeTiling(L"",(*iter),strVectorFile,strContainer,strTilesFolder,strZooms,strMinZoom,strEdges,strShiftX,strShiftY,strImagesType,strBackground);
			wcout<<endl<<endl;
		}
	}
	*/
	
	return 0;
}

