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
        cout<<"ERROR: can't parse output bands order from: "<<item.substr(item.find('?')+1)<<endl;
        file_list.empty();
        output_bands_num = 0;
        return false;
      }
      output_bands_num = (int)max(output_bands_num,len);
      item = item.substr(0,item.find('?'));
    }

    if (!gmx::FindFilesByPattern(file_list,item))
    {
      cout<<"ERROR: can't find input files by path: "<<item<<endl;
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

  _wputenv((L"OGR_ENABLE_PARTIAL_REPROJECTION=YES" + gdal_driver_path_w).c_str());
	_wputenv((L"PATH=" + gdal_path_w + L";" + env_PATH).c_str());
  _wputenv((L"GDAL_DATA=" + gdal_data_path_w).c_str());
  _wputenv((L"GDAL_DRIVER_PATH=" + gdal_driver_path_w).c_str());


}



bool LoadGDAL (int argc, string argv[])
{
	string gdal_path	=  ParseValueFromCmdLine("-gdal",argc,argv);
	if (gdal_path == "")
	{
		wchar_t exe_filename_w[_MAX_PATH + 1];
		GetModuleFileNameW(NULL,exe_filename_w,_MAX_PATH); 
		string exe_filename;
		wstrToUtf8(exe_filename,exe_filename_w);
    ReplaceAll(exe_filename,"\\","/");
    gdal_path = ReadGDALPathFromConfigFile(GetPath(exe_filename));
	}

	if (gdal_path=="")
	{
		cout<<"ERROR: GDAL path isn't specified"<<endl;
		return FALSE;
	}
	
	SetEnvironmentVariables(gdal_path);
   
	if (!LoadGDALDLLs(gdal_path))
	{
		cout<<"ERROR: can't load gdal dlls: bad path to gdal specified: "<<gdal_path<<endl;
		return FALSE;
	}

	return TRUE;
}

bool LoadGDALDLLs (string gdal_path)
{
	wstring gdal_dll_w;
	utf8toWStr(gdal_dll_w, GetAbsolutePath(gdal_path,"bins\\gdal201.dll")); 
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
		cout<<"ERROR: can't read GdalPath from file: "<<configFile<<endl;
		return "";
	}
  else return GetAbsolutePath (config_file_path,mr[1]);
}


map<string,string> ParseKeyValueCollectionFromCmdLine (string strCollectionName, int nArgs, string pastrArg[])
{
  map<string,string> mapReturn;
  regex rgxKeyValue(".+=.+"); //todo - check
  for (int i=0;i<nArgs;i++)
	{
		if (MakeLower(strCollectionName)==MakeLower(pastrArg[i]))
		{
      if (i!=nArgs-1)
      {
        if (regex_match(pastrArg[i+1],rgxKeyValue))
        {
          int nPos=pastrArg[i+1].find('=');
          mapReturn.insert(pair<string,string>(pastrArg[i+1].substr(0,nPos),pastrArg[i+1].substr(nPos+1)));
        }
      }
		}
	}
  return mapReturn;
}

string  ParseValueFromCmdLine (string strKey, int nArgs, string pastrArg[], bool bIsBoolean)
{
	for (int i=0;i<nArgs;i++)
	{
		if (MakeLower(strKey)==MakeLower(pastrArg[i]))
		{
			if (bIsBoolean) return strKey;
			else if (i!=nArgs-1) 
				return pastrArg[i+1];
		}
	}
	return "";
}


/*
Class ImageTilingParametersList
{
name, boolean, 
}

*/


bool InitCmdLineArgsFromFile (string strFileName,int &nArgs,string *&pastrArgv, string strExeFilePath)
{
  string strFileContent = gmx::ReadTextFile(strFileName);
  if (strFileContent=="") return false;
  strFileContent = " " + ((strFileContent.find('\n')!=string::npos) ? strFileContent.substr(0,strFileContent.find('\n')) : strFileContent) + " ";
    
  std::regex rgxCmdPattern( "\\s+\"([^\"]+)\"\\s|\\s+([^\\s\"]+)\\s");
  match_results<string::const_iterator> mr;

  string astrBuffer[1000];
  if (strExeFilePath=="") nArgs=0;
  else
  {
    astrBuffer[0]=strExeFilePath;
    nArgs=1;
  }

  while (regex_search(strFileContent,mr,rgxCmdPattern,std::regex_constants::match_not_null))
  {
    astrBuffer[nArgs] = mr.size()==2 ? mr[1].str() : 
                       mr[1].str()=="" ? mr[2] : mr[1];
    nArgs++;
    strFileContent = strFileContent.substr(mr[0].str().size()-1);
  }

  if (nArgs==0) return false;
  
  if (pastrArgv) delete[]pastrArgv;
  pastrArgv = new string[nArgs];
  for (int i=0;i<nArgs;i++)
    pastrArgv[i]=astrBuffer[i];

  return true;
}

};

