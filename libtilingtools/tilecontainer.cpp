#include "tilecontainer.h"

namespace gmx
{

GMXTileContainer* GMXTileContainer::OpenForWriting (TileContainerOptions *p_params)
{
  if (!p_params) return 0;
  if (p_params->max_zoom_<0) return 0;
  if (p_params->path_=="") return 0;
  if (!p_params->p_matrix_set_) return 0;
  if (p_params->tiling_srs_envp_.MaxX<p_params->tiling_srs_envp_.MinX) return 0;
  if (p_params->tile_type_==TileType::NDEF_TILE_TYPE) return 0;

  GMXTileContainer* p_gmx_tc = new GMXTileContainer();
  
  int cache_size = p_params->extra_options_["cache_size"]!="" ?
                    atoi(p_params->extra_options_["cache_size"].c_str()) :
                    -1;
  int max_volume_size = p_params->extra_options_["volume_size"]!="" ?
                     atoi(p_params->extra_options_["volume_size"].c_str()) :
                      GMXTileContainer::DEFAULT_MAX_VOLUME_SIZE;
  MercatorProjType merc_type = ((MercatorTileMatrixSet*)p_params->p_matrix_set_)->merc_type();
  
  if (!p_gmx_tc->OpenForWriting(p_params->path_,
                                p_params->tile_type_,
                                merc_type,
                                p_params->tiling_srs_envp_,
                                p_params->max_zoom_,
                                cache_size,
                                max_volume_size))
  {
    delete(p_gmx_tc);
    return 0;
  }
  return p_gmx_tc;
};


bool GMXTileContainer::OpenForWriting	(	string				container_file_name, 
										TileType			tile_type,
										MercatorProjType	merc_type,
										OGREnvelope			envelope, 
										int					max_zoom, 
										int				cache_size,
                    unsigned int max_volume_size)
{
	int tile_bounds[128];
	for (int i=0;i<32;i++)
	{
		tile_bounds[4*i] = (tile_bounds[4*i+1] = (tile_bounds[4*i+2] = (tile_bounds[4*i+3] = -1)));
		if (i<=max_zoom)
		{
      MercatorTileMatrixSet merc_grid(merc_type);
			merc_grid.CalcTileRange(envelope,i,tile_bounds[4*i],tile_bounds[4*i+1],tile_bounds[4*i+2],tile_bounds[4*i+3]);
		}
	}
	return OpenForWriting(container_file_name,tile_type,merc_type,tile_bounds,cache_size,max_volume_size);
};


GMXTileContainer::GMXTileContainer	() 
{
  max_tiles_		= 0;
	p_sizes_			= NULL; 
	p_offsets_		= NULL;
	p_tile_cache_	= NULL;
	use_cache_	  = FALSE;
  is_opened_    = FALSE;
  p_metadata_=0;
  max_volumes_ = 1000;
  pp_container_volumes_ = NULL;
};	


bool GMXTileContainer::OpenForReading (string container_file_name)
{
	if (is_opened_ && !read_only_) Close(); 
  MakeEmpty();
	
  pp_container_volumes_ = new FILE*[max_volumes_];
  for (int i=0;i<max_volumes_;i++)
    pp_container_volumes_[i]=NULL;


  if (!(pp_container_volumes_[0] = GMXFileSys::OpenFile(container_file_name,"rb"))) return FALSE;
  container_file_name_ = container_file_name;
	char	head[12];
	fread(head,1,12,pp_container_volumes_[0]);
	if (!((head[0]=='G')&&(head[1]=='M')&&(head[2]=='T')&&(head[3]=='C'))) 
	{
		cout<<"ERROR: incorrect input tile container file: "<<container_file_name<<endl;
    MakeEmpty();
		return FALSE;
	}
	
  memcpy(&max_volume_size_,&head[4],4);
  if (max_volume_size_ == 65535) max_volume_size_ = 0;
	
  merc_type_ = (head[9] == 0) ? WORLD_MERCATOR : WEB_MERCATOR;
	tile_type_ = (TileType)head[11];
  use_cache_ = FALSE;
	p_tile_cache_ = NULL;

	char bounds[512];
	fread(bounds,1,512,pp_container_volumes_[0]);

	max_tiles_ = 0;
	for (int i=0;i<32;i++)
	{
		minx_[i] = *((int*)(&bounds[i*16]));
		miny_[i] = *((int*)(&bounds[i*16+4]));
		maxx_[i] = *((int*)(&bounds[i*16+8]));
		maxy_[i] = *((int*)(&bounds[i*16+12]));
		//if (maxx_[i]>0) 	this->max_zoom = i;
		max_tiles_ += (maxx_[i]-minx_[i])*(maxy_[i]-miny_[i]);
	}

	unsigned char* offset_size = new unsigned char[max_tiles_*13];
	fread(offset_size,1,max_tiles_*13,pp_container_volumes_[0]);

  p_sizes_		= new unsigned int[max_tiles_];
	p_offsets_		= new uint64_t[max_tiles_];

	for (int i=0; i<max_tiles_;i++)
	{
		p_offsets_[i]	= *((uint64_t*)(&offset_size[i*13]));
		if ((p_offsets_[i]<<32) == 0)
			p_offsets_[i]	= (p_offsets_[i]>>32);
		p_sizes_[i]	= *((unsigned int*)(&offset_size[i*13+8]));
	}

	delete[]offset_size;

  is_opened_    = TRUE;
  read_only_		= TRUE;
	return TRUE;
};
	

GMXTileContainer::~GMXTileContainer()
{
	MakeEmpty();
}
	
//fclose(containerFileData);

Metadata* GMXTileContainer::GetMetadata ()
{
  Metadata *p_metadata = NULL;
  if (!read_only_ || !is_opened_) return NULL;
  
  GMXFileSys::Fseek64(pp_container_volumes_[0],10,SEEK_SET);
  char num_tags;
  if (fread(&num_tags,1,1,pp_container_volumes_[0])!=1) return NULL;
  if (num_tags==0) return NULL;

  p_metadata = new Metadata();
  GMXFileSys::Fseek64(pp_container_volumes_[0],524 + 13*max_tiles_,SEEK_SET);
  char buf[5];
  buf[4]=0;

  for (int i=0;i<num_tags;i++)
  {
    fread(buf,1,4,pp_container_volumes_[0]);
    string tag_name(buf);
    if (tag_name.size()!=4) break;
    unsigned int tag_size;
    fread(&tag_size,1,4,pp_container_volumes_[0]);
    if (tag_size == 0) continue;
    char *tag_data = new char[tag_size];
    fread(tag_data,1,tag_size,pp_container_volumes_[0]);

    Metatag *p_metatag = Metadata::DeserializeTag(tag_name,tag_size,tag_data);
    if (p_metatag) p_metadata->AddTagDirectly(p_metatag);
  }

  return (p_metadata->TagCount()) ? p_metadata : NULL;
}


bool GMXTileContainer::ExtractAndStoreMetadata (TilingParameters* p_params) 
{
  //ToDo
  /*
  //from GMXMakeTiling
   Metadata metadata;
  MetaHistogram histogram;
  MetaNodataValue nodata_value;
  MetaHistogramStatistics hist_stat;
  bool nodata_defined = false;
  double  nodata_val;
  if (p_tiling_params->p_transparent_color_)
  {
    nodata_defined = true;
    nodata_val = p_tiling_params->p_transparent_color_[0];
  }
  else nodata_val = raster_bundle.GetNodataValue(nodata_defined);
  if (nodata_defined) nodata_value.nodv_ = nodata_val;
  
  if (histogram.IsInitiated())
    histogram.CalcStatistics(&hist_stat,(nodata_defined) ? &nodata_val : 0); 
  
  */

  //from GMXAsyncWarpChunkAndMakeTiling
  /*
  gmx::MetaHistogram        *p_histogram = p_chunk_tiling_params->p_histogram_;
  if (p_histogram) 
  {
    if (!p_histogram->IsInitiated())
      p_histogram->Init(p_merc_buffer->get_num_bands(),p_merc_buffer->get_data_type());
    p_merc_buffer->AddPixelDataToMetaHistogram(p_histogram);
  }
  */

  //from RasterBuffer
  /*
  template <typename T>	
bool    RasterBuffer::AddPixelDataToMetaHistogram(T type, MetaHistogram *p_hist)
{
  T *p_pixel_data_t = (T*)p_pixel_data_;
  unsigned int n = x_size_*y_size_;
  unsigned int m = 0;

  int _num_bands = IsAlphaBand() ? num_bands_-1 : num_bands_;
  for (int i=0;i<y_size_;i++)
  {
    for (int j=0;j<x_size_;j++)
    {
      for (int k=0;k<_num_bands;k++)
      {
        p_hist->AddValue(k,p_pixel_data_t[k*n+m]);
      }
      m++;
    }
  }
  return TRUE;
}
  */

  return true;
}


bool		GMXTileContainer::AddTile(int z, int x, int y, unsigned char *p_data, unsigned int size)
{
  if (read_only_ || !is_opened_) return FALSE;
  int64_t n	= TileID(z,x,y);
  if ((n>= max_tiles_)||(n<0)) return FALSE;
  
  addtile_mutex_.lock(); 

	if (p_tile_cache_)
	{
    if (p_tile_cache_->AddTile(z,x,y,p_data,size))
    {
      p_sizes_[n]		= size;
      addtile_mutex_.unlock();
      return TRUE;
    }
	}
	
  bool result = AddTileToContainerFile(z,x,y,p_data,size);
  addtile_mutex_.unlock();

  return result;
};

bool		GMXTileContainer::GetTile(int z, int x, int y, unsigned char *&p_data, unsigned int &size)
{
  if (!is_opened_) return FALSE;

	if (p_tile_cache_)	
  {
    if (p_tile_cache_->GetTile(z,x,y,p_data,size)) return TRUE;
  }
	
  return GetTileFromContainerFile(z,x,y,p_data,size);
};

bool		GMXTileContainer::TileExists(int z, int x, int y)
{
  if (!is_opened_) return FALSE;

	unsigned int n = TileID(z,x,y);
	if (n>= max_tiles_ || n<0) return FALSE;
	return (p_sizes_[n]>0) ? TRUE : FALSE;
}; 


bool		GMXTileContainer::Close()
{
  if (!is_opened_) return TRUE;
  else if (read_only_) MakeEmpty();
  else
	{
    if (p_tile_cache_) WriteTilesToContainerFileFromCache();
    //_fseeki64(pp_container_volumes_[0],0,0);
    GMXFileSys::Fseek64(pp_container_volumes_[0], 0, 0);
    unsigned char* header;
    this->WriteHeaderToByteArray(header);
    fwrite(header,1,HeaderSize(),pp_container_volumes_[0]);
    delete[]header;
    MakeEmpty();
  }
   
	return TRUE;		
};


bool		GMXTileContainer::GetTileBounds (int tile_bounds[128])
{
	if (max_tiles_<=0) return FALSE;
	if (maxx_==0 || maxy_==0 || minx_==0 || miny_==0) return FALSE;
	for (int z=0;z<32;z++)
	{
		if ((maxx_[z]==0)||(maxy_[z]==0)) tile_bounds[4*z] = (tile_bounds[4*z+1] = (tile_bounds[4*z+2] = (tile_bounds[4*z+3] =-1)));
		else 
		{
			tile_bounds[4*z]		= minx_[z];
			tile_bounds[4*z+1]	= miny_[z];
			tile_bounds[4*z+2]	= maxx_[z]-1;
			tile_bounds[4*z+3]	= maxy_[z]-1;
		}
	}
	return TRUE;
}



int 		GMXTileContainer::GetTileList(list<pair<int, pair<int,int>>> &tile_list, 
											int min_zoom, 
											int max_zoom, 
											string vector_file
											)
{
  
	OGRGeometry *p_border = NULL;
  if (!is_opened_) return 0;
  MercatorTileMatrixSet merc_grid(this->merc_type_);
  if (vector_file!="") 
	{
    if( !(p_border = VectorOperations::ReadAndTransformGeometry(vector_file,merc_grid.GetTilingSRSRef()))) 
		{
			cout<<"ERROR: can't open vector file: "<<vector_file<<endl;
			return 0;
		}
	  if (!merc_grid.AdjustIfOverlap180Degree(p_border))
    {
      cout<<"ERROR: AdjustIfOverlapAbscissa fail"<<endl;
      delete(p_border);
      return 0;
    }
  }

 	for (unsigned int i=0; i<max_tiles_;i++)
	{
		int x,y,z;
    pair<int,pair<int,int>> p;
		if (p_sizes_[i]>0) 
		{
			if(!TileXYZ(i,z,x,y)) continue;
			if ((max_zoom>=0)&&(z>max_zoom)) continue;
			if ((min_zoom>=0)&&(z<min_zoom)) continue;
			if (vector_file!="")
      {
        OGRPolygon *p_envp = VectorOperations::CreateOGRPolygonByOGREnvelope(merc_grid.CalcEnvelopeByTile(z,x,y));
        if (!p_border->Intersects(p_envp))
        {
          delete(p_envp);
          continue;
        }
        delete(p_envp);
      }

      p.first =z;
      p.second.first=x;
      p.second.second=y;
      tile_list.push_back(p);
		}
	}

	delete(p_border);
	return tile_list.size();	
};
	
int64_t		GMXTileContainer::TileID( int z, int x, int y)
{
  if (!is_opened_) return -1;
  if ((x<minx_[z]) || (x>=maxx_[z]) || (y<miny_[z]) || (y>=maxy_[z])) return -1;

	unsigned int num = 0;
	for (int s=0;s<z;s++)
		num+=(maxx_[s]-minx_[s])*(maxy_[s]-miny_[s]);
	return (num + (maxx_[z]-minx_[z])*(y-miny_[z]) + x-minx_[z]);
};

bool		GMXTileContainer::TileXYZ(int64_t n, int &z, int &x, int &y)
{
  if (!is_opened_) return FALSE;

	if ((max_tiles_>0) && (n>=max_tiles_)) return FALSE; 
	int s = 0;
	for (z=0;z<32;z++)
	{
		if ((s+(maxx_[z]-minx_[z])*(maxy_[z]-miny_[z]))>n) break;
		s+=(maxx_[z]-minx_[z])*(maxy_[z]-miny_[z]);
	}
	y = miny_[z] + ((n-s)/(maxx_[z]-minx_[z]));
	x = minx_[z] + ((n-s)%(maxx_[z]-minx_[z]));

	return TRUE;
};

TileType	GMXTileContainer::GetTileType()
{
	return tile_type_;
};

MercatorProjType	GMXTileContainer::GetProjType()
{
	return merc_type_;
};


int			GMXTileContainer::GetMaxZoom()
{
	int max_z = -1;
	for (int z=0;z<32;z++)
		if (maxx_[z]>0) max_z = z;
	return max_z;
};

bool 	GMXTileContainer::OpenForWriting	(	string				container_file_name, 
									                        TileType			tile_type,
									                        MercatorProjType	merc_type,
                                          int					tile_bounds[128], 
									                        int 				cache_size,
                                          unsigned int max_volume_size)
{
  if (is_opened_ && !read_only_) Close(); 
  MakeEmpty();

  is_opened_            = TRUE;
	read_only_					  = FALSE;
	tile_type_					  = tile_type;
	merc_type_					  = merc_type;
	container_file_name_	= container_file_name;
 	container_byte_size_	= 0;

  pp_container_volumes_ = new FILE*[max_volumes_];
  for (int i=0;i<max_volumes_;i++)
    pp_container_volumes_[i]=NULL;
  
  max_volume_size_	= max_volume_size;
  

  if (!DeleteVolumes()) return FALSE;
    
  if (! (pp_container_volumes_[0] = GMXFileSys::OpenFile(container_file_name_.c_str(),"wb+")))
  {
    MakeEmpty();
    return FALSE;
  }

	if (cache_size!=0) 
	{
		use_cache_	= TRUE;
		p_tile_cache_	= (cache_size<0) ? new TileCache() : new TileCache(cache_size);
	}
	else 
	{
		use_cache_	= FALSE;
		p_tile_cache_	= NULL;
	}

	p_sizes_		= NULL;
	p_offsets_		= NULL;
	int max_zoom = 0;
	for (int z =0; z<32;z++)
	{
		if (tile_bounds[4*z+2]>=0)
		{
			max_zoom = z;
			minx_[z] = tile_bounds[4*z];
			miny_[z] = tile_bounds[4*z+1];
			maxx_[z] = tile_bounds[4*z+2]+1;
			maxy_[z] = tile_bounds[4*z+3]+1;
		}
		else minx_[z] = (miny_[z] = (maxx_[z] = (maxy_[z] = 0)));
	}

	max_tiles_ = TileID(max_zoom,maxx_[max_zoom]-1,maxy_[max_zoom]-1) + 1;

	p_sizes_		= new unsigned int[max_tiles_];
	p_offsets_		= new uint64_t[max_tiles_];
	for (unsigned int i=0;i<max_tiles_;i++)
	{
		p_sizes_[i]		= 0;
		p_offsets_[i]		= 0;
	}
  
	return TRUE;
};

int GMXTileContainer::FillUpCurrentVolume ()
{

  int volume_num = GetVolumeNum(container_byte_size_);
  //_fseeki64(pp_container_volumes_[volume_num],0,SEEK_END);
  GMXFileSys::Fseek64(pp_container_volumes_[volume_num], 0, SEEK_END);
  int block_size = max_volume_size_ - (container_byte_size_ % max_volume_size_);
  unsigned char *block = new unsigned char[block_size];
  for (int i=0;i<block_size;i++)
    block[i] = 0;
  fwrite(block,1,block_size,pp_container_volumes_[volume_num]);
  delete[]block;
  return block_size;
};

bool   GMXTileContainer::DeleteVolumes()
{
  list<string> file_list;

  GMXFileSys::FindFilesByPattern(file_list,container_file_name_ +"*");
    
  regex pattern("\\.{0,1}[0-9]{0,3}");
  for (list<string>::iterator iter = file_list.begin(); iter!=file_list.end(); iter++)
  {
    if ((*iter).length()>container_file_name_.length())
    {
      if (regex_match((*iter).substr(container_file_name_.length(),(*iter).length()-container_file_name_.length()),pattern))
      {      
        if (! GMXFileSys::FileDelete(*iter))
        {
          cout<<"ERROR: can't delete file: "<<*iter<<endl;
          return FALSE;
        }
      }
    }
  }
  return TRUE;
}


int  GMXTileContainer::GetVolumeNum (uint64_t tile_offset)
{
  return (max_volume_size_ == 0) ? 0 : (tile_offset/max_volume_size_);
}

uint64_t  GMXTileContainer::GetTileOffsetInVolume (uint64_t tile_container_offset)
{
  return tile_container_offset - GetVolumeNum(tile_container_offset)*((uint64_t)max_volume_size_);
}


string GMXTileContainer::GetVolumeName (int num)
{
  return (num == 0) ? container_file_name_ : container_file_name_ + "." + GMXString::ConvertIntToString(num+1);
}


bool	GMXTileContainer::AddTileToContainerFile(int z, int x, int y, unsigned char *p_data, unsigned int size)
{
	//ToDo - fix if n<0
  if (this->read_only_) return FALSE;
  
  unsigned int n	= TileID(z,x,y);
	if (n>= max_tiles_) return FALSE;

  if (container_byte_size_ == 0)        //first tile
  {
    unsigned char* header = new unsigned char[HeaderSize()];
    if (!header) return NULL;
    for (int i=0;i<HeaderSize();i++)
      header[i] = 0;
    fwrite(header,1,HeaderSize(),pp_container_volumes_[0]);
	  delete[]header;
    container_byte_size_ = HeaderSize();
  }
  
  if (max_volume_size_ > 0)
  {
    if ((container_byte_size_ /max_volume_size_) < ((container_byte_size_ + size - 1)/max_volume_size_))
        container_byte_size_ += FillUpCurrentVolume();
  }
  
  int volume_num = GetVolumeNum(container_byte_size_);
  if (pp_container_volumes_[volume_num] == NULL)
  {
    if (!(pp_container_volumes_[volume_num] = GMXFileSys::OpenFile(GetVolumeName(volume_num).c_str(),"wb+")))
      return FALSE;
  }

  //_fseeki64(pp_container_volumes_[volume_num],0,SEEK_END);
  GMXFileSys::Fseek64(pp_container_volumes_[volume_num], 0, SEEK_END);
  fwrite(p_data,sizeof(char),size,pp_container_volumes_[volume_num]);
  
  p_offsets_[n] = container_byte_size_;
  p_sizes_[n]		= size;
  container_byte_size_ +=size;
  
  return TRUE;
};
	
bool	GMXTileContainer::GetTileFromContainerFile (int z, int x, int y, unsigned char *&p_data, unsigned int &size)
{
	unsigned int n	= TileID(z,x,y);
	if (n>= max_tiles_ || n<0) return FALSE;
	if (!(size = p_sizes_[n])) return FALSE;

  int volume_num = GetVolumeNum(p_offsets_[n]);
  if (!pp_container_volumes_[volume_num])
  {
    if (!(pp_container_volumes_[volume_num] = GMXFileSys::OpenFile(GetVolumeName(volume_num).c_str(),"rb")))
    {
      cout<<"ERROR: can't open file: "<<GetVolumeName(volume_num).c_str()<<endl;
      return FALSE;
    }
  }

  //_fseeki64(pp_container_volumes_[volume_num],GetTileOffsetInVolume(p_offsets_[n]),0);
  GMXFileSys::Fseek64(pp_container_volumes_[volume_num], GetTileOffsetInVolume(p_offsets_[n]), 0);
  p_data			= new unsigned char[size];
	return (size==fread(p_data,1,size,pp_container_volumes_[volume_num]));
};

bool	GMXTileContainer::WriteTilesToContainerFileFromCache()
{
  if (container_byte_size_ == 0 && (p_tile_cache_->cache_size() + HeaderSize() < max_volume_size_) )
	{
		unsigned char* header = new unsigned char[HeaderSize()];
    for (int i=0;i<HeaderSize();i++)
      header[i] = 0;
    fwrite(header,1,HeaderSize(),pp_container_volumes_[0]);
		delete[]header;
    container_byte_size_ = HeaderSize();
    		
    int k=0;
 	  unsigned char	*tile_data = 0;
    int max_tiles_in_block = 1000;
    unsigned int tile_size;
    int x,y,z;

    while (k<max_tiles_)
    {
      int n=k;
      int block_size = 0;
      int tiles_in_block=0;
      while ((tiles_in_block<max_tiles_in_block)&&(n<max_tiles_))
      {
        this->TileXYZ(n,z,x,y);
        if (p_tile_cache_->GetTile(z,x,y,tile_data,tile_size))
        {
          block_size+=tile_size;
          tiles_in_block++;
          delete[]tile_data;
        }
        n++;
      }
    
      if (block_size==0) break;
    
      unsigned char *p_block = new unsigned char[block_size];
      n=k;
      tiles_in_block =0;
      block_size =0;
      while ((tiles_in_block<max_tiles_in_block)&&(n<max_tiles_))
      {
        this->TileXYZ(n,z,x,y);
        if (p_tile_cache_->GetTile(z,x,y,tile_data,tile_size))
        {
          memcpy(p_block+block_size,tile_data,tile_size);
          delete[]tile_data;
          p_offsets_[n]=container_byte_size_+block_size;
          block_size+=tile_size;
          tiles_in_block++;
        }
        n++;
      }

      if (!fwrite(p_block,1,block_size,pp_container_volumes_[0]))
        return FALSE;
      container_byte_size_+=block_size;
		  delete[]p_block;
      k=n;
    }
  }
  else
  {
    int x,y,z;
    unsigned char* tile_data = 0;
    unsigned int tile_size;

    for (int n=0;n<max_tiles_;n++)
    {
      TileXYZ(n,z,x,y);
      if (p_tile_cache_->GetTile(z,x,y,tile_data,tile_size))
      {
        if (!this->AddTileToContainerFile(z,x,y,tile_data,tile_size))
        {
          cout<<"ERROR: can't write tile to container volume"<<endl;
          return FALSE;
        }
      }
    }
  }
	
  return TRUE;
};



bool	GMXTileContainer::WriteHeaderToByteArray(unsigned char*	&p_data)
{
	p_data = new unsigned char[HeaderSize()];
	string file_type = "GMTC";
	memcpy(p_data,file_type.c_str(),4);
	memcpy(&p_data[4],&max_volume_size_,4);
	p_data[8]	= 0;
	p_data[9]	= merc_type_;
  p_data[10]	= (p_metadata_) ? p_metadata_->TagCount() : 0;
	p_data[11]	= tile_type_;

	for (int z=0;z<32;z++)
	{
		memcpy(&p_data[12+z*16],&minx_[z],4);
		memcpy(&p_data[12+z*16+4],&miny_[z],4);
		memcpy(&p_data[12+z*16+8],&maxx_[z],4);
		memcpy(&p_data[12+z*16+12],&maxy_[z],4);
	}
	//int t[36] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	//memcpy(&p_data[12+32*16],t,144);

	char	tile_info[13];
  for (unsigned int i = 0; i<max_tiles_; i++)
	{
		if (p_sizes_[i]==0)
		{
			for (int j = 0; j<13;j++)
				tile_info[j] = 0;
		}
		else
		{
			memcpy(&tile_info,&p_offsets_[i],8);
			memcpy(&tile_info[8],&p_sizes_[i],4);
			tile_info[12]	=0;
		}
		memcpy(&p_data[12 + 512 + 13*i],tile_info,13);
	}

  if (p_metadata_)
  {
    int m_size;
    void *pm_data = 0;
    if (p_metadata_->GetAllSerialized(m_size,pm_data))
    {
      memcpy(&p_data[12 + 512 + 13*max_tiles_],pm_data,m_size);
    }
    delete[]((unsigned char*)pm_data);
  }

	return TRUE;
};

	
unsigned int GMXTileContainer::HeaderSize()
{
  return (4+4+4+512+max_tiles_*(4+8+1)) + ((p_metadata_) ? p_metadata_->GetAllSerializedSize() : 0);
};

void GMXTileContainer::MakeEmpty ()
{
  is_opened_    = FALSE;

  delete[]p_sizes_;
	p_sizes_ = NULL;
	delete[]p_offsets_;
	p_offsets_ = NULL;
	max_tiles_ = 0;
	
 
  //addtile_mutex_.unlock();
  
	for (int i=0;i<32;i++)
		maxx_[i]=(minx_[i]=(maxy_[i]=(miny_[i]=0)));
	delete(p_tile_cache_);
	p_tile_cache_ = NULL;

  if (pp_container_volumes_)
  {
    for (int i=0;i<max_volumes_;i++)
      if (pp_container_volumes_[i]) fclose(pp_container_volumes_[i]);
    delete[]pp_container_volumes_;
    pp_container_volumes_ = NULL;
  }

  if (p_metadata_) delete(p_metadata_);
  p_metadata_=0;
};

///*
RasterBuffer* GMXTileContainer::CreateWebMercatorTileFromWorldMercatorTiles(
										GMXTileContainer *poSrcContainer,
										int nZ, int nX, int nY)
{
	MercatorTileMatrixSet oWebMercGrid(WEB_MERCATOR);
	double dblRes = oWebMercGrid.CalcPixelSizeByZoom(nZ);
	OGREnvelope oWebMercEnvp = oWebMercGrid.CalcEnvelopeByTile(nZ, nX, nY);
	OGREnvelope oWorldMercEnvp;
	oWorldMercEnvp.MinX = oWebMercEnvp.MinX;
	oWorldMercEnvp.MaxX = oWebMercEnvp.MaxX;

	oWorldMercEnvp.MaxY = MercatorTileMatrixSet::MercY(
		MercatorTileMatrixSet::MercToLat(oWebMercEnvp.MaxY, WEB_MERCATOR),
						WORLD_MERCATOR);
	oWorldMercEnvp.MinY = MercatorTileMatrixSet::MercY(
		MercatorTileMatrixSet::MercToLat(oWebMercEnvp.MinY, WEB_MERCATOR),
						WORLD_MERCATOR);

	MercatorTileMatrixSet oWorldMercGrid(WORLD_MERCATOR);
	oWorldMercEnvp.MinX += 1e-05;
	oWorldMercEnvp.MaxX -= 1e-05;
	oWorldMercEnvp.MaxY -= 1e-05;
	oWorldMercEnvp.MinY += 1e-05;

	int nWorldMercUpperTileY, nWorldMercLowerTileY;
	oWorldMercGrid.CalcTileRange(oWorldMercEnvp, nZ, nX, nWorldMercLowerTileY, nX, nWorldMercUpperTileY);

	unsigned int nSize;
	unsigned char* pabData;

	if (!poSrcContainer->GetTile(nZ, nX, nWorldMercUpperTileY, pabData, nSize)) return 0;
	RasterBuffer oInputupperTileBuffer;
	oInputupperTileBuffer.CreateBufferFromInMemoryData(pabData, nSize, poSrcContainer->GetTileType());
	delete[]pabData;
		
	if (!poSrcContainer->GetTile(nZ, nX, nWorldMercLowerTileY, pabData, nSize)) return 0;
	RasterBuffer oInputLowerTileBuffer;
	oInputLowerTileBuffer.CreateBufferFromInMemoryData(pabData, nSize, poSrcContainer->GetTileType());
	delete[]pabData;

	RasterBuffer* poOutputTileBuffer;
	poOutputTileBuffer->CreateBuffer(&oInputupperTileBuffer);
	poOutputTileBuffer->InitByValue(0);

	MercatorTileMatrixSet oWorldMercGrid(WEB_MERCATOR);
	OGREnvelope oWorldMercEnvpUpper = oWorldMercGrid.CalcEnvelopeByTile(nZ, nX, nWorldMercUpperTileY);
	int nUpperBlockOffTop = int(((oWorldMercEnvpUpper.MaxY - oWorldMercEnvp.MaxY) / 
		oWorldMercGrid.CalcPixelSizeByZoom(nZ)) + 0.5);

	poOutputTileBuffer->SetPixelDataBlock(0, 0, 256, 256 - nUpperBlockOffTop,
		oInputupperTileBuffer.GetPixelDataBlock(0, nUpperBlockOffTop, 256, 256 - nUpperBlockOffTop));
	poOutputTileBuffer->SetPixelDataBlock(0, 256 - nUpperBlockOffTop, 256, nUpperBlockOffTop,
		oInputLowerTileBuffer.GetPixelDataBlock(0, 0, 256, nUpperBlockOffTop));
	return poOutputTileBuffer;
}
//*/


MBTileContainer::MBTileContainer ()
{
  read_only_=true;
	p_sql3_db_ = NULL;
};

MBTileContainer::~MBTileContainer ()
{
	Close();
}

/*
MBTileContainer (string file_name)
{
	if (! OpenForReading(file_name))
	{
		cout<<"Error: can't open for reading: "<<file_name<<endl;
	}
}
*/

int		MBTileContainer::GetMinZoom()
{
	if (p_sql3_db_ == NULL) return -1;
	string str_sql = "SELECT MIN(zoom_level) FROM tiles";
	sqlite3_stmt *stmt	= NULL;
	if (SQLITE_OK != sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1, &stmt, NULL)) return -1;
	
