#pragma once

#ifndef IMAGE_TILLING_TILENAME_H
#define IMAGE_TILLING_TILENAME_H
#include "stdafx.h"
#include "RasterBuffer.h"
#include "StringFuncs.h"
#include "FileSystemFuncs.h"
#include "GeometryFuncs.h"

class MercatorTileGrid
{
public:
	static const int TILE_SIZE = 256;

	static double	getULX()
	{
		return -20037508.342789243076588408880691;
	}

	static double	getULY()
	{
		return 20037508.342789243076588408880691;
	}
	
	static int calcZoomByResolution (double dRes)
	{
		double a = calcResolutionByZoom(0);
		for (int i=0;i<22;i++)
		{
			if (fabs(a-dRes)<0.0001) return i;
			a/=2;
		}
		return -1;
	}

	static double calcResolutionByZoom (int nZoom)
	{
		return (nZoom<0) ? 0 : getULY()/(1<<(nZoom+7));
	}

	static int calcZoomByEnvelope (OGREnvelope envelope)
	{
		return calcZoomByResolution ((envelope.MaxX-envelope.MinX)/TILE_SIZE);
	}

	static OGREnvelope calcEnvelopeByTile (int nZoom, int nX, int nY)
	{
		double size = calcResolutionByZoom(nZoom)*TILE_SIZE;
		OGREnvelope oEnvelope;
			
		oEnvelope.MinX = getULX() + nX*size;
		oEnvelope.MaxY = getULY() - nY*size;
		oEnvelope.MaxX = oEnvelope.MinX + size;
		oEnvelope.MinY = oEnvelope.MaxY - size;
		
		return oEnvelope;
	}

	static __int64 TileID(int z, int x, int y)
	{
		__int64 tileID = 0;
		for (int i=0;i<z;i++)
			tileID+=(((__int64)1)<<(2*i));
		return (tileID + ((__int64)y)*(1<<z) + x);
	}

	static void TileXYZ(__int64 tileID, int &z, int &x, int &y)
	{
		__int64 n = 0;
		for (int i=0;i<z;i++)
		{
			if (n+((__int64)1)<<(2*i)>tileID)
			{
				z = i;
				break;
			}
			else n+=((__int64)1)<<(2*i)>tileID;
		}
		y = (tileID-n)/(((__int64)1)<<(2*z));
		y = (tileID-n)%(((__int64)1)<<(2*z));
	}


	static OGREnvelope calcEnvelopeByTileRange (int nZoom, int minx, int miny, int maxx, int maxy)
	{
		double size = calcResolutionByZoom(nZoom)*TILE_SIZE;
		OGREnvelope oEnvelope;
			
		oEnvelope.MinX = getULX() + minx*size;
		oEnvelope.MaxY = getULY() - miny*size;
		oEnvelope.MaxX = getULX() + (maxx+1)*size;
		oEnvelope.MinY = getULY() - (maxy+1)*size;
		
		return oEnvelope;
	}


	static OGREnvelope calcPixelEnvelope(OGREnvelope oImageEnvelope, int z)
	{
		OGREnvelope oPixelEnvelope;
	
		oPixelEnvelope.MinX = floor(((oImageEnvelope.MinX-getULX()-E)/calcResolutionByZoom(z))+0.5);
		oPixelEnvelope.MinY = floor(((getULY()-oImageEnvelope.MaxY-E)/calcResolutionByZoom(z))+0.5);
		oPixelEnvelope.MaxX = oPixelEnvelope.MinX + floor((oImageEnvelope.MaxX - oImageEnvelope.MinX)/calcResolutionByZoom(z)+0.5);
		oPixelEnvelope.MaxY = oPixelEnvelope.MinY + floor((oImageEnvelope.MaxY - oImageEnvelope.MinY)/calcResolutionByZoom(z)+0.5);
		
		return oPixelEnvelope;
	}

	static void calcTileByPoint (double x, double y, int z, int &nX, int &nY)
	{
		//double E = 1e-4;
		nX = (int)floor((x-getULX())/(TILE_SIZE*calcResolutionByZoom(z)));
		nY = (int)floor((getULY()-y)/(TILE_SIZE*calcResolutionByZoom(z)));
	}

	static void calcTileRange (OGREnvelope oEnvelope, int z, int &minX, int &minY, int &maxX, int &maxY)
	{
		calcTileByPoint(oEnvelope.MinX,oEnvelope.MaxY,z,minX,minY);
		calcTileByPoint(oEnvelope.MaxX,oEnvelope.MinY,z,maxX,maxY);
	}
		
};
//const double MercatorTileGrid::E = 1e-4;

typedef enum { 
	JPEG_TILE		=0,
	PNG_TILE		=1,
	TIFF_TILE		=4
} TileType;


class TileName
{
public:

