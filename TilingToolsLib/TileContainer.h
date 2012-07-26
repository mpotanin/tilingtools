#pragma once
#include "stdafx.h"
#include "TileName.h"


//unsigned int GetTiffTileType (GDALDataType type, int bands);
class TileContainer
{
public:
	~TileContainer(void)
	{
		empty();
	};

	virtual BOOL		addTile(int x, int y, int z, BYTE *pData, unsigned int size) = 0;
	virtual	BOOL		getTile(int x, int y, int z, BYTE *&pData, unsigned int &size) = 0;
	virtual	BOOL		tileExists(int x, int y, int z) = 0; 
	virtual BOOL		closeContainer() = 0;
	virtual int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile = L"") = 0;

	__int64		tileID( int z, int x, int y)
	{
		unsigned int num = 0;
		for (int s=0;s<z;s++)
			num+=(maxx[s]-minx[s])*(maxy[s]-miny[s]);
		return (num + (maxx[z]-minx[z])*(y-miny[z]) + x-minx[z]);
	};

	BOOL		tileXYZ(__int64 n, int &x, int &y, int &z)
	{
		if ((maxTiles>0) && (n>=maxTiles)) return FALSE; 
		int s = 0;
		for (z=0;z<32;z++)
		{
			if ((s+(maxx[z]-minx[z])*(maxy[z]-miny[z]))>n) break;
			s+=(maxx[z]-minx[z])*(maxy[z]-miny[z]);
		}
		y = miny[z] + ((n-s)/(maxx[z]-minx[z]));
		x = minx[z] + ((n-s)%(maxx[z]-minx[z]));

		return TRUE;
	};

protected:
	BOOL	initBounds (OGREnvelope	envelope)
	{
		for (int i=0;i<32;i++)
		{
			maxx[i]=(minx[i]=(maxy[i]=(miny[i]=0)));
			if (i<=maxZoom)
			{
				MercatorTileGrid::calcTileRange(envelope,i,minx[i],miny[i],maxx[i],maxy[i]);
				maxx[i]++;maxy[i]++;
			}
		}
		return TRUE;
	};
			


	BOOL	initBuffer(OGREnvelope			envelope, 
						int					maxZoom)
	{
		if (!initBounds(envelope)) return FALSE;
		this->USE_BUFFER	= TRUE;
		this->maxZoom		= maxZoom;
		maxTiles = tileID(maxZoom,maxx[maxZoom]-1,maxy[maxZoom]-1) + 1;

		sizes		= NULL;
		tileData	= NULL;
		offsets		= NULL;
		sizes		= new unsigned int[maxTiles];
		tileData	= new BYTE*[maxTiles];
		offsets		= new unsigned __int64[maxTiles];
		for (unsigned int i=0;i<maxTiles;i++)
		{
			sizes[i]		= 0;
			tileData[i]	= NULL;
			offsets[i]		= 0;
		}
		return TRUE;
	};

	BOOL	addTileToBuffer(int x, int y, int z, BYTE *pData, unsigned int size)
	{
		__int64 n	= tileID(z,x,y);
		if ((n>= maxTiles)&&(n<0)) return FALSE;
		if (sizes[n]>0) delete[]tileData[n];
	
		sizes[n]		= size;
		tileData[n]	= new BYTE[size];
		memcpy(tileData[n],pData,size);
		return TRUE;
	};

	//BOOL			tileExistsInBuffer (int x, int y, int z);

	BOOL	getTileFromBuffer(int x, int y, int z, BYTE *&pData, unsigned int &size)
	{
		__int64 n	= tileID(z,x,y);
		if ((n>= maxTiles)&&(n<0)) return FALSE;

		pData			= new BYTE[sizes[n]];
		memcpy(pData,tileData[n],sizes[n]);
		size			= sizes[n];
		return TRUE;
	};

	void empty ()
	{
		delete[]sizes;
		sizes = NULL;
		delete[]offsets;
		offsets = NULL;
		if (USE_BUFFER)
		{
			for (int i=0; i<maxTiles; i++)
				delete[]tileData[i];
			delete[]tileData;
			tileData =  NULL;
			maxTiles = 0;
		}

		for (int i=0;i<32;i++)
			maxx[i]=(minx[i]=(maxy[i]=(miny[i]=0)));
	};
	
