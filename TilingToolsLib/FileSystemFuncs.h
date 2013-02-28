#pragma once
#include "stdafx.h"
#include <windows.h>


//string		_GetAbsolutePath(string path, string fileName);
namespace GMX
{

string		RemovePath	(const string& fileName);
string		RemoveExtension(string& fileName);
string		GetPath(string fileName);
string		RemoveEndingSlash(string	folderName);
BOOL		FileExists	(string fileName);
BOOL		IsDirectory	(string path);
BOOL		FindFilesInFolderByPattern (list<string> &fileList, string searchPattern);
BOOL		FindFilesInFolderByExtension (list<string> &fileList, string strFolder, string	strExtension, BOOL bRecursive);
BOOL		WriteToTextFile(string fileName, string strText);
BOOL		writeWLDFile	(string fileName, double dULx, double dULy, double dRes);
BOOL		SaveDataToFile	(string fileName, void *pData, int size);
BOOL		ReadDataFromFile(string fileName, BYTE *&pData, unsigned int &size);
string		GetAbsolutePath (string basePath, string relativePath);
string		GetExtension (string path);
BOOL		CreateDirectory(string path);
BOOL		DeleteFile(string	path);
BOOL		RenameFile(string oldPath, string newPath);
FILE*		OpenFile(string	fileName, string mode);



}