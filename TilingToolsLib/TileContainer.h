#pragma once
#include "stdafx.h"
#include "TileName.h"
#include "TileCache.h"
#include "VectorBorder.h"
#include "histogram.h"
#include "TilingParameters.h"

namespace gmx
{

class TileContainerOptions
{
public:
  TileContainerOptions()
  {
    max_zoom_=-1;
    min_zoom_=-1;
    p_tile_name_=0;
    p_matrix_set_=0;
    tiling_srs_envp_.MaxX=-1;
    tiling_srs_envp_.MinX=1;
    tiling_srs_envp_.MaxY=-1;
    tiling_srs_envp_.MinY=1;
    tile_type_=TileType::NDEF_TILE_TYPE;
  };

public:
  string path_;
  int max_zoom_;
  int min_zoom_;
  TileName* p_tile_name_;
  map<string,string> extra_options_;
  ITileMatrixSet* p_matrix_set_;
  OGREnvelope tiling_srs_envp_;
  TileType  tile_type_;
};

class ITileContainer
{
public:
  virtual ~ITileContainer() {};
  virtual bool		          AddTile(int z, int x, int y, BYTE *p_data, unsigned int size) = 0;
	virtual	bool		          GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size) = 0;
	virtual	bool		          TileExists(int z, int x, int y) = 0; 
	virtual bool		          Close() = 0;
	virtual int 		          GetTileList(list<pair<int, pair<int,int>>> &tile_list, int min_zoom, int max_zoom, string vector_file = "") = 0;
	virtual int			          GetMaxZoom() = 0;
	virtual bool		          GetTileBounds (int tile_bounds[128]) = 0;
  virtual bool		          OpenForReading (string path) = 0;
	virtual TileType				  GetTileType() = 0;
	virtual MercatorProjType	GetProjType() = 0;
  virtual Metadata* GetMetadata () {return NULL;}; 
  virtual bool ExtractAndStoreMetadata (TilingParameters* p_params) {return true;}; 
  

	virtual __int64		TileID( int z, int x, int y)
	{
		__int64 n = ((((__int64)1)<<(2*z))-1)/3;
		n = n<<1;
		n += y*(((__int64)1)<<(z+1)) + x;
		return n;
	};

	virtual bool		TileXYZ(__int64 id, int &z, int &x, int &y)
	{
		int i = 0;
		for (i=0;i<32;i++)
			if (( (((((__int64)1)<<(2*i+2))-1)/3)<<1) > id) break;
		if (i==32) return FALSE;
		else z = i;
		
		__int64 n = (((((__int64)1)<<(2*z))-1)/3)<<1;

		y = ((id-n)>>(z+1));
		x = (id-n) % (1<<(z+1));

		return TRUE;
	};
};




class GMXTileContainer : public ITileContainer
{
public:
  static GMXTileContainer* OpenForWriting (TileContainerOptions *p_params);

  bool 		OpenForWriting			(	string				container_file_name, 
									              TileType			tile_type,
									              MercatorProjType	merc_type,
									              OGREnvelope			envelope, 
									              int					max_zoom, 
									              int         cache_size,
                                unsigned int max_volume_size = DEFAULT_MAX_VOLUME_SIZE
								              );

  bool 		OpenForWriting			( string				container_file_name, 
									              TileType			tile_type,
									              MercatorProjType	merc_type,
                               	int					tile_bounds[128], 
                                int         cache_size,
                                unsigned int max_volume_size = DEFAULT_MAX_VOLUME_SIZE
								              );

	GMXTileContainer				();
	~GMXTileContainer				();

	bool				OpenForReading			(string container_file_name);
	bool				AddTile			(int z, int x, int y, BYTE *p_data, unsigned int size);
	bool				GetTile			(int z, int x, int y, BYTE *&p_data, unsigned int &size);
	bool				TileExists		(int z, int x, int y);
	bool				Close			();
	int 				GetTileList		( list<pair<int, pair<int,int>>> &tile_list, 
											        int min_zoom,
                              int max_zoom, 
											        string vector_file = ""
										        );
	bool		GetTileBounds (int tile_bounds[128]);

  virtual Metadata* GetMetadata (); 
  virtual bool ExtractAndStoreMetadata (TilingParameters* p_params);

