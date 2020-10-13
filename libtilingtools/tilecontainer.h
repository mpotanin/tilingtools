#pragma once
#include "stdafx.h"
#include "tilename.h"
#include "tilecache.h"
#include "vectorborder.h"
#include "histogram.h"
#include "tilingparameters.h"
#include "rasterbuffer.h"

namespace ttx
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
    p_tile_bounds_ = 0;
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
  int* p_tile_bounds_;
};

class ITileContainer
{
public:
	virtual ~ITileContainer() {};
	virtual bool AddTile(int z, int x, int y, unsigned char *p_data, unsigned int size) = 0;
	virtual	bool GetTile(int z, int x, int y, unsigned char *&p_data, unsigned int &size) = 0;
	virtual	bool TileExists(int z, int x, int y) = 0; 
	virtual bool Close() = 0;
	virtual int GetTileList(list<pair<int, pair<int,int>>> &tile_list, int min_zoom, 
							int max_zoom, string vector_file = "") = 0;
	virtual int	GetMaxZoom() = 0;
	virtual bool GetTileBounds (int tile_bounds[128]) = 0;
	virtual bool OpenForReading (string path) = 0;
	virtual TileType GetTileType() = 0;
	virtual MercatorProjType GetProjType() = 0;
  
	virtual Metadata* GetMetadata () {return NULL;}; 
	virtual bool ExtractAndStoreMetadata (TilingParameters* p_params) {return true;}; 

	static int*		GetTileBounds(list<pair<int, pair<int, int>>> *p_tile_list)
	{
		int *p_tile_bounds = new int[128];
		for (int i = 0; i<128; i++)
			p_tile_bounds[i] = -1;

		for (auto iter : *p_tile_list)
		{
			int z = iter.first;
			int x = iter.second.first;
			int y = iter.second.second;
			p_tile_bounds[4 * z] = (p_tile_bounds[4 * z] == -1 || p_tile_bounds[4 * z] > x) ? x : p_tile_bounds[4 * z];
			p_tile_bounds[4 * z + 1] = (p_tile_bounds[4 * z + 1] == -1 || p_tile_bounds[4 * z + 1] > y) ? y : p_tile_bounds[4 * z + 1];
			p_tile_bounds[4 * z + 2] = (p_tile_bounds[4 * z + 2] == -1 || p_tile_bounds[4 * z + 2] < x) ? x : p_tile_bounds[4 * z + 2];
			p_tile_bounds[4 * z + 3] = (p_tile_bounds[4 * z + 3] == -1 || p_tile_bounds[4 * z + 3] < y) ? y : p_tile_bounds[4 * z + 3];
		}

		return p_tile_bounds;
	};
  
 	virtual int64_t		TileID( int z, int x, int y)
	{
		int64_t n = ((((int64_t)1)<<(2*z))-1)/3;
		n = n<<1;
		n += y*(((int64_t)1)<<(z+1)) + x;
		return n;
	};

	virtual bool		TileXYZ(int64_t id, int &z, int &x, int &y)
	{
		int i = 0;
		for (i=0;i<32;i++)
			if (( (((((int64_t)1)<<(2*i+2))-1)/3)<<1) > id) break;
		if (i==32) return false;
		else z = i;
		
		int64_t n = (((((int64_t)1)<<(2*z))-1)/3)<<1;

		y = ((id-n)>>(z+1));
		x = (id-n) % (1<<(z+1));

		return true;
	};

	
};


class GMXTileContainer : public ITileContainer
{
public:
	static GMXTileContainer* OpenForWriting (TileContainerOptions *p_params);

	bool 		OpenForWriting			(	string container_file_name, 
									        TileType tile_type,
									        MercatorProjType merc_type,
									        OGREnvelope envelope, 
									        int max_zoom, 
									        int cache_size,
											unsigned int max_volume_size = DEFAULT_MAX_VOLUME_SIZE
								              );

	bool 		OpenForWriting			(	string container_file_name, 
											TileType tile_type,
											MercatorProjType merc_type,
                               				int tile_bounds[128], 
											int cache_size,
											unsigned int max_volume_size = DEFAULT_MAX_VOLUME_SIZE
								              );

	GMXTileContainer				();
	~GMXTileContainer				();

	bool				OpenForReading			(string container_file_name);
	bool				AddTile			(int z, int x, int y, unsigned char* p_data, unsigned int size);
	//bool				GetWebMercTile (int z, int x, int y, unsigned char* &p_data, unsigned int &size);
	bool				GetTile			(int z, int x, int y, unsigned char* &p_data, unsigned int &size);
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

	int64_t				TileID			( int z, int x, int y);
	bool				TileXYZ			(int64_t n, int &z, int &x, int &y);
	TileType			GetTileType	();
	MercatorProjType	GetProjType();
	int					GetMaxZoom();

