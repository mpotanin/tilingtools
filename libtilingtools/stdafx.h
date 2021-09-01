#pragma once

#define _FILE_OFFSET_BITS 64
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <string>
#else
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

#include "../../cpp_common_utils/filesystemfuncs.h"
#include "../../cpp_common_utils/stringfuncs.h"

#include <stdint.h>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <regex>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "vrtdataset.h"
#include "gdalwarper.h"
//#include "gd.h"
#include "sqlite3.h"


#ifdef WIN32
//debug
//#include "../openjpeg-2.3.0/openjpeg.h"
//end-debug
#endif