# Command line tools for tiling georeferenced raster data

## Table of Contents
  * [Download binary](#download-binary)
  * [Using imagetiling](#using-imagetiling)
  * [Using copytiles](#using-copytiles)
  * [Building from Source](#building-from-source)
  
## Download Binary
[Download win-x64 binary 3.0.4 version (30.03.2017)](http://kosmosnimki.ru/downloads/tilingtools-3.0.4-win-x64.zip) ready for use package compiled with Microsoft Visual C++ 2013.

## Using imagetiling

### Parameters
* **-i** - input raster file or file name template (**obligatory parameter**)
* **-ap** - flag parameter which turns off mosaic mode - each input file is processed apart. By default: one layer of tiles is generated (mosaic mode)
* **-o** - output file or folder. Default: output path is generated by adding some suffix to input path
* **-z** - max zoom (base zoom). Default: max zoom is calculated from input raster file resolution and spatial reference system
* **-b** - vector clip mask. Default: is not defined
* **-minz** - min zoom. Default: 0 or 1
* **-tt** - tile type: png, jpg, jp2, tif. Default: jpg
* **-r** - resampling method tha is used to generate base zoom tiles (pyramid level tiles are generated from previous layer tiles by average resampling). Possible values: near, bilinear, cubic, lanczos. Default: cubic for general cases and near for rasters with index color table
* **-q** - compression quality 1-100. Default: 85
* **-of** - output tile container format: gmxtiles, mbtiles. Default: separate tiles 
* **-co** - creation options specific to output tile container format
* **-tsrs** - tiling spatial reference system. Possible values: 0 (World Mercator, EPSG:3395), 1 (Web Mercator or Spherical Mercatorv, EPSG:3857). Default value: 1 
* **-tnt** - output tile name templete. Default value: {z}/{x}/{y}
* **-nd** - nodata value. Default: isn't defined
* **-ndt** - nodata value tolerance. Default: isn't defined
* **-bgc** - background color. Default: black color - 0,0,0
* **-bnd** - raster band list order. Default: isn't defined
* **-wt** - working threads number. Default: 2
* **-pseudo_png** - code uint16 value into png red and green bands. Default: isn't defined

### Examples
```
imagetiling -i image.tif -of mbtiles -o image.mbtiles -tt png
imagetiling -i image1.tif -i image2.tif -o image1-2_tiles -tt jpg -z 18 -minz 10
imagetiling -i images/\*.tif -of mbtiles -o images_tiles -tnt {z}_{x}_{y}.png
imagetiling -i image.tif -b clip.shp -nd 0 -of mbtiles -o image.mbtiles -tt png
```

## Using CopyTiles
### Parameters
* **-i** - input folder or file
* **-o** - output folder or file 
* **-of** - output tile container type
* **-b** - vector clip mask: shp,tab,mif,kml,geojson
* **-tt** - tile type: jpg,png
* **-z** - min-max zoom, specifies zoom range of tiles to copy 
* **-co** - creation options specific to output tile container format
* **-tsrs** - tiling spatial reference system. Possible values: 0 (World Mercator, EPSG:3395), 1 (Web Mercator or Spherical Mercatorv, EPSG:3857). Default value: 1 
* **-i_tnt** - input tile name template
* **-o_tnt** - output tile name template

### Examples
```
 copytiles -i tiles -i_tnt standard -tt png -of MBTiles -o tiles.mbtiles -z 10-15
 copytiles -i tiles -i_tnt {z}/{x}/{y}.png -of MBTiles -o tiles.mbtiles -b zone.shp
 copytiles -i tiles -i_tnt {z}/{x}/{y}.png -o tiles_new -o_tnt {z}_{x}_{y}.png
```

## Building from Source
### Building on Windows
* requirements:
  * x64 platform
  * Visual Studio 2010/2013/2015
* git clone tilingtools repository or download it as zip-archive 
* unzip gdal210.zip archive (after unzip there must be a path: TilinTools-master/gdal210/include)
* open TilingTools.sln with Visual Studio 2010/2013/2015
* compile TilingTools (set in Configuration Manager: configuration=**Release, platform=x64**)
* if compilation succeed exe-files will be created in folder /x64/Release/  

### Building on Linux
* requirements:
  * x64 platform
  * gcc 4.9.0 or newer version
  * development x64 version of the libraries: gdal, sqlite3, gd (if git is installed you may run: ```apt-get install git make libgdal-dev sqlite3 libgd-dev```)
* git clone tilingtools repository (```git clone https://github.com/scanex/tilingtools tilingtools```) or download it as zip-archive  
* open tilingtools dir and run ```make```
