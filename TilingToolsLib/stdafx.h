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

//#include "../pthread/pthread.h"
#include "../gdal/include/gdal_priv.h"
#include "../gdal/include/ogrsf_frmts.h"
#include "../gdal/include/gd.h"
#include "../gdal/include/vrtdataset.h"
#include "../gdal/include/gdalwarper.h"
#include "../gdal/include/sqlite3.h"
//#include "../openjpeg-2.0/include/openjpeg.h"
#include "../openjpeg-2.0.0/src/lib/openjp2/openjpeg.h"
//#include "../gdal110/x86/include/openjpeg.h"



#include <time.h>


using namespace std;
// TODO: reference additional headers your program requires here

#endif