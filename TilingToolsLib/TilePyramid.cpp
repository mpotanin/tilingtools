#include "StdAfx.h"
#include "TilePyramid.h"
using namespace GMT;

namespace GMT
{


TileContainer* GMTOpenTileContainerForReading (wstring fileName)
{
	TileContainer *poTC = NULL;
	if (MakeLower(fileName).find(L".mbtiles") != wstring::npos) poTC = new MBTileContainer();
	else poTC = new GMTileContainer();

	if (poTC->openForReading(fileName)) return poTC;
	else
	{
		poTC->close();
		delete(poTC);
		return NULL;
	}
};


GMTileContainer::GMTileContainer	(	wstring				containerFileName, 
										TileType			tileType,
										MercatorProjType	mercType,
										OGREnvelope			envelope, 
										int					maxZoom, 
										BOOL				useBuffer)
{
	int tileBounds[92];
	for (int i=0;i<23;i++)
	{
		tileBounds[4*i] = (tileBounds[4*i+1] = (tileBounds[4*i+2] = (tileBounds[4*i+3] = 0)));
		if (i<=maxZoom)
		{
			MercatorTileGrid::calcTileRange(envelope,i,tileBounds[4*i],tileBounds[4*i+1],tileBounds[4*i+2],tileBounds[4*i+3]);
			tileBounds[4*i+2]++;tileBounds[4*i+3]++;
		}
	}
	init(tileBounds,useBuffer,containerFileName,tileType,mercType);
};


GMTileContainer::GMTileContainer	() 
{
	this->maxTiles		= 0;
	this->sizes			= NULL; 
	this->offsets		= NULL;
	this->poTileBuffer	= NULL;
	this->USE_BUFFER	= FALSE;

};	

