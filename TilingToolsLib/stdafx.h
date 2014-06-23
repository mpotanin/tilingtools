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

///*

//#include "../pthread/pthread.h"
#include "../gdal110/include/gdal_priv.h"
#include "../gdal110/include/ogrsf_frmts.h"
#include "../gdal110/include/gd.h"
#include "../gdal110/include/vrtdataset.h"
#include "../gdal110/include/gdalwarper.h"
#include "../gdal110/include/sqlite3.h"
//#include "../openjpeg-2.0/include/openjpeg.h"
#include "../openjpeg-2.0.0/src/lib/openjp2/openjpeg.h"
//#include "../gdal110/x86110/x86/include/openjpeg.h"
//*/

#ifndef NO_KAKADU
#include "../kakadu/include/kdu_elementary.h"
#include "../kakadu/include/kdu_messaging.h"
#include "../kakadu/include/kdu_params.h"
#include "../kakadu/include/kdu_compressed.h"
#include "../kakadu/include/kdu_sample_processing.h"

// Application level includes
#include "../kakadu/include/kdu_file_io.h"
#include "../kakadu/include/kdu_stripe_compressor.h"
#include "../kakadu/include/kdu_stripe_decompressor.h"
#endif

/*
#include "../kakadu/v7_3_3-01328N/coresys/common/kdu_elementary.h"
#include "../kakadu/v7_3_3-01328N/coresys/common/kdu_messaging.h"
#include "../kakadu/v7_3_3-01328N/coresys/common/kdu_params.h"
#include "../kakadu/v7_3_3-01328N/coresys/common/kdu_compressed.h"
#include "../kakadu/v7_3_3-01328N/coresys/common/kdu_sample_processing.h"

// Application level includes
#include "../kakadu/v7_3_3-01328N/apps/compressed_io/kdu_file_io.h"
#include "../kakadu/v7_3_3-01328N/apps/support/kdu_stripe_compressor.h"
#include "../kakadu/v7_3_3-01328N/apps/support/kdu_stripe_decompressor.h"
*/

#include <time.h>


using namespace std;
// TODO: reference additional headers your program requires here

#endif