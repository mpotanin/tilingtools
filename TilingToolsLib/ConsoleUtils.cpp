#include "StdAfx.h"
#include "ConsoleUtils.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"


namespace gmx
{


void	SetEnvironmentVariables (string gdal_path)
{
	wstring env_PATH = (_wgetenv(L"PATH")) ? _wgetenv(L"PATH") : L"";

	wstring gdal_path_w, gdal_data_path_w, gdal_driver_path_w;
	utf8toWStr(gdal_path_w,GetAbsolutePath(gdal_path,"bins"));
  utf8toWStr(gdal_data_path_w,GetAbsolutePath(gdal_path,"bins\\gdal-data"));
  gdal_driver_path_w=L"";

	_wputenv((L"PATH=" + gdal_path_w + L";" + env_PATH).c_str());
  _wputenv((L"GDAL_DATA=" + gdal_data_path_w).c_str());
  _wputenv((L"GDAL_DRIVER_PATH=" + gdal_driver_path_w).c_str());


}



BOOL LoadGDAL (int argc, string argv[])
{
	string gdal_path	=  ReadConsoleParameter("-gdal",argc,argv);
	if (gdal_path == "")
	{
		wchar_t exe_filename_w[_MAX_PATH + 1];
		GetModuleFileName(NULL,exe_filename_w,_MAX_PATH); 
		string exe_file_name;
		wstrToUtf8(exe_file_name,exe_filename_w);
		gdal_path = ReadGDALPathFromConfigFile(GetPath(exe_file_name));
	}

	if (gdal_path=="")
	{
		cout<<"ERROR: GDAL path isn't specified"<<endl;
		return FALSE;
	}
	
	SetEnvironmentVariables(gdal_path);

	if (!LoadGDALDLLs(gdal_path))
	{
		cout<<"ERROR: can't load gdal dlls: bad path to gdal specified"<<endl;
		return FALSE;
	}

	return TRUE;
}

BOOL LoadGDALDLLs (string gdal_path)
{
	wstring gdal_dll_w;
	utf8toWStr(gdal_dll_w, GetAbsolutePath(gdal_path,"bins\\gdal18.dll"));
	HMODULE b = LoadLibrary(gdal_dll_w.c_str());
	return (b != NULL);
}

///*
string ReadGDALPathFromConfigFile (string config_file_path)
{
	string	configFile = (config_file_path=="") ? "TilingTools.config" : GetAbsolutePath (config_file_path,"TilingTools.config");
	
	FILE *fp = fopen(configFile.c_str(),"r");
	if (!fp) return "";
	string s;
	char c;
	while (1==fscanf(fp,"%c",&c))
		s+=c;
	fclose(fp);
	s+=' ';

	std::tr1::regex rx_template;
	rx_template = "^GdalPath=(.*[^\\s$])";
	
	match_results<string::const_iterator> mr;
	regex_search(s, mr, rx_template);
	if (mr.size()<2)
	{
		cout<<"ERROR: can't read GdalPath from file: "<<configFile<<endl;
		return "";
	}

	return GetAbsolutePath (config_file_path,mr[1]);
}
//*/

/*
string ReadGDALPathFromConfigFile (string config_file_path)
{
	string	strGdalTag = "<gdalpath>";
	string	strGdalCloseTag = "</gdalpath>";

	string	configFile = (config_file_path=="") ? "TilingTools.config" : GetAbsolutePath (config_file_path,"TilingTools.config");
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
	
	return GetAbsolutePath (config_file_path,strGdalPath);
}
*/

string  ReadConsoleParameter (string str_pattern, int argc, string argv[], BOOL is_flag_param)
{
	for (int i=0;i<argc;i++)
	{
		string strArg(argv[i]);
		strArg = MakeLower(strArg);
		str_pattern = MakeLower(str_pattern);
		//cout<<str_pattern<<" "<<strArg<<endl;
		
		if (str_pattern==strArg)
		{
			if (is_flag_param) return str_pattern;
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