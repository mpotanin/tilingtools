// ImageTiling.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


void PrintHelp ()
{

  cout<<"Usage:\n";
  cout<<"ImageTiling [-file input path] [-tiles output path] [-border vector border] [-zoom tiling zoom] [-min_zoom pyramid min tiling zoom]"; 
  cout<<" [-container write to geomixer container file] [-quality jpeg/jpeg2000 quality 0-100] [-mbtiles write to MBTiles file] [-proj tiles projection (0 - World_Mercator, 1 - Web_Mercator)] [-tile_type jpg|png|tif] [-template tile name template] [-nodata_rgb transparent color for png tiles]\n";
    
  cout<<"\nEx.1 - image to simple tiles:\n";
  cout<<"ImageTiling -file c:\\image.tif"<<endl;
  cout<<"(default values: -tiles=c:\\image_tiles -min_zoom=1 -proj=0 -template=kosmosnimki -tile_type=jpg)"<<endl;
  
  cout<<"\nEx.2 - image to tiles packed into geomixer container:\n";
  cout<<"ImageTiling -file c:\\image.tif -container -quality 90"<<endl;
  cout<<"(default values: -tiles=c:\\image.tiles -min_zoom=1 -proj=0 -tile_type=jpg)"<<endl;

  cout<<"\nEx.3 - tiling folder of images:\n";
  cout<<"ImageTiling -file c:\\images\\*.tif -container -template {z}/{x}/{z}_{x}_{y}.png  -tile_type png -nodata_rgb \"0 0 0\""<<endl;
  cout<<"(default values: -min_zoom=1 -proj=0)"<<endl;

}

void Exit()
{
  cout<<"Tiling finished - press any key"<<endl;
  char c;
  cin>>c;
}


