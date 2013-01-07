#include "StdAfx.h"
#include "TileName.h"
namespace GMT
{


StandardTileName::StandardTileName (wstring baseFolder, wstring strTemplate)
{
	if (!validateTemplate(strTemplate)) return;
	if (!FileExists(baseFolder)) return;

	wstring strExt = strTemplate.substr(strTemplate.rfind(L".")+1,strTemplate.length()-strTemplate.rfind(L".")-1);
	this->tileType =	(MakeLower(strExt)==L"jpg") ? JPEG_TILE :
						(MakeLower(strExt)==L"png") ? PNG_TILE :
						(MakeLower(strExt)==L"tif") ? TIFF_TILE : JPEG_TILE;
	this->baseFolder	= baseFolder;
	zxyPos[0] = (zxyPos[1] = (zxyPos[2] = 0));

	if (strTemplate[0] == L'/' || strTemplate[0] == L'\\') 	strTemplate = strTemplate.substr(1,strTemplate.length()-1);
	ReplaceAll(strTemplate,L"/",L"\\");
	this->strTemplate = strTemplate;
		
	ReplaceAll(strTemplate,L"\\",L"\\\\");
	int n = 0;
	int num = 2;
	//int k;
	while (strTemplate.find(L'{',n)!=std::wstring::npos)
	{
		wstring str = strTemplate.substr(strTemplate.find(L'{',n),strTemplate.find(L'}',n)-strTemplate.find(L'{',n)+1);
		if (str == L"{z}")
			zxyPos[0] = (zxyPos[0] == 0) ? num : zxyPos[0];
		else if (str == L"{x}")
			zxyPos[1] = (zxyPos[1] == 0) ? num : zxyPos[1];
		else if (str == L"{y}")
			zxyPos[2] = (zxyPos[2] == 0) ? num : zxyPos[2];
		num++;
		n = strTemplate.find(L'}',n) + 1;
	}

	ReplaceAll(strTemplate,L"{z}",L"(\\d+)");
	ReplaceAll(strTemplate,L"{x}",L"(\\d+)");
	ReplaceAll(strTemplate,L"{y}",L"(\\d+)");
	rxTemplate = (L"(.*)" + strTemplate) + L"(.*)";
}

BOOL	StandardTileName::validateTemplate	(wstring strTemplate)
{
	if (strTemplate.find(L"{z}",0)==wstring::npos)
	{
		wcout<<"Error: bad tile name template: missing {z}"<<endl;
		return FALSE;
	}
	if (strTemplate.find(L"{x}",0)==wstring::npos)
	{
		wcout<<"Error: bad tile name template: missing {x}"<<endl;
		return FALSE;
	}
	if (strTemplate.find(L"{y}",0)==wstring::npos) 
	{
		wcout<<"Error: bad tile name template: missing {y}"<<endl;
		return FALSE;
	}

	if (strTemplate.find(L".",0)==wstring::npos) 
	{
		wcout<<"Error: bad tile name template: missing extension"<<endl;
		return FALSE;
	}
		
	wstring strExt = strTemplate.substr(strTemplate.rfind(L".")+1,strTemplate.length()-strTemplate.rfind(L".")-1);
	if ( (MakeLower(strExt)!=L"jpg")&& (MakeLower(strExt)==L"png") && (MakeLower(strExt)==L"tif") )
	{
		wcout<<"Error: bad tile name template: missing extension, must be: .jpg, .png, .tif"<<endl;
		return FALSE;
	}
	return TRUE;
}

wstring	StandardTileName::getTileName (int nZoom, int nX, int nY)
{
	wstring tileName = strTemplate;
	ReplaceAll(tileName,L"{z}",ConvertIntToWString(nZoom));
	ReplaceAll(tileName,L"{x}",ConvertIntToWString(nX));
	ReplaceAll(tileName,L"{y}",ConvertIntToWString(nY));
	return tileName;
}

BOOL StandardTileName::extractXYZFromTileName (wstring strTileName, int &z, int &x, int &y)
{
	if (!regex_match(strTileName,rxTemplate)) return FALSE;
	match_results<wstring::const_iterator> mr;
	regex_search(strTileName, mr, rxTemplate);
	if ((mr.size()<=zxyPos[0])||(mr.size()<=zxyPos[1])||(mr.size()<=zxyPos[2])) return FALSE;
	z = (int)_wtof(mr[zxyPos[0]].str().c_str());
	x = (int)_wtof(mr[zxyPos[1]].str().c_str());
	y = (int)_wtof(mr[zxyPos[2]].str().c_str());
		
	return TRUE;
}


BOOL StandardTileName::createFolder (int nZoom, int nX, int nY)
{
	wstring strTileName = getTileName(nZoom,nX,nY);
	int n = 0;
	while (strTileName.find(L"\\",n)!=std::wstring::npos)
	{
		if (!FileExists(GetAbsolutePath(baseFolder,strTileName.substr(0,strTileName.find(L"\\",n)))))
			if (!CreateDirectory(GetAbsolutePath(baseFolder,strTileName.substr(0,strTileName.find(L"\\",n))).c_str(),NULL)) return FALSE;	
		n = (strTileName.find(L"\\",n)) + 1;
	}
	return TRUE;
}


KosmosnimkiTileName::KosmosnimkiTileName (wstring strTilesFolder, TileType tileType)
{
	this->baseFolder	= strTilesFolder;
	this->tileType		= tileType;
}


	
wstring	KosmosnimkiTileName::getTileName (int nZoom, int nX, int nY)
{
	if (nZoom>0)
	{
		nX = nX-(1<<(nZoom-1));
		nY = (1<<(nZoom-1))-nY-1;
	}
	swprintf(buf,L"%d\\%d\\%d_%d_%d.%s",nZoom,nX,nZoom,nX,nY,this->tileExtension(this->tileType).c_str());
	return buf;
}

BOOL KosmosnimkiTileName::extractXYZFromTileName (wstring strTileName, int &z, int &x, int &y)
{
	strTileName = RemovePath(strTileName);
	strTileName = RemoveExtension(strTileName);
	int k;

	wregex pattern(L"[0-9]{1,2}_-{0,1}[0-9]{1,7}_-{0,1}[0-9]{1,7}");
	//wregex pattern(L"(\d+)_-?(\d+)_-{0,1}[0-9]{1,7}");

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


BOOL KosmosnimkiTileName::createFolder (int nZoom, int nX, int nY)
{
	if (nZoom>0)
	{
		nX = nX-(1<<(nZoom-1));
		nY = (1<<(nZoom-1))-nY-1;
	}

	swprintf(buf,L"%d",nZoom);
	wstring str = GetAbsolutePath(this->baseFolder, buf);
	if (!FileExists(str))
	{
		if (!CreateDirectory(str.c_str(),NULL)) return FALSE;	
	}

	swprintf(buf,L"%d",nX);
	str = GetAbsolutePath(str,buf);
	if (!FileExists(str))
	{
		if (!CreateDirectory(str.c_str(),NULL)) return FALSE;	
	}
		
	return TRUE;
}


}