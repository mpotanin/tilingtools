#pragma once
#include "stdafx.h"
#include <windows.h>


namespace gmx
{

string	RemovePath	(string strFile);
string	RemoveExtension(string strFile);
string	GetPath(string strFileName);
string	RemoveEndingSlash(string	strFolderName);
bool		FileExists	(string strFileName);
bool		IsDirectory	(string strPath);

bool		FindFilesByPattern (list<string> &listFiles, string strSearchPattern);
bool		FindFilesByExtensionRecursive (list<string> &listFiles, string strFolder, string	strExtension);

bool		WriteToTextFile(string strFileName, string strText);
bool		WriteWLDFile	(string strFileName, double dblULX, double dblULY, double dblRes);
bool		SaveDataToFile	(string strFileName, void *pabData, int nSize);
bool		ReadBinaryFile(string strFileName, void *&pabData, int &nSize);
string  ReadTextFile(string strFileName);
string	GetAbsolutePath (string strBasePath, string strRelativePath);
string	GetExtension (string strPath);
bool		GMXCreateDirectory(string strPath);
bool		GMXDeleteFile(string	strPath);
bool		RenameFile(string strOldPath, string strNewPath);
FILE*		OpenFile(string	strFileName, string strMode);

}