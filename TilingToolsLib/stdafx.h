// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef IMAGETILLINGLIB_STDAFX_H
#define IMAGETILLINGLIB_STDAFX_H 

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _FILE_OFFSET_BITS 64

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




#include "include\gdal_priv.h"
#include "include\ogrsf_frmts.h"
#include "include\gd.h"
#include "include\vrtdataset.h"
#include "include\gdalwarper.h"
#include "include\sqlite3.h"
#include <time.h>


#include <windows.h>
using namespace std;
// TODO: reference additional headers your program requires here

#endif