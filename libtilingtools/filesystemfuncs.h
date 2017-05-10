#pragma once
#include "stdafx.h"


class GMXFileSys
{
public:
  static string	RemovePath	(string strFile);
  static string RemoveExtension(string strFile);
  static string	GetPath(string strFileName);
  static string	RemoveEndingSlash(string	strFolderName);
  static bool		FileExists	(string strFileName);
  static bool		IsDirectory	(string strPath);

  static string GetRuntimeModulePath();

  static int		FindFilesByPattern (list<string> &listFiles, string strSearchPattern);
  static int		FindFilesByExtensionRecursive (list<string> &listFiles, string strFolder, string	strExtension);

  static bool		WriteToTextFile(string strFileName, string strText);
  static bool		WriteWLDFile	(string strFileName, double dblULX, double dblULY, double dblRes);
  static bool		SaveDataToFile	(string strFileName, void *pabData, int nSize);
  static bool		ReadBinaryFile(string strFileName, void *&pabData, int &nSize);
  static string ReadTextFile(string strFileName);
  static string	GetAbsolutePath (string strBasePath, string strRelativePath);
  static string	GetExtension (string strPath);
  static bool		CreateDir(string strPath);
  static bool		FileDelete(string	strPath);
  static bool		RenameFile(string strOldPath, string strNewPath);
  static FILE*	OpenFile(string	strFileName, string strMode);

};

class GMXThreading
{
public:
  static std::launch GetLaunchPolicy();
};