	/*
	GMTileContainer (wstring fileName)
	{
		if (!openForReading(fileName))
		{
			wcout<<L"Error: can't open for reading: "<<fileName<<endl;
		}
	}
	*/

BOOL GMTileContainer::openForReading (wstring containerFileName)
{
	
	this->bReadOnly		= TRUE;
	this->maxTiles		= 0;
	this->sizes			= NULL; 
	this->offsets		= NULL;
	this->poTileBuffer	= NULL;
	this->USE_BUFFER	= FALSE;
	
	if (!(this->containerFileData = _wfopen(containerFileName.c_str(),L"rb"))) return FALSE;
	BYTE	head[12];
	fread(head,1,12,this->containerFileData);
	if (!((head[0]=='G')&&(head[1]=='M')&&(head[2]=='T')&&(head[3]=='C'))) 
	{
		wcout<<L"Error: incorrect input tile container file: "<<containerFileName<<endl;
		return FALSE;
	}
		


	this->MAX_TILES_IN_CONTAINER = 0;
	this->mercType = (head[9] == 0) ? WORLD_MERCATOR : WEB_MERCATOR;
	this->tileType = (head[11] == 0) ? JPEG_TILE : (head[11]==1) ? PNG_TILE : TIFF_TILE;
	this->USE_BUFFER = FALSE;
	this->poTileBuffer = NULL;

	BYTE bounds[512];
	fread(bounds,1,512,this->containerFileData);

	this->maxTiles = 0;
	for (int i=0;i<23;i++)
	{
		this->minx[i] = *((int*)(&bounds[i*16]));
		this->miny[i] = *((int*)(&bounds[i*16+4]));
		this->maxx[i] = *((int*)(&bounds[i*16+8]));
		this->maxy[i] = *((int*)(&bounds[i*16+12]));
		//if (this->maxx[i]>0) 	this->maxZoom = i;
		this->maxTiles += (this->maxx[i]-this->minx[i])*(this->maxy[i]-this->miny[i]);
	}

	BYTE*			offset_size = new BYTE[this->maxTiles*13];
	fread(offset_size,1,this->maxTiles*13,this->containerFileData);


	this->sizes		= new unsigned int[this->maxTiles];
	this->offsets		= new unsigned __int64[this->maxTiles];

	for (int i=0; i<this->maxTiles;i++)
	{
		this->offsets[i]	= *((unsigned __int64*)(&offset_size[i*13]));
		if ((this->offsets[i]<<32) == 0)
			this->offsets[i]	= (this->offsets[i]>>32);
		this->sizes[i]	= *((unsigned int*)(&offset_size[i*13+8]));
	}

	delete[]offset_size;
	return TRUE;
};
	

GMTileContainer::~GMTileContainer()
{
	empty();
}
	
//fclose(containerFileData);


BOOL		GMTileContainer::addTile(int z, int x, int y, BYTE *pData, unsigned int size)
{
	if (USE_BUFFER)
	{
		__int64 n	= tileID(z,x,y);
		if ((n>= maxTiles)||(n<0)) return FALSE;
		sizes[n]		= size;
		return poTileBuffer->addTile(z,x,y,pData,size);
	}
	else return addTileToContainerFile(z,x,y,pData,size);
};

BOOL		GMTileContainer::getTile(int z, int x, int y, BYTE *&pData, unsigned int &size)
{
	if (USE_BUFFER)		return poTileBuffer->getTile(z,x,y,pData,size);
	else				return getTileFromContainerFile(z,x,y,pData,size);
};

BOOL		GMTileContainer::tileExists(int z, int x, int y)
{
	unsigned int n = tileID(z,x,y);
	if (n>= maxTiles && n<0) return FALSE;
	return (sizes[n]>0) ? TRUE : FALSE;
}; 

BOOL		GMTileContainer::close()
{
	if (bReadOnly)
	{
		empty();
		return TRUE;
	}
	

	if (USE_BUFFER) 
	{
		writeTilesToContainerFileFromBuffer();
		writeContainerFileHeader();
	}
	else 
	{
		wstring	fileTempName	= containerFileName + L".temp";
		FILE	*fpNew;
		if (!(fpNew = _wfopen(fileTempName.c_str(),L"wb"))) 	return FALSE;
		BYTE	*header;
		this->writeHeaderToByteArray(header);
		fwrite(header,1,headerSize(),fpNew);
		delete[]header;
		fseek(containerFileData,headerSize(),0);
		
		int blockLen = 10000000;
		BYTE	*block = new BYTE[blockLen];
		for (unsigned __int64 i = headerSize(); i<containerLength; i+=blockLen)
		{
			int blockLen_ = (i+blockLen > containerLength) ? containerLength - i : blockLen;

			fread(block,1,blockLen_,containerFileData);
			fwrite(block,1,blockLen_,fpNew);
		}
		delete[]block;

		fclose(containerFileData);
		containerFileData = NULL;

		fclose(fpNew);
		DeleteFile(containerFileName.c_str());
		_wrename(fileTempName.c_str(),containerFileName.c_str());
	}
	
	empty();
	return TRUE;		
};


int 		GMTileContainer::getTileList(	list<__int64> &tileList, 
											int minZoom, 
											int maxZoom, 
											wstring vectorFile, 
											MercatorProjType mercType
											)
{
	VectorBorder *poVB = NULL;

	if (vectorFile!=L"") 
	{
		if(!(poVB = VectorBorder::createFromVectorFile(vectorFile,this->mercType))) 
		{
			wcout<<L"Error: can't open vector file: "<<vectorFile<<endl;
			return 0;
		}
	}

	for (unsigned int i=0; i<maxTiles;i++)
	{
		int x,y,z;
		if (sizes[i]>0) 
		{
			if(!tileXYZ(i,z,x,y)) continue;
			if ((maxZoom>=0)&&(z>maxZoom)) continue;
			if ((minZoom>=0)&&(z<minZoom)) continue;
			if (vectorFile!=L"") 
				if (!poVB->intersects(z,x,y)) continue;
			tileList.push_back(tileID(z,x,y));
		}
	}

	delete(poVB);
	return tileList.size();	
};
	
__int64		GMTileContainer::tileID( int z, int x, int y)
{
	unsigned int num = 0;
	for (int s=0;s<z;s++)
		num+=(maxx[s]-minx[s])*(maxy[s]-miny[s]);
	return (num + (maxx[z]-minx[z])*(y-miny[z]) + x-minx[z]);
};

BOOL		GMTileContainer::tileXYZ(__int64 n, int &z, int &x, int &y)
{
	if ((maxTiles>0) && (n>=maxTiles)) return FALSE; 
	int s = 0;
	for (z=0;z<23;z++)
	{
		if ((s+(maxx[z]-minx[z])*(maxy[z]-miny[z]))>n) break;
		s+=(maxx[z]-minx[z])*(maxy[z]-miny[z]);
	}
	y = miny[z] + ((n-s)/(maxx[z]-minx[z]));
	x = minx[z] + ((n-s)%(maxx[z]-minx[z]));

	return TRUE;
};

TileType	GMTileContainer::getTileType()
{
	return this->tileType;
};

MercatorProjType	GMTileContainer::getProjType()
{
	return this->mercType;
};

OGREnvelope GMTileContainer::getMercatorEnvelope()
{
	OGREnvelope envelope;
	envelope.MinX = (envelope.MaxX = (envelope.MinY = (envelope.MaxY = 0)));

	for (int z =0;z<23;z++)
	{
		if (maxx[z]>0 && maxy[z]>0)
			envelope = MercatorTileGrid::calcEnvelopeByTileRange(z,minx[z],miny[z],maxx[z],maxy[z]);
		else break;
	}
	return envelope;
};

int			GMTileContainer::getMaxZoom()
{
	int maxZ = -1;
	for (int z=0;z<23;z++)
		if (maxx[z]>0) maxZ = z;
	return maxZ;
};

BOOL 	GMTileContainer::init	(	int					tileBounds[92], 
									BOOL				useBuffer, 
									wstring				containerFileName, 
									TileType			tileType,
									MercatorProjType	mercType)
{
	this->bReadOnly					= FALSE;
	this->tileType					= tileType;
	this->mercType					= mercType;
	this->containerFileName			= containerFileName;// + L".tiles";
	this->containerFileData			= NULL;
	this->containerLength			= 0;
	this->MAX_TILES_IN_CONTAINER	= 0;

	if (useBuffer) 
	{
		this->USE_BUFFER	= TRUE;
		this->poTileBuffer	= new TileBuffer();
	}
	else 
	{
		this->USE_BUFFER	= FALSE;
		this->poTileBuffer	= NULL;
	}

	this->sizes		= NULL;
	this->offsets		= NULL;
	int maxZoom = 0;
	for (int z =0; z<23;z++)
	{
		if (tileBounds[4*z+2]>0)
		{
			maxZoom = z;
			minx[z] = tileBounds[4*z];
			miny[z] = tileBounds[4*z+1];
			maxx[z] = tileBounds[4*z+2];
			maxy[z] = tileBounds[4*z+3];
		}
		else minx[z] = (miny[z] = (maxx[z] = (maxy[z] = 0)));
	}

	this->maxTiles = tileID(maxZoom,maxx[maxZoom]-1,maxy[maxZoom]-1) + 1;

	sizes		= new unsigned int[maxTiles];
	offsets		= new unsigned __int64[maxTiles];
	for (unsigned int i=0;i<maxTiles;i++)
	{
		sizes[i]		= 0;
		offsets[i]		= 0;
	}
	return TRUE;
};


BOOL	GMTileContainer::writeContainerFromBuffer()
{
	if (USE_BUFFER) 
	{
		writeTilesToContainerFileFromBuffer();
		writeContainerFileHeader();
	}
	return TRUE;
}


BOOL	GMTileContainer::addTileToContainerFile(int z, int x, int y, BYTE *pData, unsigned int size)
{
	unsigned int n	= tileID(z,x,y);
	if (n>= maxTiles) return FALSE;

	if (!containerFileData)
	{
		if (!(containerFileData = _wfopen(containerFileName.c_str(),L"wb+")))
		{
			wcout<<L"Can't add tile to file: "<<containerFileName<<endl;
			return FALSE;
		}
		BYTE	*header;
		this->writeHeaderToByteArray(header);
		fwrite(header,1,headerSize(),containerFileData);
		delete[]header;
	}


	_fseeki64(containerFileData,0,SEEK_END);
	fwrite(pData,sizeof(BYTE),size,containerFileData);
	sizes[n]		= size;
	if (containerLength == 0) containerLength = headerSize();
	offsets[n] = containerLength;
	containerLength +=size;
	
	return TRUE;
};
	
BOOL	GMTileContainer::getTileFromContainerFile (int z, int x, int y, BYTE *&pData, unsigned int &size)
{
	unsigned int n	= tileID(z,x,y);
	if (n>= maxTiles && n<0) return FALSE;
	if (!(size = sizes[n])) return TRUE;
	
	if (!containerFileData)
	{
		if (!(containerFileData = _wfopen(containerFileName.c_str(),L"rb")))
		{
			wcout<<L"Can't add tile to file: "<<containerFileName<<endl;
			return FALSE;
		}
	}

	_fseeki64(containerFileData,offsets[n],0);
		
	pData			= new BYTE[sizes[n]];
	//Huge file _fseeki64 _ftelli64 in Visual C++
	fread(pData,1,size,containerFileData);
};

BOOL	GMTileContainer::writeTilesToContainerFileFromBuffer()
{
	if (!containerFileData)
	{
		if (!(containerFileData = _wfopen(containerFileName.c_str(),L"wb+")))
		{
			wcout<<L"Can't create file: "<<containerFileName<<endl;
			return FALSE;
		}
	}
	fseek(containerFileData,headerSize(),0);

	
	offsets[0] = headerSize();
	for (unsigned int k =1; k<maxTiles; k+=1)
		offsets[k] = offsets[k-1]+sizes[k-1];
	

	for (unsigned int k =0; k<maxTiles; k+=1000)
	{
		unsigned int l = (k+1000<maxTiles) ? k+1000 : maxTiles;
		if (offsets[l-1]-offsets[k] + sizes[l-1]==0) continue;

		BYTE *block = new BYTE[offsets[l-1]-offsets[k] + sizes[l-1]];
		unsigned int len = 0;
		for (unsigned int i = k; i<l; i++)
		{
			if (sizes[i]!=0) //ToCorrect
			{
				BYTE	*tileData = NULL;
				unsigned int tileSize;
				int x,y,z;
				this->tileXYZ(i,z,x,y);
				if (!poTileBuffer->getTile(z,x,y,tileData,tileSize)) continue;
				memcpy(&block[len],tileData,tileSize);
				len+=tileSize;
				delete[]tileData;
			}
		}
		
		fwrite(block,sizeof(BYTE),offsets[l-1]-offsets[k] + sizes[l-1],containerFileData);
		delete[]block;
	}
	return TRUE;
};

BOOL	GMTileContainer::writeContainerFileHeader()
{
	if (!containerFileData) return FALSE;
	fseek(containerFileData,0,SEEK_SET);
	
	BYTE	*containerHead;
	writeHeaderToByteArray(containerHead);
	fwrite(containerHead,1,headerSize(),containerFileData);
	fclose(containerFileData);
	containerFileData = NULL;
	delete[]containerHead;
	return TRUE;
};


BOOL	GMTileContainer::writeHeaderToByteArray(BYTE*	&pData)
{
	pData = new BYTE[headerSize()];
	string gmtc = "GMTC";
	memcpy(pData,gmtc.c_str(),4);
	memcpy(&pData[4],&MAX_TILES_IN_CONTAINER,4);
	pData[8]	= 0;
	pData[9]	= mercType;
	pData[10]	= 0;
	pData[11]	= tileType;

	for (int z=0;z<23;z++)
	{
		memcpy(&pData[12+z*16],&minx[z],4);
		memcpy(&pData[12+z*16+4],&miny[z],4);
		memcpy(&pData[12+z*16+8],&maxx[z],4);
		memcpy(&pData[12+z*16+12],&maxy[z],4);
	}
	int t[36] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	memcpy(&pData[12+23*16],t,144);

	BYTE	tileInfo[13];
	for (unsigned int i = 0; i<maxTiles; i++)
	{
		if (sizes[i]==0)
		{
			for (int j = 0; j<13;j++)
				tileInfo[j] = 0;
		}
		else
		{
			memcpy(&tileInfo,&offsets[i],8);
			memcpy(&tileInfo[8],&sizes[i],4);
			tileInfo[12]	=0;
		}
		memcpy(&pData[12 + 512 + 13*i],tileInfo,13);
	}

	return TRUE;
};

	
unsigned int GMTileContainer::headerSize()
{
	return (4+4+4+512+maxTiles*(4+8+1));
};

void GMTileContainer::empty ()
{
	delete[]sizes;
	sizes = NULL;
	delete[]offsets;
	offsets = NULL;
	maxTiles = 0;
		
	for (int i=0;i<23;i++)
		maxx[i]=(minx[i]=(maxy[i]=(miny[i]=0)));
	delete(poTileBuffer);
	poTileBuffer = NULL;
};
	





MBTileContainer::MBTileContainer ()
{
	this->pDB = NULL;
};

MBTileContainer::~MBTileContainer ()
{
	close();
}

/*
MBTileContainer (wstring fileName)
{
	if (! openForReading(fileName))
	{
		wcout<<L"Error: can't open for reading: "<<fileName<<endl;
	}
}
*/
	
int		MBTileContainer::getMaxZoom()
{
	if (this->pDB == NULL) return -1;
	string strSQL = "SELECT MAX(zoom_level) FROM tiles";
	sqlite3_stmt *stmt	= NULL;
	if (SQLITE_OK != sqlite3_prepare_v2 (this->pDB, strSQL.c_str(), strSQL.size()+1, &stmt, NULL)) return -1;
	
	int maxZoom = -1;
	if (SQLITE_ROW == sqlite3_step (stmt))
		maxZoom = sqlite3_column_int(stmt,0);
	sqlite3_finalize(stmt);
	return maxZoom;
};
	
BOOL MBTileContainer::openForReading  (wstring fileName)
{
	string		fileNameUTF8;
	wstrToUtf8(fileNameUTF8,fileName);
	if (SQLITE_OK != sqlite3_open(fileNameUTF8.c_str(),&this->pDB))
	{
		wcout<<L"Error: can't open mbtiles file: "<<fileName<<endl;
		return FALSE;
	}

	string strSQL = "SELECT * FROM metadata WHERE name = 'format'";
	sqlite3_stmt *stmt	= NULL;
	sqlite3_prepare_v2 (this->pDB, strSQL.c_str(), strSQL.size()+1, &stmt, NULL);
	
	if (SQLITE_ROW == sqlite3_step (stmt))
	{
		string format(reinterpret_cast<const char *>(sqlite3_column_text(stmt,1)), Ustrlen(sqlite3_column_text(stmt,1)));
		this->tileType = (format == "jpg") ? JPEG_TILE : PNG_TILE;
		sqlite3_finalize(stmt);
	}
	else
	{
		this->tileType = PNG_TILE;
		sqlite3_finalize(stmt);
	}

	strSQL = "SELECT * FROM metadata WHERE name = 'projection'";
	sqlite3_prepare_v2 (this->pDB, strSQL.c_str(), strSQL.size()+1, &stmt, NULL);
	if (SQLITE_ROW == sqlite3_step (stmt))
	{

		string proj(reinterpret_cast<const char *>(sqlite3_column_text(stmt,1)), Ustrlen(sqlite3_column_text(stmt,1)));
		this->mercType	=	(proj	== "0" || proj	== "WORLD_MERCATOR" || proj	== "EPSG:3395") ? WORLD_MERCATOR : WEB_MERCATOR;
		sqlite3_finalize(stmt);
	}
	else
	{
		this->mercType	=	WEB_MERCATOR;
		sqlite3_finalize(stmt);
	}

	return TRUE;
}


MBTileContainer::MBTileContainer (wstring fileName, TileType tileType,MercatorProjType mercType, OGREnvelope mercEnvelope)
{
	pDB = NULL;
	if (FileExists(fileName)) DeleteFile(fileName.c_str());
	string		fileNameUTF8;
	wstrToUtf8(fileNameUTF8,fileName);
	if (SQLITE_OK != sqlite3_open(fileNameUTF8.c_str(),&this->pDB)) return;

	string	strSQL = "CREATE TABLE metadata (name text, value text)";
	char	*errMsg = NULL;
	sqlite3_exec(this->pDB, strSQL.c_str(), NULL, 0, &errMsg);
	if (errMsg!=NULL)
	{
		cout<<"Error: sqlite: "<<errMsg<<endl;
		delete[]errMsg;
		close();
		return;
	}

	strSQL = "INSERT INTO metadata VALUES ('format',";
	strSQL += (tileType == JPEG_TILE) ? "'jpg')" :  (tileType == PNG_TILE) ? "'png')" : "'tif')";
	sqlite3_exec(this->pDB, strSQL.c_str(), NULL, 0, &errMsg);

	strSQL = "INSERT INTO metadata VALUES ('projection',";
	strSQL += (mercType == WORLD_MERCATOR) ? "'WORLD_MERCATOR')" :  "'WEB_MERCATOR')";
	sqlite3_exec(this->pDB, strSQL.c_str(), NULL, 0, &errMsg);
		
	OGREnvelope latlongEnvelope;
	latlongEnvelope.MinX = MercatorTileGrid::mercToLong(mercEnvelope.MinX, mercType);
	latlongEnvelope.MaxX = MercatorTileGrid::mercToLong(mercEnvelope.MaxX, mercType);
	latlongEnvelope.MinY = MercatorTileGrid::mercToLat(mercEnvelope.MinY, mercType);
	latlongEnvelope.MaxY = MercatorTileGrid::mercToLat(mercEnvelope.MaxY, mercType);
	char	buf[256];
	sprintf(buf,"INSERT INTO metadata VALUES ('bounds', '%lf,%lf,%lf,%lf')",
									latlongEnvelope.MinX,
									latlongEnvelope.MinY,
									latlongEnvelope.MaxX,
									latlongEnvelope.MaxY);
	sqlite3_exec(this->pDB, buf, NULL, 0, &errMsg);
				
	strSQL = "CREATE TABLE tiles (zoom_level int, tile_column int, tile_row int, tile_data blob)";
	sqlite3_exec(this->pDB, strSQL.c_str(), NULL, 0, &errMsg);
	if (errMsg!=NULL)
	{
		cout<<"Error: sqlite: "<<errMsg<<endl;
		delete[]errMsg;
		close();
		return;
	}		
}



BOOL	MBTileContainer::addTile(int z, int x, int y, BYTE *pData, unsigned int size)
{
	if (this->pDB == NULL) return FALSE;

	char buf[256];
	string strSQL;

	///*
	if (z>0) y = (1<<z) - y - 1;
	if (tileExists(z,x,y))
	{
		sprintf(buf,"DELETE * FROM tiles WHERE zoom_level = %d AND tile_column = %d AND tile_row = %d",z,x,y);
		strSQL = buf;
		char	*errMsg = NULL;
		if (SQLITE_DONE != sqlite3_exec(pDB, strSQL.c_str(), NULL, 0, &errMsg))
		{
			cout<<L"Error: can't delete existing tile: "<<strSQL<<endl;	
			return FALSE;
		}
	}
	//*/
	sprintf(buf,"INSERT INTO tiles VALUES(%d,%d,%d,?)",z,x,y);
	strSQL = buf;
	const char *tail	=	NULL;
	sqlite3_stmt *stmt	=	NULL;
	int k = sqlite3_prepare_v2(this->pDB,strSQL.c_str(), strSQL.size()+1,&stmt, &tail);
	sqlite3_bind_blob(stmt,1,pData,size,SQLITE_TRANSIENT);
	k = sqlite3_step(stmt);
	if (SQLITE_DONE != k) 
	{
		cout<<"Error sqlite-message: "<<sqlite3_errmsg(this->pDB)<<endl;
		sqlite3_finalize(stmt);
		return FALSE;
	}
	sqlite3_finalize(stmt);

	return TRUE;
}

BOOL		MBTileContainer::tileExists(int z, int x, int y)
{
	if (this->pDB == NULL) return FALSE;
		
	char buf[256];
	if (z>0) y = (1<<z) - y - 1;
	sprintf(buf,"SELECT * FROM tiles WHERE zoom_level = %d AND tile_column = %d AND tile_row = %d",z,x,y);
	string strSQL(buf);
		
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (this->pDB, strSQL.c_str(), strSQL.size()+1,&stmt, &tail);
	
	BOOL result = (SQLITE_ROW == sqlite3_step (stmt));

		

	sqlite3_finalize(stmt);
	return result;	
}

BOOL		MBTileContainer::getTile(int z, int x, int y, BYTE *&pData, unsigned int &size)
{
	if (this->pDB == NULL) return FALSE;
	char buf[256];
	string strSQL;

	if (z>0) y = (1<<z) - y - 1;
	sprintf(buf,"SELECT * FROM tiles WHERE zoom_level = %d AND tile_column = %d AND tile_row = %d",z,x,y);
	strSQL = buf;
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (this->pDB, strSQL.c_str(), strSQL.size()+1,&stmt, &tail);
	
	if (SQLITE_ROW != sqlite3_step (stmt))
	{
		sqlite3_finalize(stmt);
		return FALSE;	
	}

	size		= sqlite3_column_bytes(stmt, 3);
	pData = new BYTE[size];
	memcpy(pData, sqlite3_column_blob(stmt, 3),size);
	sqlite3_finalize(stmt);

		
	return TRUE;
}

int 		MBTileContainer::getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile, MercatorProjType mercType)
{
	if (this->pDB == NULL) return 0;
	char buf[256];
	string strSQL;

	maxZoom = (maxZoom < 0) ? 23 : maxZoom;
	sprintf(buf,"SELECT zoom_level,tile_column,tile_row FROM tiles WHERE zoom_level >= %d AND zoom_level <= %d",minZoom,maxZoom);
	strSQL = buf;
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (this->pDB, strSQL.c_str(), strSQL.size()+1,&stmt, &tail);
	
	VectorBorder *pVB = VectorBorder::createFromVectorFile(vectorFile,this->mercType);
	while (SQLITE_ROW == sqlite3_step (stmt))
	{
		int z		= sqlite3_column_int(stmt, 0);
		int x		= sqlite3_column_int(stmt, 1);
		int y		= (z>0) ? ((1<<z) - sqlite3_column_int(stmt, 2) -1) : 0;
		if (pVB)
			if (! pVB->intersects(z,x,y)) continue;
			
		tileList.push_back(tileID(z,x,y));
	}
	sqlite3_finalize(stmt);

		
	return tileList.size();
};
	


