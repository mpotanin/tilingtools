#pragma once
#include "stdafx.h"


namespace GMT
{

	
void	SetEnvironmentVariables (wstring gdalPath);
BOOL	LoadGDAL (int argc, _TCHAR* argv[]);

BOOL	LoadGDALDLLs (wstring gdalPath);
wstring ReadGDALPathFromConfig (wstring configFilePath);
wstring ReadConsoleParameter (wstring strPattern, int argc, _TCHAR* argv[], BOOL bFlagParam = FALSE);


}