protected:
	BOOL					USE_BUFFER;
	int						minx[32];
	int						miny[32];
	int						maxx[32];
	int						maxy[32];
	int						maxZoom;
	unsigned int			maxTiles;
	unsigned int			*sizes;
	BYTE					**tileData;
	unsigned __int64		*offsets;
	TileType				tileType;
	MercatorProjType		mercType;

};





class TileContainerFile : public TileContainer
{

public:
	TileContainerFile	(OGREnvelope		envelope, 
						int					maxZoom, 
						BOOL				useBuffer, 
						wstring				containerFileName, 
						TileType			tileType,
						MercatorProjType	mercType)
	{
		initContainer(envelope,maxZoom,useBuffer,containerFileName,tileType,mercType);		
	};

	TileContainerFile (wstring containerFileName)
	{
		if (!(this->containerFileData = _wfopen(containerFileName.c_str(),L"rb")))
		return;
		sizes		= NULL;
		tileData	= NULL;
		offsets		= NULL;

		BYTE	head[12];
		fread(head,1,12,containerFileData);
		this->MAX_TILES_IN_CONTAINER = 0;
		this->mercType = (head[9] == 0) ? WORLD_MERCATOR : WEB_MERCATOR;
		this->tileType = (head[11] == 0) ? JPEG_TILE : (head[11]==1) ? PNG_TILE : TIFF_TILE;
		this->USE_BUFFER = FALSE;
	

		BYTE bounds[512];
		fread(bounds,1,512,containerFileData);

		maxTiles = 0;
		for (int i=0;i<32;i++)
		{
			minx[i] = *((int*)(&bounds[i*16]));
			miny[i] = *((int*)(&bounds[i*16+4]));
			maxx[i] = *((int*)(&bounds[i*16+8]));
			maxy[i] = *((int*)(&bounds[i*16+12]));
			if (maxx[i]>0) 	maxZoom = i;
			maxTiles += (maxx[i]-minx[i])*(maxy[i]-miny[i]);
		}

		BYTE*			offset_size = new BYTE[maxTiles*13];
		fread(offset_size,1,maxTiles*13,containerFileData);


		sizes		= new unsigned int[maxTiles];
		offsets		= new unsigned __int64[maxTiles];

		for (int i=0; i<maxTiles;i++)
		{
			offsets[i]	= *((unsigned __int64*)(&offset_size[i*13]));
			sizes[i]	= *((unsigned int*)(&offset_size[i*13+8]));
		}

		delete[]offset_size;
	}
	
	//fclose(containerFileData);


	BOOL		addTile(int x, int y, int z, BYTE *pData, unsigned int size)
	{
		if (USE_BUFFER)	return addTileToBuffer(x,y,z,pData,size);
		else return addTileToContainerFile(x,y,z,pData,size);
	};

	BOOL		getTile(int x, int y, int z, BYTE *&pData, unsigned int &size)
	{
		if (USE_BUFFER)		return getTileFromBuffer(x,y,z,pData,size); 
		else				return getTileFromContainerFile(x,y,z,pData,size);
	};

	BOOL		tileExists(int x, int y, int z)
	{
		unsigned int n = tileID(z,x,y);
		if (n>= maxTiles && n<0) return FALSE;
		return (sizes[n]>0) ? TRUE : FALSE;
	}; 