	int min_zoom = -1;
	if (SQLITE_ROW == sqlite3_step (stmt))
		min_zoom = sqlite3_column_int(stmt,0);
	sqlite3_finalize(stmt);
	return min_zoom;
};
	
int		MBTileContainer::GetMaxZoom()
{
	if (p_sql3_db_ == NULL) return -1;
	string str_sql = "SELECT MAX(zoom_level) FROM tiles";
	sqlite3_stmt *stmt	= NULL;
	if (SQLITE_OK != sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1, &stmt, NULL)) return -1;
	
	int max_zoom = -1;
	if (SQLITE_ROW == sqlite3_step (stmt))
		max_zoom = sqlite3_column_int(stmt,0);
	sqlite3_finalize(stmt);
	return max_zoom;
};
	
bool MBTileContainer::OpenForReading  (string file_name)
{
  if (SQLITE_OK != sqlite3_open(file_name.c_str(),&p_sql3_db_))
	{
		cout<<"ERROR: can't open mbtiles file: "<<file_name<<endl;
		return FALSE;
	}

	string str_sql = "SELECT * FROM metadata WHERE name = 'format'";
	sqlite3_stmt *stmt	= NULL;
	sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1, &stmt, NULL);
	
	if (SQLITE_ROW == sqlite3_step (stmt))
	{
    string format(reinterpret_cast<const char *>(sqlite3_column_text(stmt,1)), GMXString::StrLen(sqlite3_column_text(stmt,1)));
		tile_type_ = (format == "jpg") ? JPEG_TILE : PNG_TILE;
		sqlite3_finalize(stmt);
	}
	else
	{
		tile_type_ = PNG_TILE;
		sqlite3_finalize(stmt);
	}

	str_sql = "SELECT * FROM metadata WHERE name = 'projection'";
	sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1, &stmt, NULL);
	if (SQLITE_ROW == sqlite3_step (stmt))
	{

		string proj(reinterpret_cast<const char *>(sqlite3_column_text(stmt,1)), GMXString::StrLen(sqlite3_column_text(stmt,1)));
		merc_type_	=	(proj	== "0" || proj	== "WORLD_MERCATOR" || proj	== "EPSG:3395") ? WORLD_MERCATOR : WEB_MERCATOR;
		sqlite3_finalize(stmt);
	}
	else
	{
		merc_type_	=	WEB_MERCATOR;
		sqlite3_finalize(stmt);
	}

	return TRUE;
}

