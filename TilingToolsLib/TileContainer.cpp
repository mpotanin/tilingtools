#include "StdAfx.h"
#include "TileContainer.h"
using namespace gmx;

namespace gmx
{

///*
ITileContainer* OpenTileContainerForReading (string file_name)
{
	ITileContainer *p_tc = NULL;
	if (MakeLower(file_name).find(".mbtiles") != string::npos) p_tc = new MBTileContainer();
	else p_tc = new GMXTileContainer();

	if (p_tc->OpenForReading(file_name)) return p_tc;
	else
	{
		p_tc->Close();
		delete(p_tc);
		return NULL;
	}
};
//*/

GMXTileContainer::GMXTileContainer (	string				container_file_name, 
										TileType			tile_type,
										MercatorProjType	merc_type,
										int					tile_bounds[128],
										BOOL				use_cache
									)
{
	Init(tile_bounds,use_cache,container_file_name,tile_type,merc_type);
};


GMXTileContainer::GMXTileContainer	(	string				container_file_name, 
										TileType			tile_type,
										MercatorProjType	merc_type,
										OGREnvelope			envelope, 
										int					max_zoom, 
										BOOL				use_cache)
{
	int tile_bounds[128];
	for (int i=0;i<32;i++)
	{
		tile_bounds[4*i] = (tile_bounds[4*i+1] = (tile_bounds[4*i+2] = (tile_bounds[4*i+3] = -1)));
		if (i<=max_zoom)
		{
			MercatorTileGrid::CalcTileRange(envelope,i,tile_bounds[4*i],tile_bounds[4*i+1],tile_bounds[4*i+2],tile_bounds[4*i+3]);
		}
	}
	Init(tile_bounds,use_cache,container_file_name,tile_type,merc_type);
};


GMXTileContainer::GMXTileContainer	() 
{
	max_tiles_		= 0;
	p_sizes_			= NULL; 
	p_offsets_		= NULL;
	p_tile_cache_	= NULL;
	use_cache_	= FALSE;

};	

	/*
	GMXTileContainer (string file_name)
	{
		if (!OpenForReading(file_name))
		{
			cout<<"ERROR: can't open for reading: "<<file_name<<endl;
		}
	}
	*/

BOOL GMXTileContainer::OpenForReading (string container_file_name)
{
	
	read_only_		= TRUE;
	max_tiles_		= 0;
	p_sizes_			= NULL; 
	p_offsets_		= NULL;
	p_tile_cache_	= NULL;
	use_cache_	= FALSE;
	
	if (!(p_container_file_ = OpenFile(container_file_name,"rb"))) return FALSE;
	BYTE	head[12];
	fread(head,1,12,p_container_file_);
	if (!((head[0]=='G')&&(head[1]=='M')&&(head[2]=='T')&&(head[3]=='C'))) 
	{
		cout<<"ERROR: incorrect input tile container file: "<<container_file_name<<endl;
		return FALSE;
	}
		


	MAX_TILES_IN_CONTAINER = 0;
	merc_type_ = (head[9] == 0) ? WORLD_MERCATOR : WEB_MERCATOR;
	tile_type_ = (head[11] == 0) ? JPEG_TILE : (head[11]==1) ? PNG_TILE : TIFF_TILE;
	use_cache_ = FALSE;
	p_tile_cache_ = NULL;

	BYTE bounds[512];
	fread(bounds,1,512,p_container_file_);

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

	BYTE*			offset_size = new BYTE[max_tiles_*13];
	fread(offset_size,1,max_tiles_*13,p_container_file_);


	p_sizes_		= new unsigned int[max_tiles_];
	p_offsets_		= new unsigned __int64[max_tiles_];

	for (int i=0; i<max_tiles_;i++)
	{
		p_offsets_[i]	= *((unsigned __int64*)(&offset_size[i*13]));
		if ((p_offsets_[i]<<32) == 0)
			p_offsets_[i]	= (p_offsets_[i]>>32);
		p_sizes_[i]	= *((unsigned int*)(&offset_size[i*13+8]));
	}

	delete[]offset_size;
	return TRUE;
};
	

GMXTileContainer::~GMXTileContainer()
{
	MakeEmpty();
}
	
//fclose(containerFileData);


BOOL		GMXTileContainer::AddTile(int z, int x, int y, BYTE *p_data, unsigned int size)
{
	__int64 n	= TileID(z,x,y);
  if ((n>= max_tiles_)||(n<0)) return FALSE;

	if (p_tile_cache_)
	{
    if (p_tile_cache_->AddTile(z,x,y,p_data,size))
    {
      p_sizes_[n]		= size;
      return TRUE;
    }
	}
	
  return AddTileToContainerFile(z,x,y,p_data,size);
};

BOOL		GMXTileContainer::GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size)
{
	if (p_tile_cache_)	
  {
    if (p_tile_cache_->GetTile(z,x,y,p_data,size)) return TRUE;
  }
	
  return GetTileFromContainerFile(z,x,y,p_data,size);
};