	BOOL		closeContainer()
	{
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
	//BOOL			unpackAllTiles();
	int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile = L"")
	{
		VectorBorder vb;
		if (vectorFile!=L"") 
		{
			if(!VectorFile::OpenAndCreatePolygonInMercator(vectorFile, vb, this->mercType)) 
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
				tileXYZ(i,x,y,z);
				if ((z>=minZoom)&&(z<=maxZoom))
				{
					if (vectorFile!=L"") 
						if (!vb.Intersects(MercatorTileGrid::calcEnvelopeByTile(z,x,y))) continue;
					__int64 k = tileID(z,x,y);
					tileList.push_back(tileID(z,x,y));
				}
			}
		}

		return tileList.size();	
	};




	//TileContainerFile (wstring containerFileName, TileName *poTileName);

protected:
	BOOL	initContainer	(OGREnvelope		envelope, 
							int					maxZoom, 
							BOOL				useBuffer, 
							wstring				containerFileName, 
							TileType			tileType,
							MercatorProjType	mercType)
	{
		this->maxZoom			= maxZoom;
		this->tileType			= tileType;
		this->mercType			= mercType;
		this->containerFileName = containerFileName;// + L".tiles";
		this->containerFileData = NULL;
		if (USE_BUFFER) return initBuffer(envelope,maxZoom);
		else 
		{
			this->USE_BUFFER = FALSE;
			if (!initBounds(envelope)) return FALSE;
			maxTiles = tileID(maxZoom,maxx[maxZoom]-1,maxy[maxZoom]-1) + 1;

			tileData	= NULL;
			sizes		= NULL;
			offsets		= NULL;
			sizes		= new unsigned int[maxTiles];
			offsets		= new unsigned __int64[maxTiles];
			for (unsigned int i=0;i<maxTiles;i++)
			{
				sizes[i]		= 0;
				offsets[i]		= 0;
			}
		}
		return TRUE;
	};


	BOOL	writeContainerFromBuffer()
	{
		if (USE_BUFFER) 
		{
			writeTilesToContainerFileFromBuffer();
			writeContainerFileHeader();
		}
		return TRUE;
	}


	BOOL	addTileToContainerFile(int x, int y, int z, BYTE *pData, unsigned int size)
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


		fseek(containerFileData,0,SEEK_END);
		fwrite(pData,sizeof(BYTE),size,containerFileData);
		sizes[n]		= size;
		if (containerLength == 0) containerLength = headerSize();
		offsets[n] = containerLength;
		containerLength +=size;
	
		return TRUE;
	};
	
	BOOL	getTileFromContainerFile (int x, int y, int z, BYTE *&pData, unsigned int &size)
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

		fseek(containerFileData,offsets[n],0);
		pData			= new BYTE[sizes[n]];
		fread(pData,1,size,containerFileData);
	};

	BOOL	writeTilesToContainerFileFromBuffer()
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
				if (sizes[i]!=0)
				{
					memcpy(&block[len],tileData[i],sizes[i]);
					len+=sizes[i];
				}
			}
		
			fwrite(block,sizeof(BYTE),offsets[l-1]-offsets[k] + sizes[l-1],containerFileData);
			delete[]block;
			//unsigned int len = 0;
			//for (unsigned int i =k; i<k+1000;i++)
			//	len+=offsets[i];
		}
		return TRUE;
	};

	BOOL	writeContainerFileHeader()
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


	BOOL	writeHeaderToByteArray(BYTE*	&pData)
	{
		pData = new BYTE[headerSize()];
		string gmtc = "GMTC";
		memcpy(pData,gmtc.c_str(),4);
		memcpy(&pData[4],&MAX_TILES_IN_CONTAINER,4);
		pData[8]	= 0;
		pData[9]	= mercType;
		pData[10]	= 0;
		pData[11]	= tileType;

		int t[4] = {0,0,0,0};
		for (int z=0;z<32;z++)
		{
			if (z<=maxZoom)
			{
				memcpy(&pData[12+z*16],&minx[z],4);
				memcpy(&pData[12+z*16+4],&miny[z],4);
				memcpy(&pData[12+z*16+8],&maxx[z],4);
				memcpy(&pData[12+z*16+12],&maxy[z],4);
			}
			else memcpy(&pData[12+z*16],t,16);
		}


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

	
	unsigned int headerSize()
	{
		return (4+4+4+512+maxTiles*(4+8+1));
	};


protected:
	unsigned int			MAX_TILES_IN_CONTAINER;
	
	wstring					containerFileName;
	FILE*					containerFileData;
	unsigned __int64		containerLength;
};






