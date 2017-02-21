#include "stdafx.h"
#include "consoleutils.h"
#include "stringfuncs.h"
#include "filesystemfuncs.h"

string GMXGDALLoader::strGDALVer = "201";

void	GMXGDALLoader::SetWinEnvVars (string gdal_path)
{
	wstring env_PATH = (_wgetenv(L"PATH")) ? _wgetenv(L"PATH") : L"";

	wstring gdal_path_w = GMXString::utf8toWStr(GMXFileSys::GetAbsolutePath(gdal_path,"bins"));
  wstring gdal_data_path_w = GMXString::utf8toWStr(GMXFileSys::GetAbsolutePath(gdal_path,"bins\\gdal-data"));
  wstring gdal_driver_path_w = L"";
  
	_wputenv((L"PATH=" + gdal_path_w + L";" + env_PATH).c_str());
  _wputenv((L"GDAL_DATA=" + gdal_data_path_w).c_str());
  _wputenv((L"GDAL_DRIVER_PATH=" + gdal_driver_path_w).c_str());


}


bool GMXGDALLoader::Load (string strExecPath)
{
#ifdef _WIN32
  string strGDALPath = ReadPathFromConfigFile(strExecPath);

	if (strGDALPath=="")
	{
		cout<<"ERROR: GDAL path isn't specified"<<endl;
		return false;
	}
	
  SetWinEnvVars(strGDALPath);
   
  if (!LoadWinDll(strGDALPath, GMXGDALLoader::strGDALVer))
	{
		cout<<"ERROR: can't load GDAL by path: "<<strGDALPath<<endl;
		return FALSE;
	}
#else
//ToDo
#endif

  GDALAllRegister();
  OGRRegisterAll();
  CPLSetConfigOption("OGR_ENABLE_PARTIAL_REPROJECTION","YES");

	return true;
}

bool GMXGDALLoader::LoadWinDll(string strGDALDir, string strDllVer)
{
  string strGDALDLL = GMXFileSys::GetAbsolutePath(strGDALDir,"bins/gdal"+strDllVer+".dll");
  HMODULE b = LoadLibraryW(GMXString::utf8toWStr(strGDALDLL).c_str());
  if (b==NULL)
    cout<<"ERROR: can't load GDAL by path: "<<strGDALDLL<<endl;
  
  return (b != NULL);
}


string GMXGDALLoader::ReadPathFromConfigFile (string config_file_path)
{
	string	configFile = (config_file_path=="") ? "TilingTools.config" : GMXFileSys::GetAbsolutePath (config_file_path,"TilingTools.config");
	string  config_str = GMXFileSys::ReadTextFile(configFile) + ' ';

	std::tr1::regex rx_template;
	rx_template = "^GdalPath=(.*[^\\s$])";
	match_results<string::const_iterator> mr;
	regex_search(config_str, mr, rx_template);
	if (mr.size()<2)
	{
		cout<<"ERROR: can't read GDAL path from file: "<<configFile<<endl;
		return "";
	}
  else return GMXFileSys::GetAbsolutePath (config_file_path,mr[1]);
}




bool GMXOptionParser::InitCmdLineArgsFromFile (string strFileName,int &nArgs,string *&pastrArgv, string strExeFilePath)
{
  string strFileContent = GMXFileSys::ReadTextFile(strFileName);
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