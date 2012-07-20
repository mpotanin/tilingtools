// CopyTiles.cpp : Defines the entry point for the console application.
//

// CopyTiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	if (!LoadGdal(argc,argv)) return 0;
	GDALAllRegister();



	if (argc == 1)
	{
		wcout<<L"Usage: "<<endl;
		wcout<<L"CopyTiles [-delete] [-from FromTilesFolder] [-to ToTilesFolder] [-border VectorBorderFile] [-type (jpg|png)][-zooms MinZoom-MaxZoom]"<<endl;
		wcout<<L"Example 1 (copy tiles, default: type=jpg ):"<<endl;
		wcout<<L"CopyTiles -from c:\\all_tiles -to c:\\moscow_reg_tiles -border c:\\moscow_reg.shp -zooms 6-14"<<endl;
		wcout<<endl;
		wcout<<L"Example 2 (delete tiles):"<<endl;
		wcout<<L"CopyTiles -delete -from c:\\all_tiles -border c:\\moscow_reg.shp -type png -zooms 6 14"<<endl;

		return 0;
	}

	wstring	strFromTilesFolder;
	wstring	strToTilesFolder;
	wstring	strBorder;
	wstring	strZooms;
	int		nMinZoom = 0;
	int		nMaxZoom;
	wstring strTilesType;
	//bool bDeleteTiles;


	/*
	strFromTilesFolder	= "D:\\GeoMixerTools\\test_data\\image_cut_merc_tiles";
	strToTilesFolder	= "D:\\GeoMixerTools\\test_data\\copy";
	strBorder			= "D:\\GeoMixerTools\\test_data\\image_cut_merc.mif";
	strZooms			= "14";
	bDeleteTiles		= false;
	//strTilesType		= "png";
	*/

	///*
	BOOL	bDeleteTiles = (ReadParameter(L"-delete",argc,argv,TRUE)!=L"");

	strFromTilesFolder	= ReadParameter(L"-from",argc,argv);
	strToTilesFolder	= ReadParameter(L"-to",argc,argv);
	strBorder			= ReadParameter(L"-border",argc,argv);
	strZooms			= ReadParameter(L"-zooms",argc,argv);
	strTilesType		= ReadParameter(L"-type",argc,argv);
	//*/

	if (strFromTilesFolder == L"")
	{
		wcout<<L"Error: missing \"-from\" parameter"<<endl;
		return 0;
	}

	if ((strToTilesFolder == L"")&&(!bDeleteTiles))
	{
		wcout<<L"Error: missing \"-to\" parameter"<<endl;
		return 0;
	}

	if (strBorder == L"")
	{
		wcout<<L"Error: missing \"-border\" parameter"<<endl;
		return 0;
	}

	if (strZooms == L"")
	{
		wcout<<L"Error: missing \"-zooms\" parameter"<<endl;
		return 0;
	}

	if (!FileExists(strFromTilesFolder))
	{
		wcout<<L"Error: can't find folder: "<<strFromTilesFolder<<endl;
		return 0;
	}
	
	if (strToTilesFolder!=L"")
	{
		if (!FileExists(strToTilesFolder))
		{
			if (!CreateDirectory(strToTilesFolder.c_str(),NULL))
			{
				wcout<<L"Error: can't create folder: "<<strToTilesFolder<<endl;
				return 0;
			}
		}
	}
	
	
	if (!FileExists(strBorder))
	{
		wcout<<L"Error: can't find file: "<<strBorder<<endl;
		return 0;
	}
	
	if (strZooms==L"")
	{
		wcout<<L"Error: you must specify min-max zoom levels"<<endl;
		return 0;
	}
	else
	{
		if (strZooms.find(L"_")>=0)
		{
			nMinZoom = (int)_wtof(strZooms.substr(0,strZooms.find(L"-")).data());
			nMaxZoom = (int)_wtof(strZooms.substr(strZooms.find(L"-")+1,strZooms.size()-strZooms.find(L"-")-1).data());
			if (nMinZoom>nMaxZoom) {int t; t=nMaxZoom; nMaxZoom = nMinZoom; nMinZoom = t;}
		}
	}
	
	/*
		
	for (int nZoom = nMinZoom; nZoom<=nMaxZoom;nZoom++)
	{
		wcout<<L"Zoom "<<nZoom<<L": ";//endl;
		list<wstring> oTilesList;
		wcout<<L"calculating number of tiles: ";
	
		VectorBorder	oVectorBorder;
		if (!VectorFile::OpenAndCreatePolygonInMercator(strBorder,oVectorBorder))
		{
			wcout<<L"Error: can't read border polygon: "<<endl;
			return 0;
		}

		//
		//FindTilesByVectorBorder(new KosmosnimkiTileName(strFromTilesFolder,strTilesType),nZoom,false,oVectorBorder,oTilesList,strTilesType);
		
		FindTilesByVectorBorder(new StandardTileName(strFromTilesFolder,strTilesType),nZoom,false,oVectorBorder,oTilesList,strTilesType);
		wcout<<oTilesList.size()<<L"  ";
		int nTiles = 0;
		int nBadTiles = 0;
		if (oTilesList.size()!=0)
		{
			if (!bDeleteTiles) wcout<<L"copying tiles: ";
			else if (strToTilesFolder!=L"") wcout<<L"moving tiles: ";
			else wcout<<L"deleting tiles: ";
			for (list<wstring>::iterator iter = oTilesList.begin();iter!=oTilesList.end();iter++)
			{
				BOOL bError = FALSE;
				if (strToTilesFolder!=L"")
				{
					int x,y,z;

					//KosmosnimkiTileName oTileName(strToTilesFolder,strTilesType);
					StandardTileName		oTileName(strToTilesFolder,strTilesType);
					oTileName.ExtractXYZFromTileName((*iter),z,x,y);
					oTileName.CreateFolder(z,x,y);
					if (!CopyFile((*iter).data(),oTileName.getFullTileName(z,x,y).data(),FALSE)) bError = true;
				}
				if (bDeleteTiles&&(!bError))
				{
					if (!DeleteFile((*iter).data())) bError = true;
				}
				
				if (bError) nBadTiles++;
				else nTiles++;
			}
			if (!bDeleteTiles)
			{
				wcout<<nTiles<<L" tiles copied"<<endl;
			}
			else if (strToTilesFolder!=L"") 
			{
				wcout<<nTiles<<L" tiles moved"<<endl;
			}
			else wcout<<nTiles<<L" tiles deleted"<<endl;
			if (nBadTiles>0) wcout<<nBadTiles<<L" - bad tiles"<<endl;
		}
	}
	*/
	return 0;
}