	static wstring tileExtension(TileType tileType)
	{
		switch (tileType)
		{
			case JPEG_TILE:
				return L"jpg";
			case PNG_TILE:
				return L"png";
			case TIFF_TILE:
				return L"tif";
		}
		return L"";
	}

////////////////////////////////////////////////////////////////////////////////////////////
///��� ������ ����� ����������� � ����������� ������ (��. KosmosnimkiTileName)
////////////////////////////////////////////////////////////////////////////////////////////
	virtual	wstring		getTileName				(int nZoom, int nX, int nY) = 0;
	virtual	BOOL		extractXYZFromTileName	(wstring strTileName, int &z, int &x, int &y) = 0;
	virtual	BOOL		createFolder			(int nZoom, int nX, int nY) = 0;
///////////////////////////////////////////////////////////////////////////////////////////

	wstring getFullTileName (int nZoom, int nX, int nY)
	{
		return tilesFolder + L"\\" + getTileName(nZoom,nX,nY);
	}

	wstring getTilesFodler ()
	{
		return tilesFolder;
	}
	
	
	

public:
	wstring		tilesFolder;
	TileType	tileType;
	//wstring	tileExtension;

protected:
	_TCHAR buf[1000];
};


class StandardTileName : public TileName
{
public:
	StandardTileName (wstring tilesFolder, TileType tileType = JPEG_TILE)
	{
		this->tilesFolder	= tilesFolder;
		this->tileType		= tileType;
		//this->tileExtension = (tileType==JPEG_TILE) ? L"jpg" : (tileType==PNG_TILE) ? L"png" : L"tif";
	}


	wstring	getTileName (int nZoom, int nX, int nY)
	{
		swprintf(buf,L"%d\\%d\\%d_%d_%d.%s",nZoom,nX,nZoom,nX,nY,tileExtension(this->tileType).c_str());
		return buf;
	}

	BOOL extractXYZFromTileName (wstring strTileName, int &z, int &x, int &y)
	{
		strTileName = RemovePath(strTileName);
		strTileName = RemoveExtension(strTileName);
		int k;
	
		wregex pattern(L"[0-9]{1,2}_[0-9]{1,7}_[0-9]{1,7}");
		if (!regex_match(strTileName,pattern)) return FALSE;
		
		z = (int)_wtof(strTileName.substr(0,strTileName.find('_')).c_str());
		strTileName = strTileName.substr(strTileName.find('_')+1);
		
		x = (int)_wtof(strTileName.substr(0,strTileName.find('_')).c_str());
		y = (int)_wtof(strTileName.substr(strTileName.find('_')+1).c_str());

		return TRUE;
	}


	BOOL createFolder (int nZoom, int nX, int nY)
	{
		
		swprintf(buf,L"%d",nZoom);
		wstring str = CombinePathAndFile(this->tilesFolder, buf);
		if (!FileExists(str))
		{
			if (!CreateDirectory(str.c_str(),NULL)) return FALSE;	
		}

		swprintf(buf,L"%d",nX);
		str = CombinePathAndFile(str,buf);
		if (!FileExists(str))
		{
			if (!CreateDirectory(str.c_str(),NULL)) return FALSE;	
		}
		
		return TRUE;
	}

	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////


};


class KosmosnimkiTileName : public TileName
{
public:

	KosmosnimkiTileName (wstring strTilesFolder, TileType tileType = JPEG_TILE)
	{
		this->tilesFolder	= strTilesFolder;
		this->tileType		= tileType;
	}


	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////
	
	wstring	getTileName (int nZoom, int nX, int nY)
	{
		if (nZoom>0)
		{
			nX = nX-(1<<(nZoom-1));
			nY = (1<<(nZoom-1))-nY-1;
		}
		swprintf(buf,L"%d\\%d\\%d_%d_%d.%s",nZoom,nX,nZoom,nX,nY,this->tileExtension(this->tileType).c_str());
		return buf;
	}

	BOOL extractXYZFromTileName (wstring strTileName, int &z, int &x, int &y)
	{
		strTileName = RemovePath(strTileName);
		strTileName = RemoveExtension(strTileName);
		int k;

		wregex pattern(L"[1-9]{1,2}_-{0,1}[0-9]{1,7}_-{0,1}[0-9]{1,7}");
		if (!regex_match(strTileName,pattern)) return FALSE;

		z = (int)_wtof(strTileName.substr(0,strTileName.find('_')).c_str());
		strTileName = strTileName.substr(strTileName.find('_')+1);
		
		x = (int)_wtof(strTileName.substr(0,strTileName.find('_')).c_str());
		y = (int)_wtof(strTileName.substr(strTileName.find('_')+1).c_str());

		if (z>0)
		{
			x+=(1<<(z-1));
			y=(1<<(z-1))-y-1;
		}

		return TRUE;
	}


	BOOL createFolder (int nZoom, int nX, int nY)
	{
		if (nZoom>0)
		{
			nX = nX-(1<<(nZoom-1));
			nY = (1<<(nZoom-1))-nY-1;
		}

		swprintf(buf,L"%d",nZoom);
		wstring str = CombinePathAndFile(this->tilesFolder, buf);
		if (!FileExists(str))
		{
			if (!CreateDirectory(str.c_str(),NULL)) return FALSE;	
		}

		swprintf(buf,L"%d",nX);
		str = CombinePathAndFile(str,buf);
		if (!FileExists(str))
		{
			if (!CreateDirectory(str.c_str(),NULL)) return FALSE;	
		}
		
		return TRUE;
	}

	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////
};



#endif