OGREnvelope MBTileContainer::getMercatorEnvelope()
{
	OGREnvelope envelope;
	envelope.MinX = (envelope.MaxX = (envelope.MinY = (envelope.MaxY = 0)));
	int bounds[92];
	if (!getTileBounds(bounds)) return envelope;


	for (int z =0;z<23;z++)
	{
		if (bounds[4*z+2]>0 && bounds[4*z+3]>0)
			envelope = MercatorTileGrid::calcEnvelopeByTileRange(z,bounds[4*z],bounds[4*z+1],bounds[4*z+2],bounds[4*z+3]);
	}
	return envelope;
};


BOOL		MBTileContainer::getTileBounds (int tileBounds[92])
{
	for (int z=0; z<23; z++)
	{
		tileBounds[4*z]	= 	(tileBounds[4*z+1] =	-1);
		tileBounds[4*z + 2]	= 	(tileBounds[4*z+3] =	0);
	}

		

	if (this->pDB == NULL) return FALSE;
	char buf[256];
	string strSQL;

	sprintf(buf,"SELECT zoom_level,tile_column,tile_row FROM tiles");
	strSQL = buf;
	sqlite3_stmt *stmt	=	NULL;
	const char *tail	=	NULL;
	sqlite3_prepare_v2 (this->pDB, strSQL.c_str(), strSQL.size()+1,&stmt, &tail);
	
	while (SQLITE_ROW == sqlite3_step (stmt))
	{
		int z		= sqlite3_column_int(stmt, 0);
		int x		= sqlite3_column_int(stmt, 1);
		int y		= (z>0) ? ((1<<z) - sqlite3_column_int(stmt, 2) -1) : 0;

		tileBounds[4*z]		= (tileBounds[4*z] == -1 || tileBounds[4*z] > x) ? x : tileBounds[4*z];
		tileBounds[4*z+1]	= (tileBounds[4*z+1] == -1 || tileBounds[4*z+1] > y) ? y : tileBounds[4*z+1];
		tileBounds[4*z+2]	= (tileBounds[4*z+2] == 0 || tileBounds[4*z+2] < x + 1) ? x + 1 : tileBounds[4*z+2];
		tileBounds[4*z+3]	= (tileBounds[4*z+3] == 0 || tileBounds[4*z+3] < y + 1) ? y + 1 : tileBounds[4*z+3];
	}

	for (int z=0; z<23; z++)
	{
		tileBounds[4*z]		= (tileBounds[4*z] == -1) ? 0 : tileBounds[4*z];
		tileBounds[4*z+1]	= (tileBounds[4*z+1] == -1) ? 0 : tileBounds[4*z+1];

	}

	sqlite3_finalize(stmt);
		
	return TRUE;
};