MBTileContainer* MBTileContainer::OpenForWriting (TileContainerOptions *p_params)
{
  if (!p_params) return 0;
  if (p_params->path_=="") return 0;
  if (!p_params->p_matrix_set_) return 0;
  if (p_params->tiling_srs_envp_.MaxX<p_params->tiling_srs_envp_.MinX) return 0;
  if (p_params->tile_type_==TileType::NDEF_TILE_TYPE) return 0;
  
  return new MBTileContainer(p_params->path_,
    p_params->tile_type_,
    ((MercatorTileMatrixSet*)p_params->p_matrix_set_)->merc_type(),
    p_params->tiling_srs_envp_);
}


MBTileContainer::MBTileContainer (string file_name, TileType tile_type,MercatorProjType merc_type, OGREnvelope merc_envp)
{
	p_sql3_db_ = NULL;
	if (GMXFileSys::FileExists(file_name)) GMXFileSys::FileDelete(file_name.c_str());
	if (SQLITE_OK != sqlite3_open(file_name.c_str(),&p_sql3_db_)) return;

	char	*p_err_msg = NULL;
  sqlite3_exec(p_sql3_db_,
               "CREATE TABLE metadata (name text, value text)",
               NULL, 
               0,
               &p_err_msg);
	if (p_err_msg!=NULL)
	{
		cout<<"ERROR: sqlite: "<<p_err_msg<<endl;
		delete[]p_err_msg;
		Close();
		return;
	}

  string	str_sql;
	str_sql = "INSERT INTO metadata VALUES ('format',";
	str_sql += (tile_type == JPEG_TILE) ? "'jpg')" :  (tile_type == PNG_TILE) ? "'png')" : "'tif')";
	sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg);

	str_sql = "INSERT INTO metadata VALUES ('projection',";
	str_sql += (merc_type == WORLD_MERCATOR) ? "'WORLD_MERCATOR')" :  "'WEB_MERCATOR')";
	sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg);
		
	OGREnvelope latlong_envp;

	latlong_envp.MinX = MercatorTileMatrixSet::MecrToLong(merc_envp.MinX, merc_type);
	latlong_envp.MaxX = MercatorTileMatrixSet::MecrToLong(merc_envp.MaxX, merc_type);
	latlong_envp.MinY = MercatorTileMatrixSet::MercToLat(merc_envp.MinY, merc_type);
	latlong_envp.MaxY = MercatorTileMatrixSet::MercToLat(merc_envp.MaxY, merc_type);

  char	buf[256];
	sprintf(buf,"INSERT INTO metadata VALUES ('bounds', '%lf,%lf,%lf,%lf')",
									latlong_envp.MinX,
									latlong_envp.MinY,
									latlong_envp.MaxX,
									latlong_envp.MaxY);
	sqlite3_exec(p_sql3_db_, buf, NULL, 0, &p_err_msg);

				

  sqlite3_exec(p_sql3_db_, "CREATE TABLE tiles ("
  "zoom_level INTEGER NOT NULL,"
  "tile_column INTEGER NOT NULL,"
  "tile_row INTEGER NOT NULL,"
  "tile_data BLOB NOT NULL,"
  "UNIQUE (zoom_level, tile_column, tile_row) )", 0, 0, &p_err_msg);
	if (p_err_msg!=NULL)
	{
		cout<<"ERROR: sqlite: "<<p_err_msg<<endl;
		delete[]p_err_msg;
		Close();
		return;
	}		
  read_only_=false;
}