BOOL		GMXTileContainer::TileExists(int z, int x, int y)
{
	unsigned int n = TileID(z,x,y);
	if (n>= max_tiles_ && n<0) return FALSE;
	return (p_sizes_[n]>0) ? TRUE : FALSE;
}; 

BOOL		GMXTileContainer::Close()
{
	if (read_only_)
	{
		MakeEmpty();
		return TRUE;
	}
	
	if (p_tile_cache_) WriteTilesToContainerFileFromCache();
	
	string	file_temp_name	= container_file_name_ + ".temp";
	FILE	*p_temp_file = OpenFile(file_temp_name,"wb");
	if (!p_temp_file) 	return FALSE;
	BYTE	*header;
	this->WriteHeaderToByteArray(header);
	fwrite(header,1,HeaderSize(),p_temp_file);
	delete[]header;
	fseek(p_container_file_,HeaderSize(),0);
		
	int blockLen = 10000000;
	BYTE	*block = new BYTE[blockLen];
	for (unsigned __int64 i = HeaderSize(); i<container_byte_size_; i+=blockLen)
	{
		int blockLen_ = (i+blockLen > container_byte_size_) ? container_byte_size_ - i : blockLen;
    fread(block,1,blockLen_,p_container_file_);
		fwrite(block,1,blockLen_,p_temp_file);
	}
	delete[]block;

	fclose(p_container_file_);
	p_container_file_ = NULL;

	fclose(p_temp_file);
	DeleteFile(container_file_name_.c_str());
	RenameFile(file_temp_name.c_str(),container_file_name_.c_str());
	
	MakeEmpty();
	return TRUE;		
};


