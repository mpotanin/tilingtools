imagetiling.o:
	gcc gcc -std=c++11 -I /usr/include/gdal -I ../libtilingtools/ -c imagetiling.cpp -o imagetiling.o
imagetiling.exe:
	gcc -std=c++11 -I /usr/include/gdal -I ../libtilingtools/ -L ../libtilingtools/ -l gdal -l tilingtools imagetiling.cpp -o imagetiling.exe