TileType	MBTileContainer::getTileType()
{
	return this->tileType;
};

MercatorProjType	MBTileContainer::getProjType()
{
	return this->mercType;
};

BOOL MBTileContainer::close ()
{
	if (pDB!=NULL)
	{
		sqlite3_close(pDB);
		pDB = NULL;
	}
	return FALSE;
};





TileFolder::TileFolder (TileName *poTileName, BOOL useBuffer)
{
	this->poTileName	= poTileName;
	this->USE_BUFFER	= useBuffer;
	poTileBuffer		= (useBuffer) ? new TileBuffer() :  NULL;
};
	
TileFolder::~TileFolder ()
{
	close();
};

BOOL TileFolder::close()
{
	delete(poTileBuffer);
	poTileBuffer = NULL;
	return TRUE;
};

BOOL	TileFolder::addTile(int z, int x, int y, BYTE *pData, unsigned int size)
{
	if (USE_BUFFER)	poTileBuffer->addTile(z,x,y,pData,size);
	return writeTileToFile(z,x,y,pData,size);
};



BOOL		TileFolder::getTile(int z, int x, int y, BYTE *&pData, unsigned int &size)
{
	if (USE_BUFFER)	return poTileBuffer->getTile(z,x,y,pData,size); 
	else			return readTileFromFile(z,x,y,pData,size);
};

