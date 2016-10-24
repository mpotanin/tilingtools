// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef IMAGETILLINGLIB_STDAFX_H
#define IMAGETILLINGLIB_STDAFX_H 

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _FILE_OFFSET_BITS 64

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string>
#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <regex>
#include <math.h>
#include <cstdlib>
#include <time.h>


#include "../gdal210/include/gdal_priv.h"
#include "../gdal210/include/ogrsf_frmts.h"
#include "../gdal210/include/gd.h"
#include "../gdal210/include/vrtdataset.h"
#include "../gdal210/include/gdalwarper.h"
#include "../gdal210/include/sqlite3.h"

#include "../gdal210/include/openjpeg-2.1/openjpeg.h"


using namespace std;
// TODO: reference additional headers your program requires here

#endif