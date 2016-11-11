// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef IMAGETILLINGLIB_STDAFX_H
#define IMAGETILLINGLIB_STDAFX_H 

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _FILE_OFFSET_BITS 64

#ifdef WIN32
#include <windows.h>
#include <string>
#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <regex>

#else
#include <string.h>
#include <list.h>
#include <map.h>
#include <iostream.h>
#include <fstream.h>
#include <regex.h>
#include <dirent.h>
#endif

#include <stdio.h>
#include <tchar.h>
#include <math.h>
#include <stdlib.h>
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