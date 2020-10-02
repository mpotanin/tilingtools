#include "consoleutils.h"
#include "stringfuncs.h"
#include "filesystemfuncs.h"

#ifdef _WIN32
string GMXGDALLoader::strGDALWinVer = "300";


void GMXGDALLoader::SetWinEnvVars (string strGDALPath)
{
	wstring wstrPATH = (_wgetenv(L"PATH")) ? _wgetenv(L"PATH") : L"";
	wstring strGDALPathW = GMXString::utf8toWStr(GMXFileSys::GetAbsolutePath(strGDALPath,"bin"));
	wstring wstrGDALDataPath = GMXString::utf8toWStr(GMXFileSys::GetAbsolutePath(strGDALPath,"bin\\gdal-data"));
	wstring wstrGDALDriverPath = L"";
	wstring wstrPROJLIBPath = GMXString::utf8toWStr(GMXFileSys::GetAbsolutePath(strGDALPath, "bin\\proj_lib"));
  
	_wputenv((L"PATH=" + strGDALPathW + L";" + wstrPATH).c_str());
	_wputenv((L"GDAL_DATA=" + wstrGDALDataPath).c_str());
	_wputenv((L"GDAL_DRIVER_PATH=" + wstrGDALDriverPath).c_str());
	_wputenv((L"PROJ_LIB=" + wstrPROJLIBPath).c_str());
}

bool GMXGDALLoader::LoadWinDll(string strGDALDir, string strDllVer)
{
  string strGDALDLL = GMXFileSys::GetAbsolutePath(strGDALDir, "bin/gdal" + strDllVer + ".dll");
  HMODULE b = LoadLibraryW(GMXString::utf8toWStr(strGDALDLL).c_str());
  if (b == NULL)
    cout << "ERROR: can't load GDAL by path: " << strGDALDLL << endl;

  return (b != NULL);
}

string GMXGDALLoader::ReadPathFromConfigFile(string strConfigFilePath)
{
	string	strConfigFile = (strConfigFilePath == "") ? "TilingTools.config" : GMXFileSys::GetAbsolutePath(strConfigFilePath, "TilingTools.config");
	string  strConfigText = GMXFileSys::ReadTextFile(strConfigFile) + ' ';

	std::regex rgxPathInput("^GdalPath=(.*[^\\s$])");
	match_results<string::const_iterator> oMatch;
	regex_search(strConfigText, oMatch, rgxPathInput);
	if (oMatch.size() < 2)
	{
		cout << "ERROR: can't read GDAL path from file: " << strConfigFile << endl;
		return "";
	}
	else
	{
		if (GMXFileSys::FileExists(oMatch[1]))
			return oMatch[1];
		else
			return GMXFileSys::GetAbsolutePath(strConfigFilePath, oMatch[1]);
	}
}

#endif




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
   
  if (!LoadWinDll(strGDALPath, GMXGDALLoader::strGDALWinVer))
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




bool GMXOptionParser::InitCmdLineArgsFromFile (string strFileName, 
                                               std::vector<string> &vecArgs,
                                                string strExeFilePath)
{
  string strFileContent = GMXFileSys::ReadTextFile(strFileName);
  if ( strFileContent == "" ) return false;
  strFileContent = " " + ((strFileContent.find('\n') != string::npos) 
                    ? strFileContent.substr(0,strFileContent.find('\n')) : strFileContent) + " ";
    
  std::regex rgxCmdPattern( "\\s+\"([^\"]+)\"\\s|\\s+([^\\s\"]+)\\s");
  match_results<string::const_iterator> oMatch;

  if (strExeFilePath!="") 
	vecArgs.push_back(strExeFilePath);
  

  while (regex_search(strFileContent, oMatch, rgxCmdPattern, std::regex_constants::match_not_null))
  {
    vecArgs.push_back( oMatch.size() == 2 ? oMatch[1].str() 
                                           : oMatch[1].str() == "" ? oMatch[2] : oMatch[1]);
    strFileContent = strFileContent.substr(oMatch[0].str().size() - 1);
  }

  if (vecArgs.size() == 0) return false;
  
  return true;
}