void GMXOptionParser::PrintUsage (const GMXOptionDescriptor asDescriptors[], 
                                  int nDescriptors, 
                                  const string astrExamples[],
                                  int nExamples)
{
  //TODO
  int nMaxCol = 50;
  int nLineWidth=0;
  cout<<"Usage:"<<endl;
  for (int i=0;i<nDescriptors;i++)
  {
    if (nLineWidth+asDescriptors[i].strUsage.size()<=nMaxCol)
    {
      cout<<"["+asDescriptors[i].strOptionName+" "+asDescriptors[i].strUsage+"]";
      nLineWidth+=nLineWidth+asDescriptors[i].strUsage.size();
    }
    else
    {
      cout<<endl<<"["+asDescriptors[i].strOptionName+" "+asDescriptors[i].strUsage+"]";
      nLineWidth=asDescriptors[i].strUsage.size();
    }
  }
  cout<<endl<<endl<<"Usage examples:"<<endl;
  for (int  i=0;i<nExamples;i++)
    cout<<astrExamples[i]<<endl;
}


void GMXOptionParser::Clear()
{
  m_mapMultipleKVOptions.clear();
  m_mapOptions.clear();
  m_mapDescriptors.clear();
}



bool GMXOptionParser::Init(const GMXOptionDescriptor asDescriptors[], int nDescriptors, string astrArgs[], int nArgs)
{
  Clear();
  for (int i=0;i<nDescriptors;i++)
    m_mapDescriptors[asDescriptors[i].strOptionName]=asDescriptors[i];

  for (int i=0;i<nArgs;i++)
  {
    if (astrArgs[i][0]=='-')
    {
      if (m_mapDescriptors.find(astrArgs[i])!=m_mapDescriptors.end())
      {
        if (m_mapDescriptors[astrArgs[i]].bIsBoolean)
          this->m_mapOptions[astrArgs[i]]=astrArgs[i];
        else if (i!=nArgs-1)
        {
          if (m_mapDescriptors[astrArgs[i]].nOptionValueType==0)
            this->m_mapOptions[astrArgs[i]]=astrArgs[i+1];
          else if (m_mapDescriptors[astrArgs[i]].nOptionValueType==1)
            this->m_mapMultipleOptions[astrArgs[i]].push_back(astrArgs[i+1]);
          else
          {
            if (astrArgs[i+1].find('=')==string::npos||
              astrArgs[i+1].find('=')==astrArgs[i+1].size()-1)
            {
              cout<<"ERROR: can't parse key=value format from \""<<astrArgs[i+1]<<"\""<<endl;
              return false;
            }
            else
              m_mapMultipleKVOptions[astrArgs[i]][astrArgs[i+1].substr(0,astrArgs[i+1].find('='))]=
                                astrArgs[i+1].substr(astrArgs[i+1].find('=')+1);
          }
          i++;
        }
      }
      else
      {
        cout<<"ERROR: Unknown option name \""<<astrArgs[i]<<"\""<<endl;
        return false;
      }
    }
  }
  
  return true;
}


string GMXOptionParser::GetOptionValue(string strOptionName)
{
  return m_mapOptions.find(strOptionName)==m_mapOptions.end() ? "" : 
                                            m_mapOptions[strOptionName];
}


list<string> GMXOptionParser::GetValueList(string strMultipleOptionName)
{
  list<string> empty;
  return m_mapMultipleOptions.find(strMultipleOptionName)==m_mapMultipleOptions.end() ?
                                                                empty :
                                                                m_mapMultipleOptions[strMultipleOptionName]; 
}


map<string,string> GMXOptionParser::GetKeyValueCollection(string strMultipleKVOptionName)
{
  map<string,string> empty;
  return m_mapMultipleKVOptions.find(strMultipleKVOptionName)==m_mapMultipleKVOptions.end() ?
                                                                empty :
                                                                m_mapMultipleKVOptions[strMultipleKVOptionName]; 
}