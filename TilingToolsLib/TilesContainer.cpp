#include "TilesContainer.h"


TilesContainer::TilesContainer (wstring containerFileName, TileName *poTileName)
{
	if (!(this->containerFileData = _wfopen(containerFileName.c_str(),L"rb")))
		return;
	sizes		= NULL;
	tilesData	= NULL;
	offsets		= NULL;

	BYTE	head[12];
	fread(head,1,12,containerFileData);
	this->MAX_TILES_IN_CONTAINER = 0;
	this->mercType = (head[9] == 0) ? WORLD_MERCATOR : WEB_MERCATOR;
	this->tileType = (head[11] == 0) ? JPEG_TILE : (head[11]==1) ? PNG_TILE : TIFF_TILE;
	this->USE_CONTAINER = TRUE;
	this->USE_BUFFER = FALSE;
	this->poTileName = poTileName;


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
	tilesData	= new BYTE*[maxTiles];
	offsets		= new unsigned __int64[maxTiles];

	for (int i=0; i<maxTiles;i++)
	{
		offsets[i]	= *((unsigned __int64*)(&offset_size[i*13]));
		sizes[i]	= *((unsigned int*)(&offset_size[i*13+8]));
	}

	delete[]offset_size;
	
	//fclose(containerFileData);

}

TilesContainer::TilesContainer	(OGREnvelope envelope, 
								int			maxZoom, 
								BOOL		useBuffer, 
								BOOL		useContainer,
								TileName	*poTileName, 
								wstring		containerFileName, 
								TileType	tileType,
								MercatorProjType	mercType)
{
	this->MAX_TILES_IN_CONTAINER = 0;
	this->poTileName	= poTileName;
	this->USE_BUFFER	= useBuffer;
	this->USE_CONTAINER = useContainer;
	this->maxZoom		= maxZoom;
	this->tileType		= tileType;
	this->mercType		= mercType;


	for (int i=0;i<32;i++)
	{
		maxx[i]=(minx[i]=(maxy[i]=(miny[i]=0)));
		if (i<=maxZoom)
		{
			MercatorTileGrid::calcTileRange(envelope,i,minx[i],miny[i],maxx[i],maxy[i]);
			maxx[i]++;maxy[i]++;
		}
	}

	maxTiles = tileNumber(maxx[maxZoom]-1,maxy[maxZoom]-1,maxZoom) + 1;

	containerFileData = NULL;
	sizes		= NULL;
	tilesData	= NULL;
	offsets		= NULL;
	containerFileData = NULL;


	sizes		= new unsigned int[maxTiles];
	tilesData	= new BYTE*[maxTiles];
	offsets		= new unsigned __int64[maxTiles];
	containerLength	= 0;

	for (unsigned int i=0;i<maxTiles;i++)
	{
		sizes[i]		= 0;
		tilesData[i]	= NULL;
		offsets[i]		= 0;
	}

	if (useContainer)
	{
		this->containerFileName = containerFileName;// + L".tiles";
		this->containerFileData = NULL;
	}

}

///*
void TilesContainer::tileXYZ(unsigned int n, int &x, int &y, int &z)
{
	int s = 0;
	for (z=0;z<32;z++)
	{
		if ((s+(maxx[z]-minx[z])*(maxy[z]-miny[z]))>n) break;
		s+=(maxx[z]-minx[z])*(maxy[z]-miny[z]);
	}
	y = miny[z] + ((n-s)/(maxx[z]-minx[z]));
	x = minx[z] + ((n-s)%(maxx[z]-minx[z]));
}

///*
int 	TilesContainer::getTileList(list<__int64> &tilesList, int minZoom, int maxZoom, wstring vectorFile)
{
	return (this->USE_CONTAINER) ?	getTileListFromContainerFile(tilesList,minZoom,maxZoom,vectorFile) :
									getTileListFromDisk(tilesList,minZoom,maxZoom,vectorFile);
}
//*/