BOOL		GMXTileContainer::GetTileBounds (int tile_bounds[128])
{
	if (max_tiles_<=0) return FALSE;
	if (maxx_==0 || maxy_==0 || minx_==0 || miny_==0) return FALSE;
	for (int z=0;z<32;z++)
	{
		if ((maxx_[4*z+2]==0)||(maxx_[4*z+3]==0)) tile_bounds[4*z] = (tile_bounds[4*z+1] = (tile_bounds[4*z+2] = (tile_bounds[4*z+3] =-1)));
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
											string vector_file, 
											MercatorProjType merc_type
											)
{
	VectorBorder *p_vb = NULL;

	if (vector_file!="") 
	{
		if(!(p_vb = VectorBorder::CreateFromVectorFile(vector_file,merc_type_))) 
		{
			cout<<"ERROR: can't open vector file: "<<vector_file<<endl;
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
				if (!p_vb->Intersects(z,x,y)) continue;
      p.first =z;
      p.second.first=x;
      p.second.second=y;
      tile_list.push_back(p);
		}
	}

	delete(p_vb);
	return tile_list.size();	
};
	
__int64		GMXTileContainer::TileID( int z, int x, int y)
{
	unsigned int num = 0;
	for (int s=0;s<z;s++)
		num+=(maxx_[s]-minx_[s])*(maxy_[s]-miny_[s]);
	return (num + (maxx_[z]-minx_[z])*(y-miny_[z]) + x-minx_[z]);
};

BOOL		GMXTileContainer::TileXYZ(__int64 n, int &z, int &x, int &y)
{
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

OGREnvelope GMXTileContainer::GetMercatorEnvelope()
{
	OGREnvelope envelope;
	envelope.MinX = (envelope.MaxX = (envelope.MinY = (envelope.MaxY = 0)));

	for (int z =0;z<32;z++)
	{
		if (maxx_[z]>0 && maxy_[z]>0)
			envelope = MercatorTileGrid::CalcEnvelopeByTileRange(z,minx_[z],miny_[z],maxx_[z],maxy_[z]);
		else break;
	}
	return envelope;
};

int			GMXTileContainer::GetMaxZoom()
{
	int max_z = -1;
	for (int z=0;z<32;z++)
		if (maxx_[z]>0) max_z = z;
	return max_z;
};

BOOL 	GMXTileContainer::Init	(	int					tile_bounds[128], 
									BOOL				use_cache, 
									string				container_file_name, 
									TileType			tile_type,
									MercatorProjType	merc_type)
{
	read_only_					= FALSE;
	tile_type_					= tile_type;
	merc_type_					= merc_type;
	container_file_name_			= container_file_name;// + ".tiles";
	p_container_file_			= NULL;
	container_byte_size_			= 0;
	MAX_TILES_IN_CONTAINER	= 0;

	if (use_cache) 
	{
		use_cache_	= TRUE;
		p_tile_cache_	= new TileCache();
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
	p_offsets_		= new unsigned __int64[max_tiles_];
	for (unsigned int i=0;i<max_tiles_;i++)
	{
		p_sizes_[i]		= 0;
		p_offsets_[i]		= 0;
	}
	return TRUE;
};


BOOL	GMXTileContainer::WriteContainerFromBuffer()
{
	if (use_cache_) 
	{
		WriteTilesToContainerFileFromCache();
		WriteContainerFileHeader();
	}
	return TRUE;
}


BOOL	GMXTileContainer::AddTileToContainerFile(int z, int x, int y, BYTE *p_data, unsigned int size)
{
	//ToDo - fix if n<0
	unsigned int n	= TileID(z,x,y);
	if (n>= max_tiles_) return FALSE;

	if (!p_container_file_)
	{
		if (!(p_container_file_ = OpenFile(container_file_name_,"wb+")))
		{
			cout<<"Can't add tile to file: "<<container_file_name_<<endl;
			return FALSE;
		}
		BYTE	*header;
		this->WriteHeaderToByteArray(header);
		fwrite(header,1,HeaderSize(),p_container_file_);
		delete[]header;
	}


	_fseeki64(p_container_file_,0,SEEK_END);
	fwrite(p_data,sizeof(BYTE),size,p_container_file_);
	p_sizes_[n]		= size;
	if (container_byte_size_ == 0) container_byte_size_ = HeaderSize();
	p_offsets_[n] = container_byte_size_;
	container_byte_size_ +=size;
	
	return TRUE;
};
	
BOOL	GMXTileContainer::GetTileFromContainerFile (int z, int x, int y, BYTE *&p_data, unsigned int &size)
{
	unsigned int n	= TileID(z,x,y);
	if (n>= max_tiles_ && n<0) return FALSE;
	if (!(size = p_sizes_[n])) return TRUE;
	
	if (!p_container_file_)
	{
		if (!(p_container_file_ =  OpenFile(container_file_name_,"rb")))
		{
			cout<<"Can't read tile from file: "<<container_file_name_<<endl;
			return FALSE;
		}
	}

	_fseeki64(p_container_file_,p_offsets_[n],0);
		
	p_data			= new BYTE[p_sizes_[n]];
	//Huge file _fseeki64 _ftelli64 in Visual C++
	fread(p_data,1,size,p_container_file_);
};

BOOL	GMXTileContainer::WriteTilesToContainerFileFromCache()
{
	if (!p_container_file_)
	{
		if (!(p_container_file_ = OpenFile(container_file_name_,"wb+")))
		{
			cout<<"Can't create file: "<<container_file_name_<<endl;
			return FALSE;
		}
    fseek(p_container_file_,HeaderSize(),0);
    container_byte_size_=HeaderSize();
	}
	
  int k=0;
 	BYTE	*tile_data = NULL;
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
    
    BYTE *p_block = new BYTE[block_size];
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

    fwrite(p_block,sizeof(BYTE),block_size,p_container_file_);
    container_byte_size_+=block_size;
		delete[]p_block;
    k=n;
  }
	
  return TRUE;
};

BOOL	GMXTileContainer::WriteContainerFileHeader()
{
	if (!p_container_file_) return FALSE;
	fseek(p_container_file_,0,SEEK_SET);
	
	BYTE	*p_container_head_data;
	WriteHeaderToByteArray(p_container_head_data);
	fwrite(p_container_head_data,1,HeaderSize(),p_container_file_);
	fclose(p_container_file_);
	p_container_file_ = NULL;
	delete[]p_container_head_data;
	return TRUE;
};


BOOL	GMXTileContainer::WriteHeaderToByteArray(BYTE*	&p_data)
{
	p_data = new BYTE[HeaderSize()];
	string file_type = "GMTC";
	memcpy(p_data,file_type.c_str(),4);
	memcpy(&p_data[4],&MAX_TILES_IN_CONTAINER,4);
	p_data[8]	= 0;
	p_data[9]	= merc_type_;
	p_data[10]	= 0;
	p_data[11]	= tile_type_;

	for (int z=0;z<32;z++)
	{
		memcpy(&p_data[12+z*16],&minx_[z],4);
		memcpy(&p_data[12+z*16+4],&miny_[z],4);
		memcpy(&p_data[12+z*16+8],&maxx_[z],4);
		memcpy(&p_data[12+z*16+12],&maxy_[z],4);
	}
	int t[36] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	memcpy(&p_data[12+32*16],t,144);

	BYTE	tile_info[13];
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

	return TRUE;
};

	
unsigned int GMXTileContainer::HeaderSize()
{
	return (4+4+4+512+max_tiles_*(4+8+1));
};

void GMXTileContainer::MakeEmpty ()
{
	delete[]p_sizes_;
	p_sizes_ = NULL;
	delete[]p_offsets_;
	p_offsets_ = NULL;
	max_tiles_ = 0;
		
	for (int i=0;i<32;i++)
		maxx_[i]=(minx_[i]=(maxy_[i]=(miny_[i]=0)));
	delete(p_tile_cache_);
	p_tile_cache_ = NULL;
};
	





MBTileContainer::MBTileContainer ()
{
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
		cout<<"ERROR: can't open for reading: "<<file_name<<endl;
	}
}
*/
	
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
	
BOOL MBTileContainer::OpenForReading  (string file_name)
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
		string format(reinterpret_cast<const char *>(sqlite3_column_text(stmt,1)), StrLen(sqlite3_column_text(stmt,1)));
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

		string proj(reinterpret_cast<const char *>(sqlite3_column_text(stmt,1)), StrLen(sqlite3_column_text(stmt,1)));
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


MBTileContainer::MBTileContainer (string file_name, TileType tile_type,MercatorProjType merc_type, OGREnvelope merc_envp)
{
	p_sql3_db_ = NULL;
	if (FileExists(file_name)) DeleteFile(file_name.c_str());
	if (SQLITE_OK != sqlite3_open(file_name.c_str(),&p_sql3_db_)) return;

	string	str_sql = "CREATE TABLE metadata (name text, value text)";
	char	*p_err_msg = NULL;
	sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg);
	if (p_err_msg!=NULL)
	{
		cout<<"ERROR: sqlite: "<<p_err_msg<<endl;
		delete[]p_err_msg;
		Close();
		return;
	}

	str_sql = "INSERT INTO metadata VALUES ('format',";
	str_sql += (tile_type == JPEG_TILE) ? "'jpg')" :  (tile_type == PNG_TILE) ? "'png')" : "'tif')";
	sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg);

	str_sql = "INSERT INTO metadata VALUES ('projection',";
	str_sql += (merc_type == WORLD_MERCATOR) ? "'WORLD_MERCATOR')" :  "'WEB_MERCATOR')";
	sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg);
		
	OGREnvelope latlong_envp;
	latlong_envp.MinX = MercatorTileGrid::MecrToLong(merc_envp.MinX, merc_type);
	latlong_envp.MaxX = MercatorTileGrid::MecrToLong(merc_envp.MaxX, merc_type);
	latlong_envp.MinY = MercatorTileGrid::MercToLat(merc_envp.MinY, merc_type);
	latlong_envp.MaxY = MercatorTileGrid::MercToLat(merc_envp.MaxY, merc_type);
	char	buf[256];
	sprintf(buf,"INSERT INTO metadata VALUES ('bounds', '%lf,%lf,%lf,%lf')",
									latlong_envp.MinX,
									latlong_envp.MinY,
									latlong_envp.MaxX,
									latlong_envp.MaxY);
	sqlite3_exec(p_sql3_db_, buf, NULL, 0, &p_err_msg);
				
	str_sql = "CREATE TABLE tiles (zoom_level int, tile_column int, tile_row int, tile_data blob)";
	sqlite3_exec(p_sql3_db_, str_sql.c_str(), NULL, 0, &p_err_msg);
	if (p_err_msg!=NULL)
	{
		cout<<"ERROR: sqlite: "<<p_err_msg<<endl;
		delete[]p_err_msg;
		Close();
		return;
	}		
}



