#pragma once
#include "stdafx.h"
#include <windows.h>


namespace gmx
{

	
void	SetEnvironmentVariables (string gdal_path);
BOOL	LoadGDAL (int argc, string argv[]);

BOOL	LoadGDALDLLs (string gdal_path);
string	ReadGDALPathFromConfigFile (string config_file_path);
//string	ReadGDALPathFromConfigFile2(string config_file_path);
string	ReadConsoleParameter (string str_pattern, int argc, string argv[], BOOL is_flag_param = FALSE);


}