bool	MBTileContainer::AddTile(int z, int x, int y, unsigned char *p_data, unsigned int size)
{
	if ((!p_sql3_db_) || read_only_) return FALSE;

	char buf[256];
	string str_sql;

	///*
	if (TileExists(z,x,y))
	{
		sprintf(buf,"DELETE * FROM tiles WHERE zoom_level = %d AND tile_column = %d AND tile_row = %d",z,x,y);
		str_sql = buf;
		char	*p_err_msg = NULL;
		if (SQLITE_DONE != sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg))
		{
			cout<<"ERROR: can't delete existing tile: "<<str_sql<<endl;	
			return FALSE;
		}
	}
	//*/
  if (z>0) y = (1 << z) - y - 1;
  sprintf(buf,"INSERT INTO tiles VALUES(%d,%d,%d,?)",z,x,y);
	str_sql = buf;
	const char *tail	=	NULL;
	sqlite3_stmt *stmt	=	NULL;
	int k = sqlite3_prepare_v2(p_sql3_db_,str_sql.c_str(), str_sql.size()+1,&stmt, &tail);
	sqlite3_bind_blob(stmt,1,p_data,size,SQLITE_TRANSIENT);
	k = sqlite3_step(stmt);
	if (SQLITE_DONE != k) 
	{
		cout<<"Error sqlite-message: "<<sqlite3_errmsg(p_sql3_db_)<<endl;
		sqlite3_finalize(stmt);
		return FALSE;
	}
	sqlite3_finalize(stmt);

	return TRUE;
}