BOOL	MBTileContainer::AddTile(int z, int x, int y, BYTE *p_data, unsigned int size)
{
	if (p_sql3_db_ == NULL) return FALSE;

	char buf[256];
	string str_sql;

	///*
	if (z>0) y = (1<<z) - y - 1;
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

BOOL		MBTileContainer::TileExists(int z, int x, int y)
{
	if (p_sql3_db_ == NULL) return FALSE;
		
	char buf[256];
	if (z>0) y = (1<<z) - y - 1;
	sprintf(buf,"SELECT * FROM tiles WHERE zoom_level = %d AND tile_column = %d AND tile_row = %d",z,x,y);
	string str_sql(buf);
		
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1,&stmt, &tail);
	
	BOOL result = (SQLITE_ROW == sqlite3_step (stmt));

		

	sqlite3_finalize(stmt);
	return result;	
}

BOOL		MBTileContainer::GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size)
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
	p_data = new BYTE[size];
	memcpy(p_data, sqlite3_column_blob(stmt, 3),size);
	sqlite3_finalize(stmt);

		
	return TRUE;
}

int 		MBTileContainer::GetTileList(list<pair<int,pair<int,int>>> &tile_list, int min_zoom, int max_zoom, string vector_file, MercatorProjType merc_type)
{
	if (p_sql3_db_ == NULL) return 0;
	char buf[256];
	string str_sql;

	max_zoom = (max_zoom < 0) ? 32 : max_zoom;
	sprintf(buf,"SELECT zoom_level,tile_column,tile_row FROM tiles WHERE zoom_level >= %d AND zoom_level <= %d",min_zoom,max_zoom);
	str_sql = buf;
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (p_sql3_db_, str_sql.c_str(), str_sql.size()+1,&stmt, &tail);
	
	VectorBorder *p_vb = VectorBorder::CreateFromVectorFile(vector_file,merc_type_);
  pair<int,pair<int,int>> p;
	while (SQLITE_ROW == sqlite3_step (stmt))
	{
		int z		= sqlite3_column_int(stmt, 0);
		int x		= sqlite3_column_int(stmt, 1);
		int y		= (z>0) ? ((1<<z) - sqlite3_column_int(stmt, 2) -1) : 0;
		if (x<0 || y<0 || z <0) continue;

		if (p_vb)
			if (! p_vb->Intersects(z,x,y)) continue;
    p.first = z;
    p.second.first=x;
    p.second.second =y;
		tile_list.push_back(p);
	}
	sqlite3_finalize(stmt);

		
	return tile_list.size();
};
	