BOOL CheckArgsAndCallTiling (string input_path,
              string use_container,		
              string max_zoom_str,
              string min_zoom_str,
              string vector_file,
              string output_path,
              string tile_type_str,
              string proj_type_str,
              string quality_str,
              string tile_name_template,
              string no_data_str,
              string transp_color_str,
              string shift_x_str,
              string shift_y_str,
              string use_pixel_tiling,
              string background_color,
              string log_file,
              string cache_size_str,
              string gdal_resampling)
{


  /*
  FILE *logFile = NULL;
  if (strLogFile!="")
  {
    if((logFile = _wfreopen(strLogFile.c_str(), "w", stdout)) == NULL)
    {
      wcout<<"ERROR: can't open log file: "<<strLogFile<<endl;
      exit(-1);
    }
  }
  */
  //проверяем входной файл или директорию
  if (input_path == "")
  {
    cout<<"ERROR: missing \"-file\" parameter"<<endl;
    return FALSE;
  }


  list<string> file_list;
  gmx::FindFilesInFolderByPattern(file_list,input_path);
  if (file_list.size()==0)
  {
    cout<<"ERROR: can't find input files by path: "<<input_path<<endl;
    return FALSE;
  }

  //синициализируем обязательные параметры для тайлинга
  gmx::MercatorProjType merc_type =	(	(proj_type_str == "") || (proj_type_str == "0") || (proj_type_str == "world_mercator")
                      || (proj_type_str == "epsg:3395")
                    ) ? gmx::WORLD_MERCATOR : gmx::WEB_MERCATOR;

  gmx::TileType tile_type;
  if ((tile_type_str == ""))
  {
    if ((tile_name_template!="") && (tile_name_template!="kosmosnimki") && (tile_name_template!="standard"))
      tile_type_str = tile_name_template.substr(tile_name_template.rfind(".")+1,
                                                tile_name_template.length()-tile_name_template.rfind(".")-1
                                                );
    else
      tile_type_str =  (gmx::MakeLower(gmx::GetExtension((*file_list.begin())))== "png") ? "png" : "jpg";
  }

  if (!gmx::TileName::TileTypeByExtension(tile_type_str,tile_type))
  {
    cout<<"ERROR: not valid tile type: "<<tile_type_str;
    return FALSE;
  }
  
  GMXTilingParameters tiling_params(input_path,merc_type,tile_type);

  if (max_zoom_str != "")		tiling_params.base_zoom_ = (int)atof(max_zoom_str.c_str());
  if (min_zoom_str != "")	tiling_params.min_zoom_ = (int)atof(min_zoom_str.c_str());
  if (vector_file!="") tiling_params.vector_file_ = vector_file;
  
  tiling_params.use_container_	= !(use_container=="");
  
  if (!tiling_params.use_container_)
  {
    if (output_path == "")
      output_path = (file_list.size()>1) ? gmx::RemoveEndingSlash(gmx::GetPath((*file_list.begin())))+"_tiles" :
                                           gmx::RemoveExtension((*file_list.begin()))+"_tiles";
    
    if (!gmx::IsDirectory(output_path))
    {
      if (!gmx::CreateDirectory(output_path.c_str()))
      {
        cout<<"ERROR: can't create folder: "<<output_path<<endl;
        return FALSE;
      }
    }
    
  }
  else
  {
    string containerExt	= (use_container == "-container") ? "tiles" : "mbtiles"; 
    if (gmx::IsDirectory(output_path))
    {
      tiling_params.container_file_ =  gmx::GetAbsolutePath(output_path,gmx::RemoveExtension(gmx::RemovePath((*file_list.begin())))+"."+containerExt);
    }
    else
    {
      if (output_path == "")
       tiling_params.container_file_ = gmx::RemoveExtension((*file_list.begin())) + "." + containerExt;
      else
       tiling_params.container_file_ =  (gmx::GetExtension(output_path) == containerExt) ? output_path : output_path+"."+containerExt;
    }
  }


  if (!tiling_params.use_container_ )
  {
    if ((tile_name_template=="") || (tile_name_template=="kosmosnimki"))
        tiling_params.p_tile_name_ = new gmx::KosmosnimkiTileName(output_path,tile_type);
    else if (tile_name_template=="standard") 
        tiling_params.p_tile_name_ = new gmx::StandardTileName(	output_path,("{z}/{x}/{y}." + 
                                gmx::TileName::ExtensionByTileType(tile_type)));
    else if (gmx::ESRITileName::ValidateTemplate(tile_name_template))
        tiling_params.p_tile_name_ = new gmx::ESRITileName(output_path,tile_name_template);
    else if (gmx::StandardTileName::ValidateTemplate(tile_name_template))
        tiling_params.p_tile_name_ = new gmx::StandardTileName(output_path,tile_name_template);
    else
    {
      cout<<"ERROR: can't validate \"-template\" parameter: "<<tile_name_template<<endl;
      return FALSE;
    }
  }


  if ((shift_x_str!="")) tiling_params.shift_x_=atof(shift_x_str.c_str());
  if ((shift_y_str!="")) tiling_params.shift_y_=atof(shift_y_str.c_str());

  if (cache_size_str!="") 
    tiling_params.max_tiles_in_cache_ = atof(cache_size_str.c_str());

  if (quality_str!="")
    tiling_params.jpeg_quality_ = (int)atof(quality_str.c_str());
  
  if (no_data_str!="")
  {
    tiling_params.p_nodata_value_ = new int[1];
    tiling_params.p_nodata_value_[0] = atof(no_data_str.c_str());
  }

  if (transp_color_str!="")
  {
    BYTE	rgb[3];
    if (!gmx::ConvertStringToRGB(transp_color_str,rgb))
    {
      cout<<"ERROR: bad value of parameter: \"-no_data_rgb\""<<endl;
      return FALSE;
    }
    tiling_params.p_transparent_color_ = new BYTE[3];
    memcpy(tiling_params.p_transparent_color_,rgb,3);
  }

  tiling_params.gdal_resampling_ = gdal_resampling;

  tiling_params.auto_stretch_to_8bit_ = true;

  return GMXMakeTiling(&tiling_params);
  //if (logFile) fclose(logFile);
}



