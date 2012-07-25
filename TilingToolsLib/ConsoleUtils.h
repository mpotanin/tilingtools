#pragma once
#include "stdafx.h"


void	SetEnvironmentVariables (wstring gdalPath);
BOOL	LoadGdal (int argc, _TCHAR* argv[]);

///*
BOOL	LoadGdalDLLs (wstring gdalPath);
wstring ReadGdalPathFromConfig (wstring strPath);
wstring ReadParameter (wstring strPattern, int argc, _TCHAR* argv[], BOOL bFlagParam = FALSE);

//*/