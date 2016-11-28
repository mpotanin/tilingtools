#pragma once
#include "stdafx.h"
#include <windows.h>



class GMXGDALLoader
{
public:
  static bool Load (string strGDALPath = "");

private:
  static void SetEnvVars (string gdal_path);
  static bool LoadDLLs (string strGDALDir);
  static string ReadPathFromConfigFile (string config_file_path);
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
  static bool InitCmdLineArgsFromFile (string strFileName,int &nArgs, string *&pastrArgv, string strExeFilePath="");

public:
  static void PrintUsage(const GMXOptionDescriptor asDescriptors[],
                        int nDescriptors, 
                        const string astrExamples[], 
                        int nExamples);
  bool Init(const GMXOptionDescriptor asDescriptors[], int nDescriptors, string astrArgs[], int nArgs);
  string GetOptionValue(string strOptionName);
  list<string> GetValueList(string strMultipleOptionName);
  map<string,string> GetKeyValueCollection(string strMultipleKVOptionName);
private:
  void Clear();

private:
  map<string,GMXOptionDescriptor> m_mapDescriptors;
  map<string,string> m_mapOptions;
  map<string,map<string,string>> m_mapMultipleKVOptions;
  map<string,list<string>> m_mapMultipleOptions;
};