OGREnvelope MBTileContainer::GetMercatorEnvelope()
{
	OGREnvelope envelope;
	envelope.MinX = (envelope.MaxX = (envelope.MinY = (envelope.MaxY = 0)));
	int bounds[128];
	if (!GetTileBounds(bounds)) return envelope;


	for (int z =0;z<32;z++)
	{
		if (bounds[4*z+2]>0 && bounds[4*z+3]>0)
			envelope = MercatorTileGrid::CalcEnvelopeByTileRange(z,bounds[4*z],bounds[4*z+1],bounds[4*z+2],bounds[4*z+3]);
	}
	return envelope;
};


BOOL		MBTileContainer::GetTileBounds (int tile_bounds[128])
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

BOOL MBTileContainer::Close ()
{
	if (p_sql3_db_!=NULL)
	{
		sqlite3_close(p_sql3_db_);
		p_sql3_db_ = NULL;
	}
	return TRUE;
};





TileFolder::TileFolder (TileName *p_tile_name, BOOL use_cache)
{
	p_tile_name_	= p_tile_name;
	use_cache_	= use_cache;
	p_tile_cache_		= (use_cache) ? new TileCache() :  NULL;
};



TileFolder::~TileFolder ()
{
	Close();
};

BOOL TileFolder::Close()
{
	delete(p_tile_cache_);
	p_tile_cache_ = NULL;
	return TRUE;
};

