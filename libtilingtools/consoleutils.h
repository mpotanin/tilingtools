#pragma once
#include "stdafx.h"


class GMXGDALLoader
{
public:
  static string ReadPathFromConfigFile(string strConfigFilePpath);
  static bool Load (string strExecPath);

protected:
  static void SetWinEnvVars (string strGDALPath);
  static bool LoadWinDll (string strGDALDir, string strDllVer);
  

protected:
  static string strGDALWinVer;
};

typedef struct 
{
  string strOptionName;
  bool bIsBoolean;
  int nOptionValueType; //0 - single, 1 - multiple, 2 - multiple key=value
  string strUsage;
} GMXOptionDescriptor;


class GMXOptionParser
{
public:
  static bool InitCmdLineArgsFromFile (string strFileName, 
                                      int &nArgs, 
                                      string *&pastrArgv, 
                                      string strExeFilePath="");

public:
  static void PrintUsage (const list<GMXOptionDescriptor> listDescriptors,
                          const list<string> listExamples);
    
  bool Init (const list<GMXOptionDescriptor> listDescriptors, 
              string astrArgs[], int nArgs);
    
  string GetOptionValue (string strOptionName);
  list<string> GetValueList (string strMultipleOptionName);
  map<string,string> GetKeyValueCollection (string strMultipleKVOptionName);

private:
  void Clear();

private:
  map<string,GMXOptionDescriptor> m_mapDescriptors;
  map<string,string> m_mapSingleOptions;
  map<string,map<string,string>> m_mapMultipleKVOptions;
  map<string,list<string>> m_mapMultipleOptions;
};