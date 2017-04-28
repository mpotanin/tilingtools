libtilingtools:
	g++ -std=c++11 -I /usr/include/gdal -c vectorborder.cpp -o vectorborder.o
	g++ -std=c++11 -I /usr/include/gdal -c tilingfuncs.cpp -o tilingfuncs.o
	g++ -std=c++11 -I /usr/include/gdal -c tilename.cpp -o tilename.o
	g++ -std=c++11 -I /usr/include/gdal -c tilecontainer.cpp -o tilecontainer.o
	g++ -std=c++11 -I /usr/include/gdal -c tilecache.cpp -o tilecache.o
	g++ -std=c++11 -I /usr/include/gdal -c stringfuncs.cpp -o stringfuncs.o
	g++ -std=c++11 -I /usr/include/gdal -c rasterfile.cpp -o rasterfile.o
	g++ -std=c++11 -I /usr/include/gdal -c rasterbuffer.cpp -o rasterbuffer.o
	g++ -std=c++11 -I /usr/include/gdal -c filesystemfuncs.cpp -o filesystem.o
	g++ -std=c++11 -I /usr/include/gdal -c histogram.cpp -o histogram.o
	g++ -std=c++11 -I /usr/include/gdal -c consoleutils.cpp -o consoleutils.o
	ar cr libtilingtools.a vectorborder.o tilingfuncs.o tilename.o tilecontainer.o tilecache.o stringfuncs.o rasterfile.o rasterbuffer.o filesystem.o histogram.o consoleutils.o
