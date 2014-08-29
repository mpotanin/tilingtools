// ImageTiling.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


void PrintHelp ()
{

  cout<<"Usage: ImageTiling [-file input path] [-tiles output path]"<<endl;
  cout<<"       [-border vector border] [-zoom tiling zoom]"<<endl; 
  cout<<"       [-min_zoom min tiling zoom] [-container write to geomixer container]"<<endl;
  cout<<"       [-quality jpeg/jpeg2000 quality 0-100] [-mbtiles write to MBTiles]"<<endl;
  cout<<"       [-proj tiles projection (0 - EPSG:3395, 1 - EPSG:3857)]"<<endl;
  cout<<"       [-tile_type jpg|png|jp2] [-template tile name template]"<<endl;
  cout<<"       [-nodata transparent color for png tiles]"<<endl;
  cout<<"       [-background rgb-color for tiles backgroud]"<<endl;
  cout<<"       [-nodata_tolerance tolerance value for \"no_data\" parameter]"<<endl;

  cout<<"\nExample 1 - image to simple tiles:"<<endl;
  cout<<"ImageTiling -file c:\\image.tif"<<endl;
  cout<<"(default values: -tiles=c:\\image_tiles -min_zoom=1 -proj=0 -template=kosmosnimki -tile_type=jpg)"<<endl;
  
  cout<<"\nExample 2 - image to tiles packed into geomixer container:"<<endl;
  cout<<"ImageTiling -file c:\\image.tif -container -quality 90"<<endl;
  cout<<"(default values: -tiles=c:\\image.tiles -min_zoom=1 -proj=0 -tile_type=jpg)"<<endl;

  cout<<"\nExample 3 - tiling folder of images:"<<endl;
  cout<<"ImageTiling -file c:\\images\\*.tif -container -template {z}/{x}/{z}_{x}_{y}.png  -tile_type png -nodata \"0 0 0\""<<endl;
  cout<<"(default values: -min_zoom=1 -proj=0)"<<endl;

}

void Exit()
{
  cout<<"Tiling finished - press any key"<<endl;
  char c;
  cin>>c;
}

BOOL CreateJGW (string jpg_file, int z, int x, int y, int size)
{
  gmx::RasterFile rf;
  if (!rf.Init(jpg_file,FALSE)) return FALSE;

  int w,h;
  rf.GetPixelSize(w,h);

  OGREnvelope envp = gmx::MercatorTileGrid::CalcEnvelopeByTile(z,x,y);
  double res = gmx::MercatorTileGrid::CalcResolutionByZoom(z)* (((double)size)/(max(w,h)));
  //return gmx::WriteWLDFile(jpg_file,envp.MinX+res*0.5,envp.MaxY-res*0.5,res);
  return gmx::WriteWLDFile(jpg_file,envp.MinX + (res*0.5),envp.MaxY - (res*0.5),res);

}


BOOL CreateGeoreference(list<string> jpg_files, int z, int size)
{
  list<string>::iterator iter = jpg_files.begin();
  for (int i=0;i<(1<<z);i+=2*(size/gmx::MercatorTileGrid::TILE_SIZE))
  {
    for (int j=0;j<(1<<z);j+=2*(size/gmx::MercatorTileGrid::TILE_SIZE))
    {
      if (iter != jpg_files.end())
      {
        CreateJGW((*iter),z,i,j,size);
        iter++;
      }
    }
  }

  return TRUE;
}


BOOL ProcessFolder (string input_folder, int z, int size)
{
  list<string> files;
  if (!gmx::FindFilesInFolderByExtension(files,input_folder, "jpg", FALSE)) return FALSE;
  int num = files.size();
  if (num==0) return FALSE;

  string *arr = new string[num];
  for (int i=0;i<num;i++)
    arr[i]="";
  srand(0);
  for (list<string>::iterator iter = files.begin();iter!=files.end();iter++)
  {
    int k = rand()%num;
    for (int i = 0; i<num; i++)
    {
      if (arr[(k+i)%num]=="")
      {
        arr[(k+i)%num] = (*iter);
        break;
      }
    }
  }
  
  list<string> files_rand;
  for (int i = 0; i<num; i++)
    files_rand.push_back(arr[i]);

  return CreateGeoreference(files_rand,z,size);
}