BOOL	TileFolder::OpenForReading (string folder_name)
{
  return FileExists(folder_name);
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



BOOL	TileFolder::AddTile(int z, int x, int y, BYTE *p_data, unsigned int size)
{
	if (use_cache_)	p_tile_cache_->AddTile(z,x,y,p_data,size);
	return writeTileToFile(z,x,y,p_data,size);
};



BOOL		TileFolder::GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size)
{
	if (use_cache_)	
  {
    if (p_tile_cache_->FindTile(z,x,y))
      return p_tile_cache_->GetTile(z,x,y,p_data,size); 
  }
 
  return ReadTileFromFile(z,x,y,p_data,size);
};

BOOL		TileFolder::TileExists(int z, int x, int y)
{
	return FileExists(p_tile_name_->GetFullTileName(z,x,y));
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


int 		TileFolder::GetTileList(list<pair<int,pair<int,int>>> &tile_list, int min_zoom, int max_zoom, string vector_file,  MercatorProjType merc_type)
{
	VectorBorder *p_vb = NULL;
	if (vector_file!="") 
	{
		if( !(p_vb = VectorBorder::CreateFromVectorFile(vector_file, merc_type))) 
		{
			cout<<"ERROR: can't open vector file: "<<vector_file<<endl;
			return 0;
		}
	}

	list<string> file_list;
	FindFilesInFolderByExtension(file_list,p_tile_name_->GetBaseFolder(),TileName::TileExtension(p_tile_name_->tile_type_),TRUE);

  pair<int,pair<int,int>> p;
	for (list<string>::iterator iter = file_list.begin(); iter!=file_list.end();iter++)
	{
		int x,y,z;
		if(!p_tile_name_->ExtractXYZFromTileName((*iter),z,x,y)) continue;
		if ((max_zoom>=0)&&(z>max_zoom)) continue;
		if ((min_zoom>=0)&&(z<min_zoom)) continue;
		if (vector_file!="") 
			if (!p_vb->Intersects(z,x,y)) continue;
    p.first = z;
    p.second.first=x;
    p.second.second=y;
		tile_list.push_back(p);
	}
	delete(p_vb);
	return tile_list.size();	
};

OGREnvelope TileFolder::GetMercatorEnvelope()
{
	OGREnvelope envelope;
	envelope.MinX = (envelope.MaxX = (envelope.MinY = (envelope.MaxY = 0)));
	int bounds[128];
	if (!GetTileBounds(bounds)) return envelope;


	for (int z =0;z<32;z++)
	{
		if (bounds[4*z+2]>0 && bounds[4*z+3]>0)
			envelope = MercatorTileGrid::CalcEnvelopeByTileRange(z,bounds[4*z],bounds[4*z+1],bounds[4*z+2],bounds[4*z+3]);
		else break;
	}
	return envelope;
};


BOOL		TileFolder::GetTileBounds (int tile_bounds[128])
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

	
BOOL	TileFolder::writeTileToFile (int z, int x, int y, BYTE *p_data, unsigned int size)
{
	if (p_tile_name_==NULL) return FALSE;
	p_tile_name_->CreateFolder(z,x,y);
	return SaveDataToFile(p_tile_name_->GetFullTileName(z,x,y), p_data,size);
};

	
BOOL	TileFolder::ReadTileFromFile (int z,int x, int y, BYTE *&p_data, unsigned int &size)
{
	return ReadDataFromFile (p_tile_name_->GetFullTileName(z,x,y),p_data,size); 
};


}