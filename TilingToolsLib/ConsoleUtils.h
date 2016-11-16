#pragma once
#include "stdafx.h"
#include <windows.h>


namespace gmx
{
	
void SetEnvironmentVariables (string gdal_path);
bool LoadGDAL (int argc, string argv[]);

bool LoadGDALDLLs (string strGDALDir);
string ReadGDALPathFromConfigFile (string config_file_path);
string ParseValueFromCmdLine (string strKey, int nArgs, string pastrArgv[], bool bIsBoolean=false);
map<string,string> ParseKeyValueCollectionFromCmdLine (string strCollectionName, int nArgs, string pastrArgv[]);
bool ParseFileParameter (string str_file_param, list<string> &file_list, int &output_bands_num, int **&pp_band_mapping);
bool InitCmdLineArgsFromFile (string strFileName,int &nArgs, string *&pastrArgv, string strExeFilePath="");

/*

init static GMXOptionDescriptor[] object before main body - list all options
init GMXOptionParser object
loop over all Options and init TilingParameters
read-write extra options without parsing


*/

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