class TileContainerFolder : public TileContainer
{
public:
	TileContainerFolder (TileName *poTileName, BOOL useBuffer, TileType tileType, MercatorProjType mercType, OGREnvelope envelope, int maxZoom)
	{
		this->poTileName	= poTileName;
		this->tileType		= tileType;
		this->mercType		= mercType;
		if (useBuffer)	initBuffer(envelope,maxZoom);
		else
		{
			this->maxTiles	= 0;
			this->maxZoom	= 31;
			tileData	= NULL;
			sizes		= NULL;
			offsets		= NULL;
			initBounds(envelope);
			this->USE_BUFFER	= FALSE;
		}
	};

	TileContainerFolder(TileName *poTileName)
	{
		this->poTileName = poTileName;
		this->USE_BUFFER = FALSE;
		this->maxTiles	= 0;
		this->maxZoom	= 31;
		tileData	= NULL;
		sizes		= NULL;
		offsets		= NULL;
		OGREnvelope envelope;
		double e = 0.01;
		envelope.MaxX = -2*(envelope.MinX = MercatorTileGrid::getULX()+e);
		envelope.MinY = -(envelope.MaxY = MercatorTileGrid::getULY()-e);
		initBounds(envelope);
	};

	BOOL	addTile(int x, int y, int z, BYTE *pData, unsigned int size)
	{
		if (USE_BUFFER)	addTileToBuffer(x,y,z,pData,size);
		return writeTileToFile(x,y,z,pData,size);
	};



	BOOL		getTile(int x, int y, int z, BYTE *&pData, unsigned int &size)
	{
		if (USE_BUFFER)	return getTileFromBuffer(x,y,z,pData,size); 
		else			return readTileFromFile(x,y,z,pData,size);
	};

	BOOL		tileExists(int x, int y, int z)
	{
		return FileExists(poTileName->getFullTileName(z,x,y));
	};

	BOOL		closeContainer()
	{
		if (USE_BUFFER) empty();
		return TRUE;
	};

	//BOOL			unpackAllTiles();
	int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile = L"")
	{
		VectorBorder vb;
		if (vectorFile!=L"") 
		{
			if(!VectorFile::OpenAndCreatePolygonInMercator(vectorFile, vb, this->mercType)) 
			{
				wcout<<L"Error: can't open vector file: "<<vectorFile<<endl;
				return 0;
			}
		}

		list<wstring> filesList;
		FindFilesInFolderByExtension(filesList,this->poTileName->getTilesFodler(),TileName::tileExtension(this->tileType),TRUE);

		for (list<wstring>::iterator iter = filesList.begin(); iter!=filesList.end();iter++)
		{
			int x,y,z;
			if(!this->poTileName->extractXYZFromTileName((*iter),z,x,y)) continue;
			if ((z>=minZoom)&&(z<=maxZoom)) 
			{
				if (vectorFile!=L"") 
					if (!vb.Intersects(MercatorTileGrid::calcEnvelopeByTile(z,x,y))) continue;
				tileList.push_back(tileID(z,x,y));
			}
		}
		return tileList.size();	
	};
	
protected:
	BOOL	writeTileToFile (int x, int y, int z, BYTE *pData, unsigned int size)
	{
		if (poTileName==NULL) return FALSE;
		poTileName->createFolder(z,x,y);
		return SaveDataToFile(poTileName->getFullTileName(z,x,y), pData,size);
	};

	
	BOOL	readTileFromFile (int x,int y, int z, BYTE *&pData, unsigned int &size)
	{
		return readDataFromFile (poTileName->getFullTileName(z,x,y),pData,size); 
	};


protected:
	TileName	*poTileName;
};


