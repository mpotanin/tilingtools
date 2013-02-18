#pragma once
#include "stdafx.h"
#include <windows.h>


namespace GMT
{

	
void	SetEnvironmentVariables (string gdalPath);
BOOL	LoadGDAL (int argc, string argv[]);

BOOL	LoadGDALDLLs (string gdalPath);
string	ReadGDALPathFromConfigFile (string configFilePath);
string	ReadConsoleParameter (string strPattern, int argc, string argv[], BOOL bFlagParam = FALSE);


}
