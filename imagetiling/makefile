imagetiling.exe:
	g++ -std=c++11 -I /usr/include/gdal -I ../../cpp_common_utils -c consoleutils.cpp -o consoleutils.o
	g++ -std=c++11 -I /usr/include/gdal -I ../libtilingtools/ -I ../../cpp_common_utils -L ../libtilingtools/ imagetiling.cpp consoleutils.cpp stringfuncs.h filesystemfuncs.h -o imagetiling.exe -l gdal -l tilingtools -l sqlite3 -l gd -l pthread	
	
	
	
	
