#pragma once
#include "stdafx.h"
#include <windows.h>


namespace gmx
{
	
void	SetEnvironmentVariables (string gdal_path);
bool	LoadGDAL (int argc, string argv[]);

bool	LoadGDALDLLs (string gdal_path);
string	ReadGDALPathFromConfigFile (string config_file_path);
string	ReadConsoleParameter (string str_pattern, int argc, string argv[], bool is_flag_param = FALSE);
bool    ParseFileParameter (string str_file_param, list<string> &file_list, int &output_bands_num, int **&pp_band_mapping);


};