	__int64				TileID			( int z, int x, int y);
	bool				TileXYZ			(__int64 n, int &z, int &x, int &y);
	TileType			GetTileType	();
	MercatorProjType	GetProjType();
	int					GetMaxZoom		();

protected:
  static const unsigned int DEFAULT_MAX_VOLUME_SIZE = 0xffffffff;
	

protected:
  bool	AddTileToContainerFile(int z, int x, int y, BYTE *p_data, unsigned int size);
	bool	GetTileFromContainerFile (int z, int x, int y, BYTE *&p_data, unsigned int &size);
	bool	WriteTilesToContainerFileFromCache();
	
	bool	WriteHeaderToByteArray(BYTE*	&p_data);
	unsigned int HeaderSize();
	void MakeEmpty ();

  int               FillUpCurrentVolume();

  int                 GetVolumeNum (unsigned __int64 tile_offset);
  string              GetVolumeName (int num);
  unsigned __int64    GetTileOffsetInVolume (unsigned __int64 tile_container_offset);
  bool                DeleteVolumes();

protected:
	bool					read_only_;
	bool					use_cache_;
  bool          is_opened_;
	int						minx_[32];
	int						miny_[32];
	int						maxx_[32];
	int						maxy_[32];
	//int						max_zoom;
	unsigned int			max_tiles_;
	unsigned int			*p_sizes_;
	unsigned __int64		*p_offsets_;
	TileType				tile_type_;
	MercatorProjType		merc_type_;

	TileCache			*p_tile_cache_;
	unsigned int	max_volume_size_;
  int           max_volumes_;
	string				container_file_name_;
	Metadata*     p_metadata_;
  FILE**        pp_container_volumes_;
	unsigned __int64		container_byte_size_;

  HANDLE addtile_semaphore_;

};



class MBTileContainer  : public ITileContainer
{	

public:
  static MBTileContainer* OpenForWriting (TileContainerOptions *p_params);
	MBTileContainer ();
	~MBTileContainer ();
		
	int			GetMaxZoom();
  int			GetMinZoom();
	bool OpenForReading  (string file_name);
	MBTileContainer (string file_name, TileType tile_type,MercatorProjType merc_type, OGREnvelope merc_envp);
	
	bool		AddTile(int z, int x, int y, BYTE *p_data, unsigned int size);
	bool		TileExists(int z, int x, int y);

	bool		GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size);
	int 		GetTileList(list<pair<int, pair<int,int>>> &tile_list, int min_zoom, int max_zoom, string vector_file = "");
	
	bool		GetTileBounds (int tile_bounds[128]);
	TileType	GetTileType();
	MercatorProjType	GetProjType();
	bool Close ();

protected:
	sqlite3	*p_sql3_db_;
	TileType				tile_type_;
	MercatorProjType		merc_type_;
  bool  read_only_;
};


class TileFolder : public ITileContainer
{
public:
  static TileFolder* OpenForWriting (TileContainerOptions *p_params);
	TileFolder (TileName *p_tile_name, MercatorProjType merc_type, bool use_cache);
	~TileFolder ();
	bool  Close();
	bool	AddTile(int z, int x, int y, BYTE *p_data, unsigned int size);
	bool	GetTile(int z, int x, int y, BYTE *&p_data, unsigned int &size);
	bool	TileExists(int z, int x, int y);
	int		GetMaxZoom();
	int 	GetTileList(list<pair<int, pair<int,int>>> &tile_list, int min_zoom, int max_zoom, string vector_file = "");
	
  bool		GetTileBounds (int tile_bounds[128]);
	bool				OpenForReading			(string folder_name);
  TileType				GetTileType();
	MercatorProjType		GetProjType();
  void GetMetadata (Metadata* &p_metadata); 
	
protected:
	bool	writeTileToFile (int z, int x, int y, BYTE *p_data, unsigned int size);
	bool	ReadTileFromFile (int z,int x, int y, BYTE *&p_data, unsigned int &size);


protected:
	TileName	*p_tile_name_;
	bool		use_cache_;
	TileCache	*p_tile_cache_;
  HANDLE addtile_semaphore_;
  MercatorProjType		merc_type_;

};

class TileContainerFactory
{
public:

  static  bool GetTileContainerType (string strName,TileContainerType &eType);
  static ITileContainer* OpenForWriting(TileContainerType container_type, TileContainerOptions *p_params);
  static ITileContainer* OpenForReading (string file_name); //ToDo - move to TileContainerFactory
  static string GetExtensionByTileContainerType (TileContainerType container_type);

};

}
