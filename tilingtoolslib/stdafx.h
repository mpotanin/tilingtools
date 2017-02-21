#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _FILE_OFFSET_BITS 64
#define _CRT_SECURE_NO_WARNINGS
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

using namespace std;

#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "vrtdataset.h"
#include "gdalwarper.h"
#include "gd.h"
#include "sqlite3.h"