bool		MBTileContainer::TileExists(int z, int x, int y)
{
	if (p_sql3_db_ == NULL) return FALSE;
		
	char buf[256];
	if (z>0) y = (1<<z) - y - 1;
	sprintf(buf,"SELECT * FROM tiles WHERE zoom_level = %d AND tile_column = %d AND tile_row = %d",z,x,y);
	string str_sql(buf);
		
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1,&stmt, &tail);
	
	bool result = (SQLITE_ROW == sqlite3_step (stmt));

		

	sqlite3_finalize(stmt);
	return result;	
}

bool		MBTileContainer::GetTile(int z, int x, int y, unsigned char *&p_data, unsigned int &size)
{
	if (p_sql3_db_ == NULL) return FALSE;
	char buf[256];
	string str_sql;

	if (z>0) y = (1<<z) - y - 1;
	sprintf(buf,"SELECT * FROM tiles WHERE zoom_level = %d AND tile_column = %d AND tile_row = %d",z,x,y);
	str_sql = buf;
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1,&stmt, &tail);
	
	if (SQLITE_ROW != sqlite3_step (stmt))
	{
		sqlite3_finalize(stmt);
		return FALSE;	
	}

	size		= sqlite3_column_bytes(stmt, 3);
	p_data = new unsigned char[size];
	memcpy(p_data, sqlite3_column_blob(stmt, 3),size);
	sqlite3_finalize(stmt);

		
	return TRUE;
}

