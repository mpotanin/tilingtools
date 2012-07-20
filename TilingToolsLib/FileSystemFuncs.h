#pragma once
#include "stdafx.h"
#include "str.h"

wstring		CombinePathAndFile(wstring strPath, wstring strFile);
wstring		RemovePath	(const wstring& strFileName);
wstring		RemoveExtension(wstring& strFileName);
wstring		GetPath(wstring strFileName);
wstring		RemoveEndingSlash(wstring	folderName);
BOOL		FileExists	(wstring strFile);
BOOL		IsDirectory	(wstring strPath);
BOOL		FindFilesInFolderByPattern (list<wstring> &oFilesList, wstring searchPattern);
BOOL		FindFilesInFolderByExtension (list<wstring> &oFilesList, wstring strFolder, wstring	strExtension, BOOL bRecursive);
BOOL		WriteStringToFile(wstring strFileName, wstring strText);
BOOL		writeWLDFile	(wstring strFileName, double dULx, double dULy, double dRes);
BOOL		SaveDataToFile	(wstring strFileName, void *pData, int size);
BOOL		readDataFromFile(wstring fileName, BYTE *&pData, unsigned int &size);
				
