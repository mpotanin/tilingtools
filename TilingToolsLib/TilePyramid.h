#pragma once
#include "stdafx.h"
#include "TileName.h"
#include "TileBuffer.h"
#include "VectorBorder.h"


//unsigned int GetTiffTileType (GDALDataType type, int bands);
class TilePyramid
{
public:
	~TilePyramid(void)
	{
		//empty();
	};

	virtual BOOL		addTile(int z, int x, int y, BYTE *pData, unsigned int size) = 0;
	virtual	BOOL		getTile(int z, int x, int y, BYTE *&pData, unsigned int &size) = 0;
	virtual	BOOL		tileExists(int z, int x, int y) = 0; 
	virtual BOOL		close() = 0;
	virtual int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile = L"", MercatorProjType mercType = WORLD_MERCATOR) = 0;
	virtual __int64		tileID( int z, int x, int y) = 0;
	virtual BOOL		tileXYZ(__int64 id, int &z, int &x, int &y) = 0;
	virtual BOOL		getTileBounds (int bounds[128]) = 0;
};





class TileContainer : public TilePyramid
{

public:
	TileContainer	(	OGREnvelope			envelope, 
						int					maxZoom, 
						BOOL				useBuffer, 
						wstring				containerFileName, 
						TileType			tileType,
						MercatorProjType	mercType)
	{
		int tileBounds[128];
		for (int i=0;i<32;i++)
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


	TileContainer	(	int					tileBounds[128], 
						BOOL				useBuffer, 
						wstring				containerFileName, 
						TileType			tileType,
						MercatorProjType	mercType)
	{
		init(tileBounds,useBuffer,containerFileName,tileType,mercType);
	};

	TileContainer	() 
	{
		this->maxTiles		= 0;
		this->sizes			= NULL; 
		this->offsets		= NULL;
		this->poTileBuffer	= NULL;
		this->USE_BUFFER	= FALSE;

	};

	static TileContainer* openForReading (wstring containerFileName)
	{
		TileContainer *poContainer = new TileContainer();


		if (!(poContainer->containerFileData = _wfopen(containerFileName.c_str(),L"rb")))
			return NULL;


		BYTE	head[12];
		fread(head,1,12,poContainer->containerFileData);
		if (!((head[0]=='G')&&(head[1]=='M')&&(head[2]=='T')&&(head[3]=='C'))) 
		{
			wcout<<L"Error: incorrect input tile container file: "<<containerFileName<<endl;
			return NULL;
		}
		


		poContainer->MAX_TILES_IN_CONTAINER = 0;
		poContainer->mercType = (head[9] == 0) ? WORLD_MERCATOR : WEB_MERCATOR;
		poContainer->tileType = (head[11] == 0) ? JPEG_TILE : (head[11]==1) ? PNG_TILE : TIFF_TILE;
		poContainer->USE_BUFFER = FALSE;
		poContainer->poTileBuffer = NULL;

		BYTE bounds[512];
		fread(bounds,1,512,poContainer->containerFileData);

		poContainer->maxTiles = 0;
		for (int i=0;i<32;i++)
		{
			poContainer->minx[i] = *((int*)(&bounds[i*16]));
			poContainer->miny[i] = *((int*)(&bounds[i*16+4]));
			poContainer->maxx[i] = *((int*)(&bounds[i*16+8]));
			poContainer->maxy[i] = *((int*)(&bounds[i*16+12]));
			if (poContainer->maxx[i]>0) 	poContainer->maxZoom = i;
			poContainer->maxTiles += (poContainer->maxx[i]-poContainer->minx[i])*(poContainer->maxy[i]-poContainer->miny[i]);
		}

		BYTE*			offset_size = new BYTE[poContainer->maxTiles*13];
		fread(offset_size,1,poContainer->maxTiles*13,poContainer->containerFileData);


		poContainer->sizes		= new unsigned int[poContainer->maxTiles];
		poContainer->offsets		= new unsigned __int64[poContainer->maxTiles];

		for (int i=0; i<poContainer->maxTiles;i++)
		{
			poContainer->offsets[i]	= *((unsigned __int64*)(&offset_size[i*13]));
			if ((poContainer->offsets[i]<<32) == 0)
				poContainer->offsets[i]	= (poContainer->offsets[i]>>32);
			poContainer->sizes[i]	= *((unsigned int*)(&offset_size[i*13+8]));
		}

		delete[]offset_size;
		return poContainer;
	};
	

	~TileContainer()
	{
		empty();
	}
	
	//fclose(containerFileData);


	BOOL		addTile(int z, int x, int y, BYTE *pData, unsigned int size)
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

	BOOL		getTile(int z, int x, int y, BYTE *&pData, unsigned int &size)
	{
		if (USE_BUFFER)		return poTileBuffer->getTile(z,x,y,pData,size);
		else				return getTileFromContainerFile(z,x,y,pData,size);
	};

	BOOL		tileExists(int z, int x, int y)
	{
		unsigned int n = tileID(z,x,y);
		if (n>= maxTiles && n<0) return FALSE;
		return (sizes[n]>0) ? TRUE : FALSE;
	}; 

	BOOL		close()
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


	int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile = L"", MercatorProjType mercType = WORLD_MERCATOR)
	{
		VectorBorder *poVB = NULL;

		if (vectorFile!=L"") 
		{
			if(!(poVB = VectorBorder::createFromVectorFile(vectorFile,mercType))) 
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
	
	__int64		tileID( int z, int x, int y)
	{
		unsigned int num = 0;
		for (int s=0;s<z;s++)
			num+=(maxx[s]-minx[s])*(maxy[s]-miny[s]);
		return (num + (maxx[z]-minx[z])*(y-miny[z]) + x-minx[z]);
	};

	BOOL		tileXYZ(__int64 n, int &z, int &x, int &y)
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

	TileType	getTileType()
	{
		return this->tileType;
	};

	MercatorProjType	getProjType()
	{
		return this->mercType;
	};

	BOOL		getTileBounds (int bounds[128])
	{
		for (int z =0;z<32;z++)
		{
			bounds[4*z]		= minx[z];
			bounds[4*z+1]	= miny[z];
			bounds[4*z+2]	= (maxx[z]>0) ? maxx[z]-1 : 0;
			bounds[4*z+3]	= (maxy[z]>0) ? maxy[z]-1 : 0;
		}
		return TRUE;
	};


	//TileContainer (wstring containerFileName, TileName *poTileName);

protected:
	
	BOOL 	init	(	int					tileBounds[128], 
						BOOL				useBuffer, 
						wstring				containerFileName, 
						TileType			tileType,
						MercatorProjType	mercType)
	{
		this->tileType			= tileType;
		this->mercType			= mercType;
		this->containerFileName = containerFileName;// + L".tiles";
		this->containerFileData = NULL;
		this->containerLength	= 0;
		this->MAX_TILES_IN_CONTAINER = 0;

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
		for (int z =0; z<32;z++)
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


	BOOL	writeContainerFromBuffer()
	{
		if (USE_BUFFER) 
		{
			writeTilesToContainerFileFromBuffer();
			writeContainerFileHeader();
		}
		return TRUE;
	}


	BOOL	addTileToContainerFile(int z, int x, int y, BYTE *pData, unsigned int size)
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
	
	BOOL	getTileFromContainerFile (int z, int x, int y, BYTE *&pData, unsigned int &size)
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

	void empty ()
	{
		delete[]sizes;
		sizes = NULL;
		delete[]offsets;
		offsets = NULL;
		maxTiles = 0;
		
		for (int i=0;i<32;i++)
			maxx[i]=(minx[i]=(maxy[i]=(miny[i]=0)));
		delete(poTileBuffer);
		poTileBuffer = NULL;
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
	unsigned __int64		*offsets;
	TileType				tileType;
	MercatorProjType		mercType;

	TileBuffer				*poTileBuffer;
	unsigned int			MAX_TILES_IN_CONTAINER;
	wstring					containerFileName;
	FILE*					containerFileData;
	unsigned __int64		containerLength;
};






class TileFolder : public TilePyramid
{
public:
	TileFolder (TileName *poTileName, BOOL useBuffer)
	{
		this->poTileName	= poTileName;
		this->USE_BUFFER	= useBuffer;
		poTileBuffer		= (useBuffer) ? new TileBuffer() :  NULL;
	};
	
	~TileFolder ()
	{
		close();
	};

	BOOL close()
	{
		delete(poTileBuffer);
		poTileBuffer = NULL;
		return TRUE;
	};

	BOOL	addTile(int z, int x, int y, BYTE *pData, unsigned int size)
	{
		if (USE_BUFFER)	poTileBuffer->addTile(z,x,y,pData,size);
		return writeTileToFile(z,x,y,pData,size);
	};



	BOOL		getTile(int z, int x, int y, BYTE *&pData, unsigned int &size)
	{
		if (USE_BUFFER)	return poTileBuffer->getTile(z,x,y,pData,size); 
		else			return readTileFromFile(z,x,y,pData,size);
	};

	BOOL		tileExists(int z, int x, int y)
	{
		return FileExists(poTileName->getFullTileName(z,x,y));
	};

	__int64		tileID( int z, int x, int y)
	{
		__int64 n = 0;
		n = ((((__int64)1)<<(2*z))-1)/3;
		n = n<<1;
		n += y*(((__int64)1)<<(z+1)) + x;
		return n;
	};

	BOOL	tileXYZ( __int64 id, int &z, int &x, int &y)
	{
		int i = 0;
		for (i=0;i<23;i++)
			if (( (((((__int64)1)<<(2*i+2))-1)/3)<<1) > id) break;
		if (i==23) return FALSE;
		else z = i;
		
		__int64 n = (((((__int64)1)<<(2*z))-1)/3)<<1;

		y = ((id-n)>>(z+1));
		x = (id-n) % (1<<(z+1));

		return TRUE;
	};


	int 		getTileList(list<__int64> &tileList, int minZoom, int maxZoom, wstring vectorFile = L"",  MercatorProjType mercType = WORLD_MERCATOR)
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

	BOOL		getTileBounds (int bounds[128])
	{
		list<__int64> oTileList;
		if (!getTileList(oTileList,-1,-1,L"")) return FALSE;
		for (int z=0; z<32;z++)
		{
			bounds[4*z+3]=(bounds[4*z+2]=0);
			bounds[4*z]=(bounds[4*z+1]=(1<<(z+1)));
		}
		for (list<__int64>::const_iterator iter = oTileList.begin(); iter!=oTileList.end();iter++)
		{
			int z,x,y;
			if (tileXYZ((*iter),z,x,y))
			{
				if (bounds[4*z]>x)		bounds[4*z] = x;
				if (bounds[4*z+1]>y)	bounds[4*z+1] = y;
				if (bounds[4*z+2]<=x)	bounds[4*z+2] = (x+1);
				if (bounds[4*z+3]<=y)	bounds[4*z+3] = (y+1);
			}
		}

		return TRUE;
	};

	
protected:
	BOOL	writeTileToFile (int z, int x, int y, BYTE *pData, unsigned int size)
	{
		if (poTileName==NULL) return FALSE;
		poTileName->createFolder(z,x,y);
		return SaveDataToFile(poTileName->getFullTileName(z,x,y), pData,size);
	};

	
	BOOL	readTileFromFile (int z,int x, int y, BYTE *&pData, unsigned int &size)
	{
		return readDataFromFile (poTileName->getFullTileName(z,x,y),pData,size); 
	};


protected:
	TileName	*poTileName;
	BOOL		USE_BUFFER;
	TileBuffer	*poTileBuffer;
};