int 		MBTileContainer::GetTileList(list<pair<int,pair<int,int>>> &tile_list, int min_zoom, int max_zoom, string vector_file)
{
	if (p_sql3_db_ == NULL) return 0;
	OGRGeometry *p_border = NULL;
  MercatorTileMatrixSet merc_grid(merc_type_);
  if (vector_file!="") 
	{
    if( !(p_border = VectorOperations::ReadAndTransformGeometry(vector_file,merc_grid.GetTilingSRSRef()))) 
		{
			cout<<"ERROR: can't open vector file: "<<vector_file<<endl;
			return 0;
		}
	  if (!merc_grid.AdjustIfOverlap180Degree(p_border))
    {
      cout<<"ERROR: AdjustIfOverlapAbscissa fail"<<endl;
      delete(p_border);
      return 0;
    }
  }
  
  char buf[256];
	string str_sql;

	max_zoom = (max_zoom < 0) ? 32 : max_zoom;
	sprintf(buf,"SELECT zoom_level,tile_column,tile_row FROM tiles WHERE zoom_level >= %d AND zoom_level <= %d",min_zoom,max_zoom);
	str_sql = buf;
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1,&stmt, &tail);
	pair<int,pair<int,int>> p;
	while (SQLITE_ROW == sqlite3_step (stmt))
	{
		int z		= sqlite3_column_int(stmt, 0);
		int x		= sqlite3_column_int(stmt, 1);
		int y		= (z>0) ? ((1<<z) - sqlite3_column_int(stmt, 2) -1) : 0;
		if (x<0 || y<0 || z <0) continue;
    if (vector_file!="")
    {
      OGRPolygon *p_envp = VectorOperations::CreateOGRPolygonByOGREnvelope(merc_grid.CalcEnvelopeByTile(z,x,y));
      if (!p_border->Intersects(p_envp))
      {
        delete(p_envp);
        continue;
      }
      delete(p_envp);
    }

		p.first = z;
    p.second.first=x;
    p.second.second =y;
		tile_list.push_back(p);
	}
	sqlite3_finalize(stmt);
  delete(p_border);

		
	return tile_list.size();
};
	

