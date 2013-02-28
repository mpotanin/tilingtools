#include "StdAfx.h"
#include "ConsoleUtils.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"


namespace GMX
{


void	SetEnvironmentVariables (string gdalPath)
{
	wstring strPATH = (_wgetenv(L"PATH")) ? _wgetenv(L"PATH") : L"";
	wstring gdalPathW;
	utf8toWStr(gdalPathW,GetAbsolutePath(gdalPath,"bins"));
	_wputenv((L"PATH=" + gdalPathW + L";" + strPATH).c_str());
}



BOOL LoadGDAL (int argc, string argv[])
{
	string gdalPath	=  ReadConsoleParameter("-gdal",argc,argv);
	if (gdalPath == "")
	{
		wchar_t exeFileNameW[_MAX_PATH + 1];
		GetModuleFileName(NULL,exeFileNameW,_MAX_PATH); 
		string exeFileName;
		wstrToUtf8(exeFileName,exeFileNameW);
		gdalPath = ReadGDALPathFromConfigFile(GetPath(exeFileName));
	}

	if (gdalPath=="")
	{
		cout<<"Error: GDAL path isn't specified"<<endl;
		return FALSE;
	}
	
	SetEnvironmentVariables(gdalPath);

	if (!LoadGDALDLLs(gdalPath))
	{
		cout<<"Error: can't load gdal dlls: bad path to gdal specified"<<endl;
		return FALSE;
	}

	return TRUE;
}

BOOL LoadGDALDLLs (string gdalPath)
{
	wstring gdalDllW;
	utf8toWStr(gdalDllW, GetAbsolutePath(gdalPath,"bins\\gdal18.dll"));
	HMODULE b = LoadLibrary(gdalDllW.c_str());
	return (b != NULL);
}

string ReadGDALPathFromConfigFile (string configFilePath)
{
	string	strGdalTag = "<gdalpath>";
	string	strGdalCloseTag = "</gdalpath>";

	string	configFile = (configFilePath=="") ? "TilingTools.config" : GetAbsolutePath (configFilePath,"TilingTools.config");
	wstring configFileW;
	utf8toWStr(configFileW,configFile);
	
	FILE *fp = _wfopen(configFileW.c_str(),L"r");
	if (!fp) return "";
	string s;
	char c;
	while (1==fscanf(fp,"%c",&c))
		s+=c;
	fclose(fp);
	
	s = MakeLower(s);
	if (s.find(strGdalTag) == string::npos) return "";
	if (s.find(strGdalCloseTag) == string::npos) return "";

	string	strGdalPath = s.substr(	s.find(strGdalTag)+strGdalTag.length(),
									s.find(strGdalCloseTag) - s.find(strGdalTag) - strGdalTag.length());
	
	while (strGdalPath[0]==L' ' || strGdalPath[0]==L'\n')
		strGdalPath = strGdalPath.substr(1,strGdalPath.length()-1);
	while (strGdalPath[strGdalPath.length()-1]==L' ' || strGdalPath[strGdalPath.length()-1]==L'\n')
		strGdalPath = strGdalPath.substr(0,strGdalPath.length()-1);
	
	return GetAbsolutePath (configFilePath,strGdalPath);
}


string  ReadConsoleParameter (string strPattern, int argc, string argv[], BOOL bFlagParam)
{
	for (int i=0;i<argc;i++)
	{
		string strArg(argv[i]);
		strArg = MakeLower(strArg);
		strPattern = MakeLower(strPattern);
		//cout<<strPattern<<" "<<strArg<<endl;
		
		if (strPattern==strArg)
		{
			if (bFlagParam) return strPattern;
			if (i!=argc-1) 
			{
				string str(argv[i+1]);
				//if (str[0]=='-') return "";
				return str;
			}
			return "";
		}
	}
	return "";
}


}