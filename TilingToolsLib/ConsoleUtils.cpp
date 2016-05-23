#include "StdAfx.h"
#include "ConsoleUtils.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"


namespace gmx
{

bool    ParseFileParameter (string str_file_param, list<string> &file_list, int &output_bands_num, int **&pp_band_mapping)
{
  string _str_file_param = str_file_param + '|';
  output_bands_num = 0;
  pp_band_mapping = 0;
  std::string::size_type stdf;

  while (_str_file_param.length()>1)
  {
    string item = _str_file_param.substr(0,_str_file_param.find('|'));
    if (item.find('?')!=  string::npos)
    {
      int *p_arr = 0;
      int len = gmx::ParseCommaSeparatedArray(item.substr(item.find('?')+1),p_arr,true,0);
      if (p_arr) delete[]p_arr;
      if (len==0) 
      {
        cout<<"Error: can't parse output bands order from: "<<item.substr(item.find('?')+1)<<endl;
        file_list.empty();
        output_bands_num = 0;
        return false;
      }
      output_bands_num = (int)max(output_bands_num,len);
      item = item.substr(0,item.find('?'));
    }

    if (!gmx::FindFilesByPattern(file_list,item))
    {
      cout<<"Error: can't find input files by path: "<<item<<endl;
      file_list.empty();
      output_bands_num = 0;
      return false;
    }
    
    _str_file_param = _str_file_param.substr(_str_file_param.find('|')+1);
  }

  if (output_bands_num>0)
  {
    pp_band_mapping = new int*[file_list.size()];
    for (int i=0;i<file_list.size();i++)
    {
       pp_band_mapping[i] = new int[output_bands_num];
       for (int j=0;j<output_bands_num;j++)
         pp_band_mapping[i][j] = j+1;
    }
  }

  _str_file_param = str_file_param + '|';
  int i=0;
  while (_str_file_param.length()>1)
  {
    string item = _str_file_param.substr(0,_str_file_param.find('|'));
    list<string> _file_list;
    if (item.find('?')!=string::npos)
    {
      gmx::FindFilesByPattern(_file_list,item.substr(0,item.find('?')));
      int *p_arr = 0;
      int len = gmx::ParseCommaSeparatedArray(item.substr(item.find('?')+1),p_arr,true,0);
      if (p_arr)
      {
        for (int j=i;j<i+_file_list.size();j++)
        {
          for (int k=0;k<len;k++)
            pp_band_mapping[j][k]=p_arr[k];
        }
        delete[]p_arr;
      }
    }
    else gmx::FindFilesByPattern(_file_list,item);
       
    i+=_file_list.size();
    _str_file_param = _str_file_param.substr(_str_file_param.find('|')+1);
  }

  return TRUE;
}


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



bool LoadGDAL (int argc, string argv[])
{
	string gdal_path	=  ReadConsoleParameter("-gdal",argc,argv);
	if (gdal_path == "")
	{
		wchar_t exe_filename_w[_MAX_PATH + 1];
		GetModuleFileNameW(NULL,exe_filename_w,_MAX_PATH); 
		string exe_filename;
		wstrToUtf8(exe_filename,exe_filename_w);
		gdal_path = ReadGDALPathFromConfigFile(GetPath(exe_filename));
	}

	if (gdal_path=="")
	{
		cout<<"Error: GDAL path isn't specified"<<endl;
		return FALSE;
	}
	
	SetEnvironmentVariables(gdal_path);
   
	if (!LoadGDALDLLs(gdal_path))
	{
		cout<<"Error: can't load gdal dlls: bad path to gdal specified: "<<gdal_path<<endl;
		return FALSE;
	}

	return TRUE;
}

bool LoadGDALDLLs (string gdal_path)
{
	wstring gdal_dll_w;
	utf8toWStr(gdal_dll_w, GetAbsolutePath(gdal_path,"bins\\gdal111.dll")); 
	HMODULE b = LoadLibraryW(gdal_dll_w.c_str());
  if (b==NULL)
    cout<<"gdal dll path: "<<gdal_dll_w<<endl;
  
  return (b != NULL);
}


string ReadGDALPathFromConfigFile (string config_file_path)
{
	string	configFile = (config_file_path=="") ? "TilingTools.config" : GetAbsolutePath (config_file_path,"TilingTools.config");
	string  config_str = ReadTextFile(configFile) + ' ';

	std::tr1::regex rx_template;
	rx_template = "^GdalPath=(.*[^\\s$])";
	match_results<string::const_iterator> mr;
	regex_search(config_str, mr, rx_template);
	if (mr.size()<2)
	{
		cout<<"Error: can't read GdalPath from file: "<<configFile<<endl;
		return "";
	}
  else return GetAbsolutePath (config_file_path,mr[1]);
}


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

string  ReadConsoleParameter (string str_pattern, int argc, string argv[], bool is_flag_param)
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