bool		MBTileContainer::GetTileBounds (int tile_bounds[128])
{
	for (int z=0; z<32; z++)
		tile_bounds[4*z]		= 	(tile_bounds[4*z+1] = (tile_bounds[4*z + 2]	= 	(tile_bounds[4*z+3] =	-1)));

	if (p_sql3_db_ == NULL) return FALSE;
	char buf[256];
	string str_sql;

	sprintf(buf,"SELECT zoom_level,tile_column,tile_row FROM tiles");
	str_sql = buf;
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1,&stmt, &tail);
	
	while (SQLITE_ROW == sqlite3_step (stmt))
	{
		int z		= sqlite3_column_int(stmt, 0);
		int x		= sqlite3_column_int(stmt, 1);
		int y		= (z>0) ? ((1<<z) - sqlite3_column_int(stmt, 2) -1) : 0;

		tile_bounds[4*z]		= (tile_bounds[4*z] == -1   || tile_bounds[4*z] > x) ? x : tile_bounds[4*z];
		tile_bounds[4*z+1]	= (tile_bounds[4*z+1] == -1 || tile_bounds[4*z+1] > y) ? y : tile_bounds[4*z+1];
		tile_bounds[4*z+2]	= (tile_bounds[4*z+2] == -1 || tile_bounds[4*z+2] < x) ? x : tile_bounds[4*z+2];
		tile_bounds[4*z+3]	= (tile_bounds[4*z+3] == -1 || tile_bounds[4*z+3] < y) ? y : tile_bounds[4*z+3];
	}

	sqlite3_finalize(stmt);
		
	return TRUE;
};

TileType	MBTileContainer::GetTileType()
{
	return tile_type_;
};

MercatorProjType	MBTileContainer::GetProjType()
{
	return merc_type_;
};

bool MBTileContainer::Close ()
{
  if (!p_sql3_db_) return false;
  else if (! read_only_)
  {
    char	*p_err_msg = NULL;
    string str_sql = "INSERT INTO metadata VALUES ('maxzoom','" + GMXString::ConvertIntToString(GetMaxZoom())+"')";
    sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg);
    str_sql = "INSERT INTO metadata VALUES ('minzoom','" + GMXString::ConvertIntToString(GetMinZoom())+"')";
    sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg);
  }
  
  sqlite3_close(p_sql3_db_);
  p_sql3_db_ = NULL;
	return TRUE;
};


TileFolder* TileFolder::OpenForWriting (TileContainerOptions *p_params)
{
  if (!p_params) return 0;
  if (!p_params->p_tile_name_) return 0;
  bool b_use_cache = p_params->extra_options_["cache_size"]=="" ? true :
                    atoi(p_params->extra_options_["cache_size"].c_str()) == 0 ? false : true;
  return new TileFolder(p_params->p_tile_name_,((MercatorTileMatrixSet*)p_params->p_matrix_set_)->merc_type(),b_use_cache);
};



TileFolder::TileFolder (TileName *p_tile_name, MercatorProjType merc_type, bool use_cache)
{
  merc_type_=merc_type;
	p_tile_name_	= p_tile_name;
	use_cache_	= use_cache;
	p_tile_cache_		= (use_cache) ? new TileCache() :  NULL;
};



TileFolder::~TileFolder ()
{
	Close();
};

bool TileFolder::Close()
{
	delete(p_tile_cache_);
	p_tile_cache_ = NULL;
	return TRUE;
};

bool	TileFolder::OpenForReading (string folder_name)
{
  return GMXFileSys::FileExists(folder_name);
};

TileType	TileFolder::GetTileType()
{
  if (p_tile_name_)
    return p_tile_name_->tile_type_;
  else return NDEF_TILE_TYPE;
};

MercatorProjType		TileFolder::GetProjType()
{
  return NDEF_PROJ_TYPE;
};



bool	TileFolder::AddTile(int z, int x, int y, unsigned char *p_data, unsigned int size)
{
  addtile_mutex_.lock();
	if (use_cache_)	p_tile_cache_->AddTile(z,x,y,p_data,size);
	bool result = writeTileToFile(z,x,y,p_data,size);
  addtile_mutex_.unlock();
  return result;
};



bool		TileFolder::GetTile(int z, int x, int y, unsigned char *&p_data, unsigned int &size)
{
	if (use_cache_)	
  {
    if (p_tile_cache_->FindTile(z,x,y))
      return p_tile_cache_->GetTile(z,x,y,p_data,size); 
  }
 
  return ReadTileFromFile(z,x,y,p_data,size);
};

