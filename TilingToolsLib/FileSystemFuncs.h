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
bool		FileExists	(string filename);
bool		IsDirectory	(string path);

bool		FindFilesByPattern (list<string> &file_list, string search_pattern);
bool		FindFilesByExtensionRecursive (list<string> &file_list, string folder, string	extension);

bool		WriteToTextFile(string filename, string str_text);
bool		WriteWLDFile	(string filename, double ul_x, double ul_y, double res);
bool		SaveDataToFile	(string filename, void *p_data, int size);
bool		ReadDataFromFile(string filename, void *&p_data, int &size);
string	GetAbsolutePath (string base_path, string relative_path);
string	GetExtension (string path);
bool		GMXCreateDirectory(string path);
bool		GMXDeleteFile(string	path);
bool		RenameFile(string old_path, string new_path);
FILE*		OpenFile(string	file_name, string mode);



}