libtilingtools:
	g++ -O3 -std=c++11 -I /usr/include/gdal -c vectorborder.cpp -o vectorborder.o
	g++ -O3 -std=c++11 -I /usr/include/gdal -c tilingfuncs.cpp -o tilingfuncs.o
	g++ -O3 -std=c++11 -I /usr/include/gdal -c tilename.cpp -o tilename.o
	g++ -O3 -std=c++11 -I /usr/include/gdal -c tilecontainer.cpp -o tilecontainer.o
	g++ -O3 -std=c++11 -I /usr/include/gdal -c tilecache.cpp -o tilecache.o
	g++ -O3 -std=c++11 -I /usr/include/gdal -c rasterfile.cpp -o rasterfile.o
	g++ -O3 -std=c++11 -I /usr/include/gdal -c rasterbuffer.cpp -o rasterbuffer.o
	g++ -O3 -std=c++11 -I /usr/include/gdal -c histogram.cpp -o histogram.o
	ar cr libtilingtools.a vectorborder.o tilingfuncs.o tilename.o tilecontainer.o tilecache.o rasterfile.o rasterbuffer.o histogram.o