BOOL		TileFolder::tileExists(int z, int x, int y)
{
	return FileExists(poTileName->getFullTileName(z,x,y));
};


int			TileFolder::getMaxZoom()
{
	int maxZ = -1;
	int tileBounds[92];
	if (!getTileBounds(tileBounds)) return -1;
	for (int z=0;z<23;z++)
		if (tileBounds[4*z+2]>0) maxZ = z;
	return maxZ;
};


int 		TileFolder::getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile,  MercatorProjType mercType)
{
	VectorBorder *poVB = NULL;
	if (vectorFile!=L"") 
	{
		if( !(poVB = VectorBorder::createFromVectorFile(vectorFile, mercType))) 
		{
			wcout<<L"Error: can't open vector file: "<<vectorFile<<endl;
			return 0;
		}
	}

	list<wstring> filesList;
	FindFilesInFolderByExtension(filesList,this->poTileName->getBaseFolder(),TileName::tileExtension(poTileName->tileType),TRUE);

	for (list<wstring>::iterator iter = filesList.begin(); iter!=filesList.end();iter++)
	{
		int x,y,z;
		if(!this->poTileName->extractXYZFromTileName((*iter),z,x,y)) continue;
		if ((maxZoom>=0)&&(z>maxZoom)) continue;
		if ((minZoom>=0)&&(z<minZoom)) continue;
		if (vectorFile!=L"") 
			if (!poVB->intersects(z,x,y)) continue;
		tileList.push_back(tileID(z,x,y));
	}
	delete(poVB);
	return tileList.size();	
};

