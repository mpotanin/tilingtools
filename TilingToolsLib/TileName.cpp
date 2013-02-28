#include "StdAfx.h"
#include "TileName.h"
namespace GMX
{


StandardTileName::StandardTileName (string baseFolder, string strTemplate)
{
	if (!validateTemplate(strTemplate)) return;
	if (!FileExists(baseFolder)) return;

	string strExt = strTemplate.substr(strTemplate.rfind(".")+1,strTemplate.length()-strTemplate.rfind(".")-1);
	this->tileType =	(MakeLower(strExt)=="jpg") ? JPEG_TILE :
						(MakeLower(strExt)=="png") ? PNG_TILE :
						(MakeLower(strExt)=="tif") ? TIFF_TILE : JPEG_TILE;
	this->baseFolder	= baseFolder;
	zxyPos[0] = (zxyPos[1] = (zxyPos[2] = 0));

	if (strTemplate[0] == L'/' || strTemplate[0] == L'\\') 	strTemplate = strTemplate.substr(1,strTemplate.length()-1);
	ReplaceAll(strTemplate,"\\","/");
	this->strTemplate = strTemplate;
		
	//ReplaceAll(strTemplate,"\\","\\\\");
	int n = 0;
	int num = 2;
	//int k;
	while (strTemplate.find(L'{',n)!=std::string::npos)
	{
		string str = strTemplate.substr(strTemplate.find(L'{',n),strTemplate.find(L'}',n)-strTemplate.find(L'{',n)+1);
		if (str == "{z}")
			zxyPos[0] = (zxyPos[0] == 0) ? num : zxyPos[0];
		else if (str == "{x}")
			zxyPos[1] = (zxyPos[1] == 0) ? num : zxyPos[1];
		else if (str == "{y}")
			zxyPos[2] = (zxyPos[2] == 0) ? num : zxyPos[2];
		num++;
		n = strTemplate.find(L'}',n) + 1;
	}

	ReplaceAll(strTemplate,"{z}","(\\d+)");
	ReplaceAll(strTemplate,"{x}","(\\d+)");
	ReplaceAll(strTemplate,"{y}","(\\d+)");
	rxTemplate = ("(.*)" + strTemplate) + "(.*)";
}

BOOL	StandardTileName::validateTemplate	(string strTemplate)
{
	if (strTemplate.find("{z}",0)==string::npos)
	{
		cout<<"Error: bad tile name template: missing {z}"<<endl;
		return FALSE;
	}
	if (strTemplate.find("{x}",0)==string::npos)
	{
		cout<<"Error: bad tile name template: missing {x}"<<endl;
		return FALSE;
	}
	if (strTemplate.find("{y}",0)==string::npos) 
	{
		cout<<"Error: bad tile name template: missing {y}"<<endl;
		return FALSE;
	}

	if (strTemplate.find(".",0)==string::npos) 
	{
		cout<<"Error: bad tile name template: missing extension"<<endl;
		return FALSE;
	}
		
	string strExt = strTemplate.substr(strTemplate.rfind(".")+1,strTemplate.length()-strTemplate.rfind(".")-1);
	if ( (MakeLower(strExt)!="jpg")&& (MakeLower(strExt)=="png") && (MakeLower(strExt)=="tif") )
	{
		cout<<"Error: bad tile name template: missing extension, must be: .jpg, .png, .tif"<<endl;
		return FALSE;
	}
	return TRUE;
}

string	StandardTileName::getTileName (int nZoom, int nX, int nY)
{
	string tileName = strTemplate;
	ReplaceAll(tileName,"{z}",ConvertIntToString(nZoom));
	ReplaceAll(tileName,"{x}",ConvertIntToString(nX));
	ReplaceAll(tileName,"{y}",ConvertIntToString(nY));
	return tileName;
}

BOOL StandardTileName::extractXYZFromTileName (string strTileName, int &z, int &x, int &y)
{
	if (!regex_match(strTileName,rxTemplate)) return FALSE;
	match_results<string::const_iterator> mr;
	regex_search(strTileName, mr, rxTemplate);
	if ((mr.size()<=zxyPos[0])||(mr.size()<=zxyPos[1])||(mr.size()<=zxyPos[2])) return FALSE;
	z = (int)atof(mr[zxyPos[0]].str().c_str());
	x = (int)atof(mr[zxyPos[1]].str().c_str());
	y = (int)atof(mr[zxyPos[2]].str().c_str());
		
	return TRUE;
}


BOOL StandardTileName::createFolder (int nZoom, int nX, int nY)
{
	string strTileName = getTileName(nZoom,nX,nY);
	int n = 0;
	while (strTileName.find("/",n)!=std::string::npos)
	{
		if (!FileExists(GetAbsolutePath(baseFolder,strTileName.substr(0,strTileName.find("/",n)))))
			if (!CreateDirectory(GetAbsolutePath(baseFolder,strTileName.substr(0,strTileName.find("/",n))).c_str())) return FALSE;	
		n = (strTileName.find("/",n)) + 1;
	}
	return TRUE;
}


KosmosnimkiTileName::KosmosnimkiTileName (string strTilesFolder, TileType tileType)
{
	this->baseFolder	= strTilesFolder;
	this->tileType		= tileType;
}


	
string	KosmosnimkiTileName::getTileName (int nZoom, int nX, int nY)
{
	if (nZoom>0)
	{
		nX = nX-(1<<(nZoom-1));
		nY = (1<<(nZoom-1))-nY-1;
	}
	sprintf(buf,"%d\\%d\\%d_%d_%d.%s",nZoom,nX,nZoom,nX,nY,this->tileExtension(this->tileType).c_str());
	return buf;
}

BOOL KosmosnimkiTileName::extractXYZFromTileName (string strTileName, int &z, int &x, int &y)
{
	strTileName = RemovePath(strTileName);
	strTileName = RemoveExtension(strTileName);
	int k;

	regex pattern("[0-9]{1,2}_-{0,1}[0-9]{1,7}_-{0,1}[0-9]{1,7}");
	//wregex pattern("(\d+)_-?(\d+)_-{0,1}[0-9]{1,7}");

	if (!regex_match(strTileName,pattern)) return FALSE;

	z = (int)atof(strTileName.substr(0,strTileName.find('_')).c_str());
	strTileName = strTileName.substr(strTileName.find('_')+1);
		
	x = (int)atof(strTileName.substr(0,strTileName.find('_')).c_str());
	y = (int)atof(strTileName.substr(strTileName.find('_')+1).c_str());

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

	sprintf(buf,"%d",nZoom);
	string str = GetAbsolutePath(this->baseFolder, buf);
	if (!FileExists(str))
	{
		if (!CreateDirectory(str.c_str())) return FALSE;	
	}

	sprintf(buf,"%d",nX);
	str = GetAbsolutePath(str,buf);
	if (!FileExists(str))
	{
		if (!CreateDirectory(str.c_str())) return FALSE;	
	}
		
	return TRUE;
}


}