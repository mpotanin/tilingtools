#include "StdAfx.h"
#include "ConsoleUtils.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"

void	SetEnvironmentVariables (wstring gdalPath)
{
	wstring strPATH = (_wgetenv(L"PATH")) ? _wgetenv(L"PATH") : L"";

	wstring strBin = (L"PATH="+CombinePathAndFile(gdalPath,L"bins")+L";"+strPATH);
	//string strData = (L"GDAL_DATA="+(strFWTools+"\\data"));
	wstring strData = (L"GDAL_DATA="+CombinePathAndFile(gdalPath,L"bins\\gdal-data"));

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


BOOL LoadGdal (int argc, _TCHAR* argv[])
{
	//string gdalPath	= (ReadParameter(L"-gdal",argc,argv) !="") ?  ReadParameter(L"-gdal",argc,argv) : ReadParameter(L"-FWTools",argc,argv);

	wstring gdalPath	= ReadParameter(L"-gdal",argc,argv);
	if (gdalPath == L"")
	{
		TCHAR full_path[_MAX_PATH + 1];
		GetModuleFileName(NULL,full_path,_MAX_PATH);
		wstring strExePath(full_path);
		//cout<<"EXE-path: "<<strExePath<<endl;

		int n1 = strExePath.rfind(L'\\');
		int n2 = strExePath.rfind(L'/');
		if (n1<0&&n2<0) gdalPath = ReadGdalPathFromConfig(L"");
		else gdalPath = ReadGdalPathFromConfig(strExePath.substr(0,max(n1,n2)));
			
		if ((gdalPath!=L"")&&((gdalPath[0]==L'.')||(gdalPath[0]==L'/')||(gdalPath[0]==L'\\'))) gdalPath = FromRelativeToFullPath(gdalPath,strExePath.substr(0,max(n1,n2)));
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
	if (!LoadGdalDLLs(gdalPath))
	{
		wcout<<L"Error: can't load gdal dlls: bad path to gdal specified"<<endl;
		return FALSE;
	}

	return TRUE;
}

///*
BOOL LoadGdalDLLs (wstring gdalPath)
{
	//string strDll1 = CombinePathAndFile(gdalPath,L"bin\\gdal_fw.dll");
	//string strDll2 = CombinePathAndFile(gdalPath,L"bin\\bgd.dll");
	wstring strDll1 = CombinePathAndFile(gdalPath,L"bins\\gdal18.dll");

	//cout<<"gdal18.dll: "<<strDll1<<endl;
	HMODULE b = LoadLibrary(strDll1.data());
	//cout<<"After load: "<<endl;
	if (!b) return FALSE;
	return TRUE;
}

wstring FromRelativeToFullPath	(wstring strRelativePath, wstring strBasePath)
{

	if ((strBasePath[strBasePath.length()-1]!=L'/')&&(strBasePath[strBasePath.length()-1]!=L'\\'))
		strBasePath+=L"\\";
	
	while (strRelativePath[0]==L'/' || strRelativePath[0]==L'\\' || strRelativePath[0]==' '|| strRelativePath[0]=='.')
	{
		int n1 = strRelativePath.find(L'/');
		int n2 = strRelativePath.find(L'\\');
		if (n1<0 && n2<0) return L"";
		if (n1<0) n1 = strRelativePath.length()+1;
		if (n2<0) n2 = strRelativePath.length()+1;
		strRelativePath = strRelativePath.substr(min(n1,n2)+1,strRelativePath.length()-min(n1,n2)-1);

		n1 = strBasePath.rfind(L'/');
		n2 = strBasePath.rfind(L'\\');
		if (max(n1,n2)<0) return L"";
		strBasePath = strBasePath.substr(0,max(n1,n2));
	}

	return CombinePathAndFile (strBasePath,strRelativePath);//S + L"\\" + strRelativePath;
}



wstring ReadGdalPathFromConfig (wstring strPath)
{
	//string	strV = "2.2.8";

	wstring	strConfigFile;
	wstring	strGdalPath;
	wstring	strGdalTag = L"<gdalpath>";
	wstring	strGdalCloseTag = L"</gdalpath>";

	//string	strGdalTag = "<fwtoolspath>";
	//string	strGdalCloseTag = "</fwtoolspath>";


	//string	strFWTools = "fwtools";
	//strFWTools+=strV;
	if (strPath==L"") strConfigFile = L"TilingTools.config";
	else strConfigFile = CombinePathAndFile (strPath,L"TilingTools.config");

	//cout<<"CONFIG: "<<strConfigFile<<endl;


	FILE *fp = _wfopen(strConfigFile.data(),L"r");
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
	strGdalPath = s.substr(n1+strGdalTag.length(),n2-n1-strGdalTag.length());
	
	while (strGdalPath[0]==L' ' || strGdalPath[0]==L'\n')
		strGdalPath = strGdalPath.substr(1,strGdalPath.length()-1);
	while (strGdalPath[strGdalPath.length()-1]==L' ' || strGdalPath[strGdalPath.length()-1]==L'\n')
		strGdalPath = strGdalPath.substr(0,strGdalPath.length()-1);
	
	return strGdalPath;
}


wstring ReadParameter (wstring strPattern, int argc, _TCHAR* argv[], BOOL bFlagParam)
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


//*/