OGREnvelope TileFolder::getMercatorEnvelope()
{
	OGREnvelope envelope;
	envelope.MinX = (envelope.MaxX = (envelope.MinY = (envelope.MaxY = 0)));
	int bounds[92];
	if (!getTileBounds(bounds)) return envelope;


	for (int z =0;z<23;z++)
	{
		if (bounds[4*z+2]>0 && bounds[4*z+3]>0)
			envelope = MercatorTileGrid::calcEnvelopeByTileRange(z,bounds[4*z],bounds[4*z+1],bounds[4*z+2],bounds[4*z+3]);
		else break;
	}
	return envelope;
};


BOOL		TileFolder::getTileBounds (int tileBounds[92])
{
	list<__int64> oTileList;
	if (!getTileList(oTileList,-1,-1,L"")) return FALSE;
	for (int z=0; z<23;z++)
	{
		tileBounds[4*z+3]=(tileBounds[4*z+2]=0);
		tileBounds[4*z]=(tileBounds[4*z+1]=(1<<(z+1)));
	}
	for (list<__int64>::const_iterator iter = oTileList.begin(); iter!=oTileList.end();iter++)
	{
		int z,x,y;
		if (tileXYZ((*iter),z,x,y))
		{
			if (tileBounds[4*z]>x)		tileBounds[4*z] = x;
			if (tileBounds[4*z+1]>y)	tileBounds[4*z+1] = y;
			if (tileBounds[4*z+2]<=x)	tileBounds[4*z+2] = (x+1);
			if (tileBounds[4*z+3]<=y)	tileBounds[4*z+3] = (y+1);
		}
	}

	return TRUE;
};

	
BOOL	TileFolder::writeTileToFile (int z, int x, int y, BYTE *pData, unsigned int size)
{
	if (poTileName==NULL) return FALSE;
	poTileName->createFolder(z,x,y);
	return SaveDataToFile(poTileName->getFullTileName(z,x,y), pData,size);
};

	
BOOL	TileFolder::readTileFromFile (int z,int x, int y, BYTE *&pData, unsigned int &size)
{
	return ReadDataFromFile (poTileName->getFullTileName(z,x,y),pData,size); 
};


}