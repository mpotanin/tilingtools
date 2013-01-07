#include "StdAfx.h"
#include "ConsoleUtils.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"


namespace GMT
{


void	SetEnvironmentVariables (wstring gdalPath)
{
	wstring strPATH = (_wgetenv(L"PATH")) ? _wgetenv(L"PATH") : L"";

	wstring strBin = (L"PATH="+GetAbsolutePath(gdalPath,L"bins")+L";"+strPATH);
	//string strData = (L"GDAL_DATA="+(strFWTools+"\\data"));
	wstring strData = (L"GDAL_DATA="+GetAbsolutePath(gdalPath,L"bins\\gdal-data"));

	//string strPython = (L"PYTHONPATH="+(strFWTools+"\\pymod"));
	//string strProj = (L"PROJ_LIB="+(strFWTools+"\\proj_lib"));
	//string strGeoTiff = (L"GEOTIFF_CSV="+(strFWTools+"\\data"));
	
	
	_wputenv(strBin.data());

	//cout<<"BIN: "<<strBin<<endl;
	//putenv(strData.data());
	//putenv(strPython.data());
	//putenv(strProj.data());
	//putenv(strGeoTiff.data());
}


BOOL LoadGDAL (int argc, _TCHAR* argv[])
{
	//string gdalPath	= ( ReadConsoleParameter(L"-gdal",argc,argv) !="") ?   ReadConsoleParameter(L"-gdal",argc,argv) :  ReadConsoleParameter(L"-FWTools",argc,argv);

	wstring gdalPath	=  ReadConsoleParameter(L"-gdal",argc,argv);
	if (gdalPath == L"")
	{
		TCHAR exeFileName[_MAX_PATH + 1];
		GetModuleFileName(NULL,exeFileName,_MAX_PATH);

		wstring strExeFileName(exeFileName);
		gdalPath = ReadGDALPathFromConfig(GetPath(strExeFileName));
	}

	//cout<<"gdalpath: "<<gdalPath<<endl;

	if (gdalPath==L"")
	{
		wcout<<L"Error: Gdal path isn't specified"<<endl;
		return FALSE;
	}
	
	//if ((gdalPath[gdalPath.length()-1]==L'\\')||(gdalPath[gdalPath.length()-1]==L'/'))
	//	gdalPath = gdalPath.substr(0,gdalPath.length()-1);

	SetEnvironmentVariables(gdalPath);
	if (!LoadGDALDLLs(gdalPath))
	{
		wcout<<L"Error: can't load gdal dlls: bad path to gdal specified"<<endl;
		return FALSE;
	}

	return TRUE;
}

///*
BOOL LoadGDALDLLs (wstring gdalPath)
{
	//string strDll1 = GetAbsolutePath(gdalPath,L"bin\\gdal_fw.dll");
	//string strDll2 = GetAbsolutePath(gdalPath,L"bin\\bgd.dll");
	wstring strDll1 = GetAbsolutePath(gdalPath,L"bins\\gdal18.dll");

	//cout<<"gdal18.dll: "<<strDll1<<endl;
	HMODULE b = LoadLibrary(strDll1.data());
	//cout<<"After load: "<<endl;
	if (!b) return FALSE;
	return TRUE;
}

wstring ReadGDALPathFromConfig (wstring configFilePath)
{
	//string	strV = "2.2.8";

	

	wstring	strGdalTag = L"<gdalpath>";
	wstring	strGdalCloseTag = L"</gdalpath>";

	wstring	configFile = (configFilePath==L"") ? L"TilingTools.config" : GetAbsolutePath (configFilePath,L"TilingTools.config");

	//cout<<"CONFIG: "<<configFile<<endl;


	FILE *fp = _wfopen(configFile.data(),L"r");
	if (!fp) return L"";
	wstring s;
	_TCHAR c;
	while (1==fwscanf(fp,L"%c",&c))
		s+=c;
	fclose(fp);
	s = MakeLower(s);
	int n1 = s.find(strGdalTag);
	if (n1<0) return L"";
	int n2 = s.find(strGdalCloseTag);
	if (n2<0) return L"";

	wstring	strGdalPath;
	strGdalPath = s.substr(n1+strGdalTag.length(),n2-n1-strGdalTag.length());
	
	while (strGdalPath[0]==L' ' || strGdalPath[0]==L'\n')
		strGdalPath = strGdalPath.substr(1,strGdalPath.length()-1);
	while (strGdalPath[strGdalPath.length()-1]==L' ' || strGdalPath[strGdalPath.length()-1]==L'\n')
		strGdalPath = strGdalPath.substr(0,strGdalPath.length()-1);
	
	return GetAbsolutePath (configFilePath,strGdalPath);
}


wstring  ReadConsoleParameter (wstring strPattern, int argc, _TCHAR* argv[], BOOL bFlagParam)
{
	for (int i=0;i<argc;i++)
	{
		wstring strArg(argv[i]);
		strArg = MakeLower(strArg);
		strPattern = MakeLower(strPattern);
		//cout<<strPattern<<" "<<strArg<<endl;
		
		if (strPattern==strArg)
		{
			if (bFlagParam) return strPattern;
			if (i!=argc-1) 
			{
				wstring str(argv[i+1]);
				//if (str[0]=='-') return "";
				return str;
			}
			return L"";
		}
	}
	return L"";
}


}