int _tmain(int argc, wchar_t* argvW[])
{
  //cout.imbue(std::locale("rus_rus.866"));
  //locale myloc("");
  
  string *argv = new string[argc];
  for (int i=0;i<argc;i++)
  {
    gmx::wstrToUtf8(argv[i],argvW[i]);
    gmx::ReplaceAll(argv[i],"\\","/");
  }
  
  if (!gmx::LoadGDAL(argc,argv))
  {
    cout<<"ERROR: can't load GDAL"<<endl;
    return 1;
  }
  GDALAllRegister();
  OGRRegisterAll();

  if (argc == 1)
  {
    PrintHelp();
    return 0;
  }

  //обязательный параметр
  string input_path	= gmx::ReadConsoleParameter("-file",argc,argv);

  //дополнительные параметры
  string mosaic_mode = gmx::ReadConsoleParameter("-mosaic",argc,argv,TRUE);
  string use_container = (gmx::ReadConsoleParameter("-container",argc,argv,TRUE) != "") ? 
                      gmx::ReadConsoleParameter("-container",argc,argv,TRUE) : 
                      gmx::ReadConsoleParameter("-mbtiles",argc,argv,TRUE);
  string max_zoom_str	= gmx::ReadConsoleParameter("-zoom",argc,argv);
  string min_zoom_str	= gmx::ReadConsoleParameter("-min_zoom",argc,argv);	
  string vector_file = gmx::ReadConsoleParameter("-border",argc,argv);
  string output_path = gmx::ReadConsoleParameter("-tiles",argc,argv);
  string tile_type_str = gmx::ReadConsoleParameter("-tile_type",argc,argv);
  string proj_type_str = gmx::ReadConsoleParameter("-proj",argc,argv);
  string tile_name_template = gmx::ReadConsoleParameter("-template",argc,argv);
  string no_data_str = gmx::ReadConsoleParameter("-nodata",argc,argv) != "" ?
                      gmx::ReadConsoleParameter("-nodata",argc,argv) :
                      gmx::ReadConsoleParameter("-no_data",argc,argv);
  string transp_color_str	= gmx::ReadConsoleParameter("-nodata_rgb",argc,argv) != "" ?
                            gmx::ReadConsoleParameter("-nodata_rgb",argc,argv) :
                            gmx::ReadConsoleParameter("-no_data_rgb",argc,argv);

  string quality_str = gmx::ReadConsoleParameter("-quality",argc,argv);

  string gdal_resampling = gmx::ReadConsoleParameter("-resampling",argc,argv);

  //скрытые параметры
  //string need_Edges				=  gmx::ReadConsoleParameter("-edges",argc,argv);
  string shift_x_str = gmx::ReadConsoleParameter("-shiftX",argc,argv);
  string shift_y_str = gmx::ReadConsoleParameter("-shiftY",argc,argv);
  string use_pixel_tiling	= gmx::ReadConsoleParameter("-pixel_tiling",argc,argv,TRUE);
  string background_color	= gmx::ReadConsoleParameter("-background",argc,argv);
  string log_file	= gmx::ReadConsoleParameter("-log_file",argc,argv);
  string cache_size_str	= gmx::ReadConsoleParameter("-cache",argc,argv);
  //string	strN

  if (argc == 2)				input_path	= argv[1];
  wcout<<endl;


  //  opj_stream_t* p_opj_stream = opj_stream_default_create(false);
  //FILE *p_f = fopen("E:\\TestData\\Ikonos\\ch1-4_16bit_ip.tif","rb");
  //for (int i=0;i<100;i++)
  //{
 // cout<<i<<endl;
  /*
  for (int i=0;i<10000;i++)
  {
    cout<<i<<endl;
  BYTE *p_input_data;
  unsigned int size;
  void *p_data;int size2;
  gmx::RasterBuffer buffer;
  gmx::ReadDataFromFile("e:\\ch1-4_16bit.tif",p_input_data,size);
  buffer.CreateBufferFromTiffData(p_input_data,size);
  delete[]p_input_data;
  buffer.SaveToJP2Data(p_data,size2);
  buffer.createFromJP2Data(p_data,size2);
  delete[]p_data;
  buffer.SaveToTiffData(p_data,size2);
  gmx::SaveDataToFile("e:\\ch1-4_16bit_out.tif",p_data,size2);
  delete[]p_data;
  }*/
  //gmx::ReadDataFromFile("E:\\TestData\\Ikonos\\ch1-4_16bit_ip.tif",p_input_data,size);
  

  //gmx::ReadDataFromFile("e:\\erosb_16bit.jp2",p_input_data,size);
  //buffer.createFromJP2Data(p_input_data,size);
  //delete[]p_input_data;
  //buffer.SaveToTiffData(p_data,size2);
  //gmx::SaveDataToFile("e:\\erosb_16bit.tif",p_data,size2);

  /*
  gmx::ReadDataFromFile("e:\\Landsat8\\landsat_8_cut.tif",p_input_data,size);
  buffer.CreateBufferFromTiffData(p_input_data,size);
  delete[]p_input_data;
  buffer.SaveToJP2Data(p_data,size2);
  gmx::SaveDataToFile("e:\\landsat8_ch1-5_16bit.jp2",p_data,size2);
  delete[]p_data;
  */
  

  //buffer.SaveToJpegData(85,p_data,size2);
  //}

   //C:\Work\Projects\TilingTools\autotest\result\scn_120719_Vrangel_island_SWA.tiles -zoom 8 -cache 10


  //max_zoom			= "8";
  //proj_type	= "1";
  //tile_name_template		= "standard";
  //vector_file	= "E:\\MODIS_IRK\\A1304191903.mif";

  //-no_data_rgb "0 0 0" -tile_type png -border C:\Work\Projects\TilingTools\autotest\border\markers.tab


  //input_path		= "\\\\192.168.4.43\\shareH\\spot6\\LENINGRAD-1_5m\\*.tif";
  //transp_color_str = "0 0 0";
  //tile_type_str = "png";
  //max_zoom_str			= "8";

  //tile_name_template = "standard";
  //use_container = "-container";

  //mosaic_mode = "mosaic";
  //vector_file	  = "C:\\Work\\Projects\\TilingTools\\autotest\\Black_png\\20131007_101711_NPP_SVI.shp";
  //output_path		= "e:\\spot6_tiles";
  //tile_type_str = "jpg";
  //max_zoom_str			= "16";
  //quality_str = "4";
  



  ///*
  //input_path		= "E:\\test_images\\png\\171013.png";
  //tile_name_template = "{l}\\{r}\\{c}.png";
  //vector_file	= "C:\\Work\\Projects\\TilingTools\\autotest\\border_erosb\\erosb_border.shp";
  //max_zoom_str		= "17";
  //tile_name_template = "standard";


  //input_path		= "E:\\TestData\\o42073g8.tif";
  //vector_file	= "C:\\Work\\Projects\\TilingTools\\autotest\\border_o42073g8\\border.shp";
  //use_container = "-container";
  //max_zoom_str		= "20";
  //tile_type_str	= "png";
  //*/
  
  if (input_path == "")
  {
    cout<<"ERROR: missing \"-file\" parameter"<<endl;
    return 1;
  }


 
  if (mosaic_mode=="")
  {
    std::list<string> input_file_list;
    if (!gmx::FindFilesInFolderByPattern (input_file_list,input_path))
    {
      cout<<"Can't find input files by pattern: "<<input_path<<endl;
      return 1;
    }


    for (std::list<string>::iterator iter = input_file_list.begin(); iter!=input_file_list.end();iter++)
    {
      cout<<"Tiling file: "<<(*iter)<<endl;
      if (input_file_list.size()>1)
        vector_file = gmx::VectorBorder::GetVectorFileNameByRasterFileName(*iter);
      string output_path_fix = output_path;
      if ((input_file_list.size()>1)&&(output_path!=""))
      {
        if (!gmx::FileExists(output_path)) 
        {
          if (!gmx::CreateDirectory(output_path.c_str()))
          {
            cout<<"ERROR: can't create directory: "<<output_path<<endl;
            return 1;
          }
        }
        output_path_fix = (use_container == "") ? gmx::RemoveEndingSlash(output_path) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +"_tiles" 
                            : (use_container == "-mbtiles") ? gmx::RemoveEndingSlash(output_path) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +".mbtiles" 
                                            : gmx::RemoveEndingSlash(output_path) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +".tiles"; 
      }

      if ((! CheckArgsAndCallTiling (	(*iter),
                    use_container,		
                    max_zoom_str,
                    min_zoom_str,
                    vector_file,
                    output_path_fix,
                    tile_type_str,
                    proj_type_str,
                    quality_str,
                    tile_name_template,
                    no_data_str,
                    transp_color_str,
                    shift_x_str,
                    shift_y_str,
                    use_pixel_tiling,
                    background_color,
                    log_file,
                    cache_size_str,
                    gdal_resampling)) && input_file_list.size()==1) 
      {
        return 2;
      }
      wcout<<endl;
    }
  }
  else
  {
    if (! CheckArgsAndCallTiling (	input_path,
                  use_container,		
                  max_zoom_str,
                  min_zoom_str,
                  vector_file,
                  output_path,
                  tile_type_str,
                  quality_str,
                  proj_type_str,
                  tile_name_template,
                  no_data_str,
                  transp_color_str,
                  shift_x_str,
                  shift_y_str,
                  use_pixel_tiling,
                  background_color,
                  log_file,
                  cache_size_str,
                  gdal_resampling))
                 
    {
     return 2;
    }
  }

  return 0;
  
}