	//static RasterBuffer* CreateWebMercatorTileFromWorldMercatorTiles(GMXTileContainer *poSrcContainer,
	//	int nZ, int nX, int nY);

protected:
  static const unsigned int DEFAULT_MAX_VOLUME_SIZE = 0xffffffff;
	

protected:
  bool	AddTileToContainerFile(int z, int x, int y, unsigned char* p_data, unsigned int size);
	bool	GetTileFromContainerFile (int z, int x, int y, unsigned char* &p_data, unsigned int &size);
	bool	WriteTilesToContainerFileFromCache();
	
	bool	WriteHeaderToByteArray(unsigned char*	&p_data);
	unsigned int HeaderSize();
	void MakeEmpty ();

  int               FillUpCurrentVolume();

  int                 GetVolumeNum (uint64_t tile_offset);
  string              GetVolumeName (int num);
  uint64_t    GetTileOffsetInVolume (uint64_t tile_container_offset);
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
	uint64_t		*p_offsets_;
	TileType				tile_type_;
	MercatorProjType		merc_type_;

	TileCache			*p_tile_cache_;
	unsigned int	max_volume_size_;
  int           max_volumes_;
	string				container_file_name_;
	Metadata*     p_metadata_;
  FILE**        pp_container_volumes_;
	uint64_t		container_byte_size_;

  std::mutex  addtile_mutex_;

};



class MBTilesContainer  : public ITileContainer
{	

public:
  static MBTilesContainer* OpenForWriting (TileContainerOptions *p_params);
	MBTilesContainer ();
	~MBTilesContainer ();
		
	int			GetMaxZoom();
  int			GetMinZoom();
	bool OpenForReading  (string file_name);
	MBTilesContainer (string file_name, TileType tile_type,MercatorProjType merc_type, OGREnvelope merc_envp);
	
	bool		AddTile(int z, int x, int y, unsigned char *p_data, unsigned int size);
	bool		TileExists(int z, int x, int y);

	//bool		GetTile
	bool		GetTile(int z, int x, int y, unsigned char *&p_data, unsigned int &size);
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
	bool	AddTile(int z, int x, int y, unsigned char *p_data, unsigned int size);
	bool	GetTile(int z, int x, int y, unsigned char *&p_data, unsigned int &size);
	bool	TileExists(int z, int x, int y);
	int		GetMaxZoom();
	int 	GetTileList(list<pair<int, pair<int,int>>> &tile_list, int min_zoom, int max_zoom, string vector_file = "");
	
  bool		GetTileBounds (int tile_bounds[128]);
	bool				OpenForReading			(string folder_name);
  TileType				GetTileType();
	MercatorProjType		GetProjType();
  void GetMetadata (Metadata* &p_metadata); 
	
protected:
	bool	writeTileToFile (int z, int x, int y, unsigned char *p_data, unsigned int size);
	bool	ReadTileFromFile (int z,int x, int y, unsigned char *&p_data, unsigned int &size);


protected:
	TileName	*p_tile_name_;
	bool		use_cache_;
	TileCache	*p_tile_cache_;
  std::mutex addtile_mutex_;
  MercatorProjType		merc_type_;

};




class GTiffRasterFile : public ITileContainer
{
public:
  GTiffRasterFile(){m_poDS=0;};
  bool GetTile(int z, int x, int y, unsigned char *&p_data, unsigned int &size) {return true;};
  bool TileExists(int z, int x, int y) { return true; };
  int GetTileList(list<pair<int, pair<int, int>>> &tile_list, int min_zoom, int max_zoom, string vector_file = "")
  {return 0;};

  int	GetMaxZoom() {return 0;};

  bool GetTileBounds(int tile_bounds[128]) { return true; };
  bool OpenForReading(string path) { return true; };
  TileType GetTileType() {return TileType::JPEG_TILE;};
  MercatorProjType	GetProjType() {return MercatorProjType::WEB_MERCATOR;};

public:
  static GTiffRasterFile* OpenForWriting(TileContainerOptions *poTCOptions);
  
  bool AddTile(int z, int x, int y, unsigned char *p_data, unsigned int size);
  bool Close();

protected:
  bool InitByFirstAddedTile(unsigned char *p_data, unsigned int size);
  bool OpenForWriting(string strFileName, 
                      TileType eTileType, 
                      MercatorProjType	eMercType, 
                      int panTileBounds[128]);

protected:
  GDALDataset* m_poDS;
  string m_strFileName;
  int m_nZoom;
  TileType m_eTileType;
  MercatorProjType m_eMercType;
  int m_nMinX, m_nMaxX, m_nMaxY, m_nMinY;
  
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
