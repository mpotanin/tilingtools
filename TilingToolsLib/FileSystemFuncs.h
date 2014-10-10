#pragma once
#include "stdafx.h"
#include <windows.h>


//string		_GetAbsolutePath(string path, string file_name);
namespace gmx
{

string		RemovePath	(const string& filename);
string		RemoveExtension(string& filename);
string		GetPath(string filename);
string		RemoveEndingSlash(string	foldername);
BOOL		FileExists	(string filename);
BOOL		IsDirectory	(string path);

BOOL		FindFilesByPattern (list<string> &file_list, string search_pattern);
BOOL		FindFilesByExtensionRecursive (list<string> &file_list, string folder, string	extension);

BOOL		WriteToTextFile(string filename, string str_text);
BOOL		WriteWLDFile	(string filename, double ul_x, double ul_y, double res);
BOOL		SaveDataToFile	(string filename, void *p_data, int size);
BOOL		ReadDataFromFile(string filename, void *&p_data, int &size);
string		GetAbsolutePath (string base_path, string relative_path);
string		GetExtension (string path);
BOOL		CreateDirectory(string path);
BOOL		DeleteFile(string	path);
BOOL		RenameFile(string old_path, string new_path);
FILE*		OpenFile(string	file_name, string mode);



}