bool		TileFolder::TileExists(int z, int x, int y)
{
	return GMXFileSys::FileExists(p_tile_name_->GetFullTileName(z,x,y));
};


int			TileFolder::GetMaxZoom()
{
	int maxZ = -1;
	int tile_bounds[128];
	if (!GetTileBounds(tile_bounds)) return -1;
	for (int z=0;z<32;z++)
		if (tile_bounds[4*z+2]>0) maxZ = z;
	return maxZ;
};


int 		TileFolder::GetTileList(list<pair<int,pair<int,int>>> &tile_list, int min_zoom, int max_zoom, string vector_file)
{
  MercatorTileMatrixSet merc_grid(merc_type_);
  OGRGeometry *p_border = NULL;
	if (vector_file!="") 
	{
    if( !(p_border = VectorOperations::ReadAndTransformGeometry(vector_file,merc_grid.GetTilingSRSRef()))) 
		{
			cout<<"ERROR: can't open vector file: "<<vector_file<<endl;
			return 0;
		}
	  if (!merc_grid.AdjustIfOverlap180Degree(p_border))
    {
      cout<<"ERROR: AdjustIfOverlapAbscissa fail"<<endl;
      delete(p_border);
      return 0;
    }
  }
  
	list<string> file_list;
	GMXFileSys::FindFilesByExtensionRecursive(file_list,p_tile_name_->GetBaseFolder(),TileName::ExtensionByTileType(p_tile_name_->tile_type_));
  
  pair<int,pair<int,int>> p;
	for (list<string>::iterator iter = file_list.begin(); iter!=file_list.end();iter++)
	{
		int x,y,z;
		if(!p_tile_name_->ExtractXYZ((*iter),z,x,y)) continue;
		if ((max_zoom>=0)&&(z>max_zoom)) continue;
		if ((min_zoom>=0)&&(z<min_zoom)) continue;
    if (vector_file!="")
    {
      OGRPolygon *p_envp = VectorOperations::CreateOGRPolygonByOGREnvelope(merc_grid.CalcEnvelopeByTile(z,x,y));
      if (!p_border->Intersects(p_envp))
      {
        delete(p_envp);
        continue;
      }
      delete(p_envp);
    }
    p.first = z;
    p.second.first=x;
    p.second.second=y;
		tile_list.push_back(p);
	}
	delete(p_border);
	return tile_list.size();	
};


bool		TileFolder::GetTileBounds (int tile_bounds[128])
{
	list<pair<int,pair<int,int>>> tile_list;
	if (!GetTileList(tile_list,-1,-1,"")) return FALSE;
	for (int z=0; z<32;z++)
		tile_bounds[4*z]		= 	(tile_bounds[4*z+1] = (tile_bounds[4*z + 2]	= 	(tile_bounds[4*z+3] =	-1)));

	for (list<pair<int,pair<int,int>>>::const_iterator iter = tile_list.begin(); iter!=tile_list.end();iter++)
	{
		int z = (*iter).first;
    int x = (*iter).second.first;
    int y = (*iter).second.second;
    tile_bounds[4*z]		= (tile_bounds[4*z] == -1   || tile_bounds[4*z] > x) ? x : tile_bounds[4*z];
		tile_bounds[4*z+1]	= (tile_bounds[4*z+1] == -1 || tile_bounds[4*z+1] > y) ? y : tile_bounds[4*z+1];
		tile_bounds[4*z+2]	= (tile_bounds[4*z+2] == -1 || tile_bounds[4*z+2] < x) ? x : tile_bounds[4*z+2];
		tile_bounds[4*z+3]	= (tile_bounds[4*z+3] == -1 || tile_bounds[4*z+3] < y) ? y : tile_bounds[4*z+3];
  }

	return TRUE;
};

	
bool	TileFolder::writeTileToFile (int z, int x, int y, unsigned char *p_data, unsigned int size)
{
	if (p_tile_name_==NULL) return FALSE;
	p_tile_name_->CreateFolder(z,x,y);
	return GMXFileSys::SaveDataToFile(p_tile_name_->GetFullTileName(z,x,y), p_data,size);
};

	
bool	TileFolder::ReadTileFromFile (int z,int x, int y, unsigned char *&p_data, unsigned int &size)
{
  void *_p_data;
  int _size;
  bool result = GMXFileSys::ReadBinaryFile (p_tile_name_->GetFullTileName(z,x,y),_p_data,_size);
  size = _size;
  p_data = (unsigned char*)_p_data;
	return result;
 };

bool TileContainerFactory::GetTileContainerType (string strName, TileContainerType &eType)
{
  strName = GMXString::MakeLower(strName);
  if (strName=="")
    eType = TileContainerType::TILEFOLDER;
  else if (strName=="gmxtiles")
    eType = TileContainerType::GMXTILES;
  else if (strName=="mbtiles")
    eType = gmx::TileContainerType::MBTILES;
  else return false;

  return true;
};


ITileContainer* TileContainerFactory::OpenForWriting(TileContainerType container_type, TileContainerOptions *p_params)
{
  switch (container_type)
  {
    case TileContainerType::GMXTILES:
      return GMXTileContainer::OpenForWriting(p_params);
    case TileContainerType::MBTILES:
      return MBTileContainer::OpenForWriting(p_params);
    case TileContainerType::TILEFOLDER:
      return TileFolder::OpenForWriting(p_params);
    return 0;
  }
};

string TileContainerFactory::GetExtensionByTileContainerType (TileContainerType container_type)
{
  switch (container_type)
  {
    case TileContainerType::GMXTILES:
      return "tiles";
    case TileContainerType::MBTILES:
      return "mbtiles";
    case TileContainerType::TILEFOLDER:
      return "";
    return "";
  }
};

ITileContainer* TileContainerFactory::OpenForReading (string file_name)
{
	ITileContainer *p_tc = NULL;
  if (!GMXFileSys::FileExists(file_name)) return 0;
  if (GMXFileSys::IsDirectory(file_name)) return 0;

  if (GMXString::MakeLower(GMXFileSys::GetExtension(file_name)) == "mbtiles")
  {
    MBTileContainer* p_mbtiles = new MBTileContainer();
    if (!p_mbtiles->OpenForReading(file_name)) 
    {
      delete p_mbtiles;
      return 0;
    }
    else return p_mbtiles;
  }
  else if ((GMXString::MakeLower(GMXFileSys::GetExtension(file_name)) == "tiles")||
           (GMXString::MakeLower(GMXFileSys::GetExtension(file_name)) == "gmxtiles")) 
	{
    GMXTileContainer* p_gmx_tc = new GMXTileContainer();
    if (!p_gmx_tc->OpenForReading(file_name))
    {
		  delete(p_tc);
		  return 0;
    }
    else return p_gmx_tc;
	}
};


}