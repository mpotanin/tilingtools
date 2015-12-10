#include "StringFuncs.h"
#include "FileSystemFuncs.h"

namespace gmx
{


bool	WriteToTextFile (string filename, string str_text)
{
	ReplaceAll(filename,"\\","/");
	FILE *fp = OpenFile(filename,"w");
	if (!fp) return FALSE;
	fprintf(fp,"%s",str_text.data());
	fclose(fp);
	return TRUE;
}


string	GetPath (string filename)
{
	ReplaceAll(filename,"\\","/");
	if (filename.find_last_of("/")!=string::npos) return filename.substr(0,filename.find_last_of("/")+1);
	else return "";
}


bool FileExists (string filename)
{

  
	ReplaceAll(filename,"\\","/");
	wstring filename_w;
	utf8toWStr(filename_w,filename);
	return !(GetFileAttributesW(filename_w.c_str()) == INVALID_FILE_ATTRIBUTES); //GetFileAttributes ->stat
}


bool IsDirectory (string path)
{
	ReplaceAll(path,"\\","/");
	if (!FileExists(path)) return FALSE;
	wstring path_w;
	utf8toWStr(path_w,path);
	return (GetFileAttributesW(path_w.c_str()) & FILE_ATTRIBUTE_DIRECTORY);
}


string	RemoveEndingSlash(string	foldername)
{		
	ReplaceAll(foldername,"\\","/");

	return	(foldername == "") ? "" :
			(foldername[foldername.length()-1]== '/') ? foldername.substr(0,foldername.length()-1) : foldername;
}


string		GetAbsolutePath (string base_path, string relative_path)
{
	ReplaceAll(base_path,"\\","/");
	ReplaceAll(relative_path,"\\","/");
	base_path = RemoveEndingSlash(base_path);

	while (relative_path[0]==L'/' || relative_path[0]==' '|| relative_path[0]=='.')
	{
		if (relative_path.find(L'/')==string::npos || base_path.rfind(L'/')==string::npos)
		{
			if (relative_path[0]=='.') break;
			else return "";
		}
		relative_path	= relative_path.substr(relative_path.find(L'/')+1);
		base_path		= base_path.substr(0,base_path.rfind(L'/'));
	}

	return base_path == "" ? relative_path : base_path+ "/" + relative_path;
}


string RemovePath(const string& filename)
{
	return  (filename.find_last_of("/") != string::npos) ?
          filename.substr(filename.find_last_of("/")+1) :
          filename;
}


string RemoveExtension (string &filename)
{
	int n_point = filename.rfind(L'.');
  int n_slash = filename.rfind(L'/');
  return (n_point>n_slash) ? filename.substr(0,n_point) : filename;	
}


bool FindFilesByPattern (list<string> &file_list, string search_pattern)
{
	WIN32_FIND_DATAW find_file_data;
	HANDLE hFind ;

	wstring search_pattern_w;
  utf8toWStr(search_pattern_w, search_pattern);

	hFind = FindFirstFileW(search_pattern_w.data(), &find_file_data);
    
  if (hFind == INVALID_HANDLE_VALUE)
  {
    if (hFind) FindClose(FindClose);
    return FALSE;
  }
	
	while (true)
	{
		if (find_file_data.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
		{
			string find_filename;
			wstrToUtf8(find_filename,find_file_data.cFileName);
			file_list.push_back(GetAbsolutePath(GetPath(search_pattern),find_filename));
		}
		if (!FindNextFileW(hFind,&find_file_data)) break;
	}

  if (hFind) FindClose(hFind);
	return TRUE;
}


bool FindFilesByExtensionRecursive (list<string> &file_list, string folder, string	extension)
{
	WIN32_FIND_DATAW find_file_data;
	HANDLE hFind;
	
  extension = (extension[0] == '.') ? extension.substr(1,extension.length()-1) : extension;
  regex search_pattern(".*\\."+extension+"$");
  
  wstring path_w;
	utf8toWStr(path_w,GetAbsolutePath(folder,"*"));
  hFind = FindFirstFileW(path_w.c_str(), &find_file_data);
	
	if (hFind == INVALID_HANDLE_VALUE)
  {
    FindClose(hFind);
    return FALSE;
  }
	
	while (true)
	{
		string find_filename;
		wstrToUtf8(find_filename,find_file_data.cFileName);
		if ((find_file_data.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) && (find_file_data.dwFileAttributes!=48))
		{
			if (regex_match(find_filename,search_pattern))
        file_list.push_back(GetAbsolutePath(folder,find_filename));
		}
		else
		{
			if ((find_filename != ".") && (find_filename != ".."))	
				FindFilesByExtensionRecursive (file_list,GetAbsolutePath(folder,find_filename),extension);
		}
		if (!FindNextFileW(hFind,&find_file_data)) break;
	}

  if (hFind) FindClose(hFind);
	return TRUE;
}


bool WriteWLDFile (string raster_file, double ul_x, double ul_y, double res)
{
	if (raster_file.length()>1)
	{
		raster_file[raster_file.length()-2]=raster_file[raster_file.length()-1];
		raster_file[raster_file.length()-1]='w';
	}
	
	FILE *fp = OpenFile(raster_file,"w");
	if (!fp) return FALSE;

	fprintf(fp,"%.10lf\n",res);
	fprintf(fp,"%lf\n",0.0);
	fprintf(fp,"%lf\n",0.0);
	fprintf(fp,"%.10lf\n",-res);
	fprintf(fp,"%.10lf\n",ul_x+0.5*res);
	fprintf(fp,"%.10lf\n",ul_y-0.5*res);
	
	fclose(fp);

	return TRUE;
}


FILE*		OpenFile(string	filename, string mode)
{
	wstring	filename_w, mode_w;
	utf8toWStr(filename_w,filename);
	utf8toWStr(mode_w,mode);

	return _wfopen(filename_w.c_str(),mode_w.c_str()); //_wfopen
}


bool		RenameFile(string old_path, string new_path)
{
	wstring old_path_w, new_path_w;
	utf8toWStr(old_path_w,old_path);
	utf8toWStr(new_path_w,new_path);
	return _wrename(old_path_w.c_str(),new_path_w.c_str()); //check _wrename
}


bool		GMXDeleteFile(string path)
{
	wstring	path_w;
	utf8toWStr(path_w,path);
	return ::DeleteFileW(path_w.c_str()); //DeleteFile ->...
}


bool		GMXCreateDirectory(string path)
{
  #ifdef WIN32
 	  wstring	path_w;
	  utf8toWStr(path_w,path);
    return (_wmkdir(path_w.c_str())==0);
  #else
    return (mkdir(path.c_str())==0);
  #endif
}


bool	SaveDataToFile(string filename, void *p_data, int size)
{
	FILE *fp;
	if (!(fp = OpenFile(filename,"wb"))) return FALSE;
	fwrite(p_data,sizeof(BYTE),size,fp);
	fclose(fp);
	
	return TRUE;
}


string		GetExtension (string path)
{
  ReplaceAll(path,"\\","/");
	int n1 = path.rfind('.');
  if (n1 == string::npos) return "";
  int n2 = path.rfind('/');

  if (n2==string::npos || n1>n2) return path.substr(n1+1);
	else return "";
}


bool ReadDataFromFile(string filename, void *&p_data, int &size)
{
	FILE *fp = OpenFile(filename,"rb");
	if (!fp) return FALSE;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	p_data = new BYTE[size];
	fread(p_data,sizeof(BYTE),size,fp);

	fclose(fp);
	return TRUE;
}


}