BOOL CheckArgsAndCallTiling (map<string,string> console_params)
{
  string input_path = console_params.at("-file");
  string use_container = console_params.at("-gmxtiles") != "" ? console_params.at("-gmxtiles") : console_params.at("-mbtiles");		
  string max_zoom_str = console_params.at("-zoom");
  string min_zoom_str = console_params.at("-min_zoom");
  string vector_file = console_params.at("-border");
  string output_path = console_params.at("-tiles");
  string tile_type_str = console_params.at("-tile_type");
  string proj_type_str = console_params.at("-proj");
  string quality_str = console_params.at("-quality");
  string tile_name_template = console_params.at("-template");
  string nodata_str = console_params.at("-nodata");
  string nodata_tolerance_str = console_params.at("-nodata_tolerance");
  string shift_x_str = console_params.at("-shiftX");
  string shift_y_str = console_params.at("-shiftY");
  string use_pixel_tiling = console_params.at("-pixel_tiling");
  string background_str = console_params.at("-background");
  string log_file = console_params.at("-log_file");
  string cache_size_str = console_params.at("-cache_size");
  string gmx_volume_size_str = console_params.at("-gmx_volume_size");
  string gdal_resampling = console_params.at("-resampling");
  string temp_file_warp_path = console_params.at("-temp_file_warp");
  string max_work_threads_str = console_params.at("-work_threads");
  string max_warp_threads_str = console_params.at("-warp_threads");


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
    tiling_params.max_cache_size_ = atof(cache_size_str.c_str());
  
  if (gmx_volume_size_str!="") 
    tiling_params.max_gmx_volume_size_ = atof(gmx_volume_size_str.c_str());


  if (quality_str!="")
    tiling_params.jpeg_quality_ = (int)atof(quality_str.c_str());
  
  
  if (nodata_str!="")
  {
    BYTE	rgb[3];
    if (!gmx::ConvertStringToRGB(nodata_str,rgb))
    {
      cout<<"ERROR: bad value of parameter: \"-nodata\""<<endl;
      return FALSE;
    }
    tiling_params.p_transparent_color_ = new BYTE[3];
    memcpy(tiling_params.p_transparent_color_,rgb,3);
  }

  if (nodata_tolerance_str != "")
  {
    if (((int)atof(nodata_tolerance_str.c_str()))>=0 && ((int)atof(nodata_tolerance_str.c_str()))<=100)
      tiling_params.nodata_tolerance_ = (int)atof(nodata_tolerance_str.c_str());
  }

  if (background_str!="")
  {
    BYTE	rgb[3];
    if (!gmx::ConvertStringToRGB(background_str,rgb))
    {
      cout<<"ERROR: bad value of parameter: \"-background\""<<endl;
      return FALSE;
    }
    tiling_params.p_background_color_ = new BYTE[3];
    memcpy(tiling_params.p_background_color_,rgb,3);
  }

  tiling_params.gdal_resampling_ = gdal_resampling;

  tiling_params.auto_stretch_to_8bit_ = true;

  tiling_params.temp_file_path_for_warping_ = temp_file_warp_path;

  tiling_params.calculate_histogram_ = true;

  if (max_work_threads_str != "")
    tiling_params.max_work_threads_ = ((int)atof(max_work_threads_str.c_str())>0) ? (int)atof(max_work_threads_str.c_str()) : 0;

  if (max_warp_threads_str != "")
    tiling_params.max_warp_threads_ = ((int)atof(max_warp_threads_str.c_str())>0) ? (int)atof(max_warp_threads_str.c_str()) : 0;

  return GMXMakeTiling(&tiling_params);
  //if (logFile) fclose(logFile);
}