///*
int		TilesContainer::getTileListFromDisk(list<__int64> &tilesList, int minZoom, int maxZoom, wstring vectorFile)
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
		if (vectorFile!=L"") 
			if (vb.Intersects(MercatorTileGrid::calcEnvelopeByTile(z,x,y))) continue;
		tilesList.push_back(MercatorTileGrid::TileID(z,x,y));
	}
	return tilesList.size();
}


int		TilesContainer::getTileListFromContainerFile(list<__int64> &tilesList, int minZoom, int maxZoom, wstring vectorFile)
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
		if (sizes[i]>0) tileXYZ(i,x,y,z);
		if (vectorFile!=L"") 
			if (vb.Intersects(MercatorTileGrid::calcEnvelopeByTile(z,x,y))) continue;
		tilesList.push_back(MercatorTileGrid::TileID(z,x,y));
	}

	return tilesList.size();
}
//*/

/*
BOOL TilesContainer::unpackAllTiles()
{
	BYTE	*pData = NULL;
	for (int i=0; i<maxTiles;i++)
	{
		if (sizes[i]!=0)
		{
			fseek(containerFileData,offsets[i],0);
			pData = new BYTE[sizes[i]];
			fread(pData,1,sizes[i],containerFileData);
			int x, y,z;
			tileXYZ(i,x,y,z);
			poTileName->createFolder(z,x,y);
			SaveDataToFile(poTileName->getFullTileName(z,x,y),pData,sizes[i]);		
			delete[]pData;
		}
	}
	return TRUE;
}
*/

BOOL TilesContainer::empty()
{
	delete[] sizes;
	sizes = NULL;
	delete[] offsets;
	offsets = NULL;

	if (tilesData)
	{
		for (int i=0; i<maxTiles; i++)
			delete[]tilesData[i];
		delete[]tilesData;
		tilesData = NULL;
	}
	if (containerFileData) fclose(containerFileData);
	containerFileData=NULL;

	for (int i=0;i<32;i++)
		maxx[i]=(minx[i]=(maxy[i]=(miny[i]=0)));
	return TRUE;
}


TilesContainer::~TilesContainer(void)
{
	empty();
}


unsigned int TilesContainer::tileNumber (int x, int y, int z)
{
	unsigned int num = 0;
	for (int s=0;s<z;s++)
		num+=(maxx[s]-minx[s])*(maxy[s]-miny[s]);
	return (num + (maxx[z]-minx[z])*(y-miny[z]) + x-minx[z]);
}


BOOL	TilesContainer::addTile(int x, int y, int z, BYTE *pData, unsigned int size)
{
	if (USE_BUFFER)	
	{
		BOOL res = addTileToBuffer(x,y,z,pData,size);
		return (USE_CONTAINER) ? res : writeTileToFile(x,y,z,pData,size);
	}
	else
		return (USE_CONTAINER) ? addTileToContainerFile(x,y,z,pData,size) : writeTileToFile(x,y,z,pData,size);

	return TRUE;
}

BOOL		TilesContainer::getTile(int x, int y, int z, BYTE *&pData, unsigned int &size)
{
	if (USE_BUFFER)						return getTileFromBuffer(x,y,z,pData,size); 
	if (!USE_CONTAINER)					return readTileFromFile(x,y,z,pData,size);
	else								return getTileFromContainerFile(x,y,z,pData,size);

	return TRUE;
}

BOOL		TilesContainer::tileExists(int x, int y, int z)
{
	if (USE_CONTAINER)
	{
		return (sizes[tileNumber(x,y,z)]>0) ? TRUE : FALSE;
	}
	else return FileExists(poTileName->getFullTileName(z,x,y));
	
}


BOOL	TilesContainer::getTileFromBuffer(int x, int y, int z, BYTE *&pData, unsigned int &size)
{
	
	unsigned int n	= tileNumber(x,y,z);
	pData			= new BYTE[sizes[n]];
	memcpy(pData,tilesData[n],sizes[n]);
	size			= sizes[n];
	return TRUE;
}