void GMXOptionParser::PrintUsage(const list<GMXOptionDescriptor> listDescriptors,
								const list<string> listExamples)
{
  //TODO
  int nMaxCol = 50;
  int nLineWidth=0;
  cout<<"Usage:"<<endl;
  for ( auto oDesc : listDescriptors )
  {
      if (nLineWidth+oDesc.strUsage.size() <= nMaxCol)
      {
          cout<<"["+oDesc.strOptionName+" "+oDesc.strUsage+"]";
          nLineWidth += nLineWidth+oDesc.strUsage.size();
      }
      else
      {
          cout<<endl<<"["+oDesc.strOptionName+" "+oDesc.strUsage+"]";
          nLineWidth = oDesc.strUsage.size();
      }
  }
  cout<<endl<<endl<<"Usage examples:"<<endl;
  for (auto str :listExamples)
      cout<<str<<endl;
}


void GMXOptionParser::Clear()
{
  m_mapMultipleKVOptions.clear();
  m_mapSingleOptions.clear();
  m_mapDescriptors.clear();
}



bool GMXOptionParser::Init(const list<GMXOptionDescriptor> listDescriptors,
                            vector<string> &vecArgs)
{
	Clear();
    for (auto oDesc : listDescriptors)
        m_mapDescriptors[oDesc.strOptionName]=oDesc;

	for (int i=0; i<vecArgs.size();i++)
	{
        if (vecArgs[i][0] == '-')
        {
            if (m_mapDescriptors.find(vecArgs[i])!=m_mapDescriptors.end())
            {
                if (m_mapDescriptors[vecArgs[i]].bIsBoolean)
                    this->m_mapSingleOptions[vecArgs[i]]= vecArgs[i];
                else if (i!=vecArgs.size()-1)
                {
                    if (m_mapDescriptors[vecArgs[i]].nOptionValueType==0)
                        this->m_mapSingleOptions[vecArgs[i]]= vecArgs[i+1];
                    else if (m_mapDescriptors[vecArgs[i]].nOptionValueType==1)
                        this->m_mapMultipleOptions[vecArgs[i]].push_back(vecArgs[i+1]);
                    else
                    {
                        if (vecArgs[i+1].find('=') == string::npos
                            || vecArgs[i+1].find('=') == vecArgs[i+1].size()-1)
						{
							cout<<"ERROR: can't parse key=value format from \""<< vecArgs[i+1]<<"\""<<endl;
							return false;
						}
						else
							m_mapMultipleKVOptions[vecArgs[i]][vecArgs[i+1].substr(0, vecArgs[i+1].find('='))]=
							vecArgs[i+1].substr(vecArgs[i+1].find('=')+1);
                    }
					i++;
				}
			}
			else
			{
				cout<<"ERROR: Unknown option name \""<< vecArgs[i]<<"\""<<endl;
				return false;
			}
		}
	}
  
	return true;
}


string GMXOptionParser::GetOptionValue(string strOptionName)
{
  return m_mapSingleOptions.find(strOptionName) == m_mapSingleOptions.end() 
        ? "" : m_mapSingleOptions[strOptionName];
}


list<string> GMXOptionParser::GetValueList(string strMultipleOptionName)
{
  return m_mapMultipleOptions.find(strMultipleOptionName) == m_mapMultipleOptions.end() 
         ? list<string>() : m_mapMultipleOptions[strMultipleOptionName]; 
}


map<string,string> GMXOptionParser::GetKeyValueCollection(string strMultipleKVOptionName)
{
  return m_mapMultipleKVOptions.find(strMultipleKVOptionName)==m_mapMultipleKVOptions.end() 
        ? map<string,string>() : m_mapMultipleKVOptions[strMultipleKVOptionName];
}