int _tmain(int argc, wchar_t* argvW[])
{

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


  map<string,string> console_params;
  
  console_params.insert(pair<string,string>("-file",gmx::ReadConsoleParameter("-file",argc,argv)));
  console_params.insert(pair<string,string>("-mosaic",gmx::ReadConsoleParameter("-mosaic",argc,argv,TRUE)));
  console_params.insert(pair<string,string>("-gmxtiles",gmx::ReadConsoleParameter("-container",argc,argv,TRUE)));
  console_params.insert(pair<string,string>("-mbtiles",gmx::ReadConsoleParameter("-mbtiles",argc,argv,TRUE)));
  console_params.insert(pair<string,string>("-zoom",gmx::ReadConsoleParameter("-zoom",argc,argv)));
  console_params.insert(pair<string,string>("-min_zoom",gmx::ReadConsoleParameter("-min_zoom",argc,argv)));

  console_params.insert(pair<string,string>("-border",gmx::ReadConsoleParameter("-border",argc,argv)));
  console_params.insert(pair<string,string>("-tiles",gmx::ReadConsoleParameter("-tiles",argc,argv)));
  console_params.insert(pair<string,string>("-tile_type",gmx::ReadConsoleParameter("-tile_type",argc,argv)));
  console_params.insert(pair<string,string>("-proj",gmx::ReadConsoleParameter("-proj",argc,argv)));

  console_params.insert(pair<string,string>("-template",gmx::ReadConsoleParameter("-template",argc,argv)));
  console_params.insert(pair<string,string>("-nodata",gmx::ReadConsoleParameter("-nodata",argc,argv) != "" ?
                                                          gmx::ReadConsoleParameter("-nodata",argc,argv) :
                                                          gmx::ReadConsoleParameter("-nodata_rgb",argc,argv)));
   console_params.insert(pair<string,string>("-nodata_tolerance",gmx::ReadConsoleParameter("-nodata_tolerance",argc,argv)));
  console_params.insert(pair<string,string>("-quality",gmx::ReadConsoleParameter("-quality",argc,argv)));
  console_params.insert(pair<string,string>("-resampling",gmx::ReadConsoleParameter("-resampling",argc,argv)));
  console_params.insert(pair<string,string>("-background",gmx::ReadConsoleParameter("-background",argc,argv)));
  
  console_params.insert(pair<string,string>("-shiftX",gmx::ReadConsoleParameter("-shiftX",argc,argv)));
  console_params.insert(pair<string,string>("-shiftY",gmx::ReadConsoleParameter("-shiftY",argc,argv)));
  console_params.insert(pair<string,string>("-cache_size",gmx::ReadConsoleParameter("-cache_size",argc,argv)));
  console_params.insert(pair<string,string>("-gmx_volume_size",gmx::ReadConsoleParameter("-gmx_volume_size",argc,argv)));
  console_params.insert(pair<string,string>("-temp_file_warp",gmx::ReadConsoleParameter("-temp_file_warp",argc,argv)));
  console_params.insert(pair<string,string>("-log_file",gmx::ReadConsoleParameter("-log_file",argc,argv)));
  console_params.insert(pair<string,string>("-pixel_tiling",gmx::ReadConsoleParameter("-pixel_tiling",argc,argv,TRUE)));
  console_params.insert(pair<string,string>("-work_threads",gmx::ReadConsoleParameter("-work_threads",argc,argv)));
  console_params.insert(pair<string,string>("-warp_threads",gmx::ReadConsoleParameter("-warp_threads",argc,argv)));


  
  if (argc == 2)
     console_params.at("-file") = argv[1];

  //wstring fileW = L"C:\\share_upload\\Иванова Ирина.jpg";
  //gmx::wstrToUtf8(console_params.at("-file"),fileW);
  //console_params.at("-gmxtiles")="-container";
  
  //console_params.at("-file") = "C:\\Work\\Projects\\TilingTools\\autotest\\scn_120719_Vrangel_island_SWA.tif";
  //console_params.at("-tile_type") = "jp2";
  //console_params.at("-quality") = "0";
 // console_params.at("-tiles") = "E:\\test_images\\L8\\for_test\\rgb\\LC81120742014154LGN00_jp2.tiles";
 //console_params.at("-gmxtiles")="-container";
 //console_params.at("-zoom") = "8";

  //console_params.at("-nodata_tolerance") = "0";
  //console_params.at("-nodata") = "ffffff";

  //console_params.at("-tiles") = "\\\\rum-potanin\\share_upload\\L8_NDVI\\bugs\\LC81750282014083LGN00_ndvi_tiles5";
  //console_params.at("-template")="standard";
  //console_params.at("-resampling")="nearest";
  //console_params.at("-zoom") = "5";

  //console_params.at("-gmxtiles")="-container";
  // -gmx_volume_size 1000000 -cache_size 1000000 -resampling nearest
  //console_params.at("-gmx_volume_size")="1000000";
  //console_params.at("-cache_size")="1000000";
  //console_params.at("-tiles") = "E:\\test_images\\Arctic.2014142.terra.1km_z5_3.tiles";
  //console_params.at("-zoom") = "5";

  //C:\Work\Projects\TilingTools\Release\imagetiling -file C:\Work\Projects\TilingTools\autotest\scn_120719_Vrangel_island_SWA.tif -container -tiles C:\Work\Projects\TilingTools\autotest\result\scn_120719_Vrangel_island_SWA.tiles -zoom 8 -cache 10 -resampling cubic

  //console_params.at("-file") = "C:\\share_upload\\foto\\Voronina Marina.jpg";
  //console_params.at("-gmxtiles")="-container";

   //console_params.at("-border") = "E:\\test_images\\L8\\LC80930132014036LGN00.shp";
  //console_params.at("-nodata_rgb") = "0 0 0";
  //console_params.at("-tile_type") = "png";



  if (console_params.at("-file") == "")
  {
    cout<<"ERROR: missing \"-file\" parameter"<<endl;
    return 1;
  }


  wcout<<endl;
  if (console_params.at("-mosaic")=="")
  {
    std::list<string> input_file_list;
    if (!gmx::FindFilesInFolderByPattern (input_file_list,console_params.at("-file")))
    {
      cout<<"Can't find input files by pattern: "<<console_params.at("-file")<<endl;
      return 1;
    }

    BOOL use_container = (console_params.at("-gmxtiles")!="" || console_params.at("-mbtiles")!="");
    
    for (std::list<string>::iterator iter = input_file_list.begin(); iter!=input_file_list.end();iter++)
    {
      cout<<"Tiling file: "<<(*iter)<<endl;
      
      if (input_file_list.size()>1)
      {
        if (console_params.at("-border") == "") 
          console_params.at("-border") = gmx::VectorBorder::GetVectorFileNameByRasterFileName(*iter);
      }
      
      string ouput_path_init = console_params.at("-tiles");
      if (input_file_list.size()>1 && console_params.at("-tiles")!="")
      {
        if (!gmx::FileExists(console_params.at("-tiles"))) 
        {
          if (!gmx::CreateDirectory(console_params.at("-tiles").c_str()))
          {
            cout<<"ERROR: can't create directory: "<<console_params.at("-tiles")<<endl;
            return 1;
          }
        }        

        if (use_container)
        {
          console_params.at("-tiles") = (console_params.at("-mbtiles") != "") ? 
                                        gmx::RemoveEndingSlash(console_params.at("-tiles")) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +".mbtiles" :
                                        gmx::RemoveEndingSlash(console_params.at("-tiles")) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +".tiles"; 
        }
        else
        {
          console_params.at("-tiles") = gmx::RemoveEndingSlash(console_params.at("-tiles")) + "/" + gmx::RemovePath(gmx::RemoveExtension(*iter)) +"_tiles"; 
        }
      }
      
      console_params.at("-file") = (*iter);
   
      if ((!CheckArgsAndCallTiling (console_params)) && input_file_list.size()==1) 
        return 2;
           
      wcout<<endl;
      console_params.at("-tiles") = ouput_path_init;
    }
  }
  else
  {
    if (! CheckArgsAndCallTiling (console_params))
     return 2;
  }

  return 0;
  
}