/*
BOOL	TilesContainer::saveTile (int x, int y, int z, BYTE *pData, unsigned int size)
{
	wstring fileName = this->poTileName->getTileName(z,x,y);

	FILE *fp;
	if (!(fp = _wfopen(fileName.c_str(),L"wb"))) return FALSE;
	fwrite(pData,sizeof(BYTE),size,fp);
	fclose(fp);
	return TRUE;
}
*/

BOOL	TilesContainer::addTileToBuffer(int x, int y, int z, BYTE *pData, unsigned int size)
{
	//tilesCash->
	unsigned int n	= tileNumber(x,y,z);
	if (n>= maxTiles) return FALSE;

	if (sizes[n]>0) delete[]tilesData[n];
	
	sizes[n]		= size;
	tilesData[n]	= new BYTE[size];
	memcpy(tilesData[n],pData,size);
	/*
	if (USE_CONTAINER)
	{
		if (containerLength == 0) containerLength = headerSize();
		offsets[n] = containerLength;
		containerLength +=size;
	}
	*/
	return TRUE;
}


BOOL	TilesContainer::getTileFromContainerFile (int x, int y, int z, BYTE *&pData, unsigned int &size)
{
	unsigned int n	= tileNumber(x,y,z);
	if (n>= maxTiles) return FALSE;
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

}


BOOL	TilesContainer::writeTileToFile (int x, int y, int z, BYTE *pData, unsigned int size)
{
	if (poTileName==NULL) return FALSE;
	poTileName->createFolder(z,x,y);
	return SaveDataToFile(poTileName->getFullTileName(z,x,y), pData,size);
}

BOOL	TilesContainer::readTileFromFile (int x,int y, int z, BYTE *&pData, unsigned int &size)
{
	return readDataFromFile (poTileName->getFullTileName(z,x,y),pData,size); 
}

unsigned int TilesContainer::headerSize()
{
	//int l = (4+4+4+512+maxTiles*(4+8+1));
	//return (4+4+4+512+this->MAX_TILES_IN_CONTAINER*(4+8+1));
	return (4+4+4+512+maxTiles*(4+8+1));

}


/*
BOOL	TilesContainer::addTileToContainer(int x, int y, int z, BYTE *pData, unsigned int size)
{
	//tilesCash->
	
	if (!containerFileData)
	{
		if (!(containerFileData = _wfopen(containerFileName.c_str(),L"wb"))) return FALSE;
		containerLength = headerSize();
	}

	unsigned int n = tileNumber(x,y,z);
	//offsets[n] = containerLength;
	fseek(containerFileData,containerLength,SEEK_SET);
	fwrite(pData,sizeof(BYTE),size,containerFileData);
	containerLength +=size;
	return TRUE;
}
*/

BOOL	TilesContainer::closeContainerFile()
{
	if (USE_BUFFER && USE_CONTAINER) 
	{
		writeTilesToContainerFileFromBuffer();
		writeContainerFileHeader();
	}
	else if ((!USE_BUFFER) && USE_CONTAINER) 
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
}


BOOL	TilesContainer::writeContainerFromBuffer()
{
	if (USE_BUFFER && USE_CONTAINER) 
	{
		writeTilesToContainerFileFromBuffer();
		writeContainerFileHeader();
	}
	return TRUE;
}


BOOL	TilesContainer::writeHeaderToByteArray(BYTE*	&pData)
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
}


BOOL	TilesContainer::writeContainerFileHeader()
{
	if (!containerFileData) return FALSE;

	//fclose(containerFileData);
//	return TRUE;

	fseek(containerFileData,0,SEEK_SET);
	
	BYTE	*containerHead;
	writeHeaderToByteArray(containerHead);
	fwrite(containerHead,1,headerSize(),containerFileData);
	fclose(containerFileData);
	containerFileData = NULL;
	delete[]containerHead;
	return TRUE;
}


BOOL	TilesContainer::addTileToContainerFile(int x, int y, int z, BYTE *pData, unsigned int size)
{
	unsigned int n	= tileNumber(x,y,z);
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
}




BOOL	TilesContainer::writeTilesToContainerFileFromBuffer()
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
				memcpy(&block[len],tilesData[i],sizes[i]);
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
}

