#include "StringFuncs.h"
#include "FileSystemFuncs.h"

namespace GMT
{


BOOL	WriteToTextFile (string fileName, string strText)
{
	ReplaceAll(fileName,"\\","/");
	FILE *fp = OpenFile(fileName,"w");
	if (!fp) return FALSE;
	fprintf(fp,"%s",strText.data());
	fclose(fp);
	return TRUE;
}


string	GetPath (string fileName)
{
	ReplaceAll(fileName,"\\","/");
	if (fileName.find_last_of("/")>=0) return fileName.substr(0,fileName.find_last_of("/")+1);
	else return "";
}


BOOL FileExists (string fileName)
{
	ReplaceAll(fileName,"\\","/");
	wstring fileNameW;
	utf8toWStr(fileNameW,fileName);
	return !(GetFileAttributes(fileNameW.c_str()) == INVALID_FILE_ATTRIBUTES);
}


BOOL IsDirectory (string path)
{
	ReplaceAll(path,"\\","/");
	if (!FileExists(path)) return FALSE;
	wstring pathW;
	utf8toWStr(pathW,path);
	return (GetFileAttributes(pathW.c_str()) & FILE_ATTRIBUTE_DIRECTORY);
}


string	RemoveEndingSlash(string	folderName)
{		
	ReplaceAll(folderName,"\\","/");

	return	(folderName == "") ? "" :
			(folderName[folderName.length()-1]== '/') ? folderName.substr(0,folderName.length()-1) : folderName;
}


string		GetAbsolutePath (string basePath, string relativePath)
{
	ReplaceAll(basePath,"\\","/");
	ReplaceAll(relativePath,"\\","/");
	basePath = RemoveEndingSlash(basePath);

	while (relativePath[0]==L'/' || relativePath[0]==' '|| relativePath[0]=='.')
	{
		if (relativePath.find(L'/')==string::npos || basePath.rfind(L'/')==string::npos)
		{
			if (relativePath[0]=='.') break;
			else return "";
		}
		relativePath	= relativePath.substr(relativePath.find(L'/')+1,relativePath.length()-relativePath.find(L'/')-1);
		basePath		= basePath.substr(0,basePath.rfind(L'/'));
	}

	return (basePath+ "/" + relativePath);
}


string RemovePath(const string& fileName)
{
	return fileName.substr(fileName.find_last_of("/")+1);	
}


string RemoveExtension (string &fileName)
{
	int n = fileName.rfind(L'.');
	return fileName.substr(0,n);	
}


BOOL FindFilesInFolderByPattern (list<string> &fileList, string searchPattern)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	wstring searchPatternW;
	utf8toWStr(searchPatternW, searchPattern);

	hFind = FindFirstFile(searchPatternW.data(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	
	while (true)
	{
		if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
		{
			string findFile;
			wstrToUtf8(findFile,FindFileData.cFileName);
			fileList.push_back(GetAbsolutePath(GetPath(searchPattern),findFile));
		}
		if (!FindNextFile(hFind,&FindFileData)) break;
	}

	return TRUE;
}


BOOL FindFilesInFolderByExtension (list<string> &fileList, string strFolder, string	strExtension, BOOL bRecursive)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	
	wstring pathW;
	utf8toWStr(pathW,GetAbsolutePath(strFolder,"*"));
	hFind = FindFirstFile(pathW.c_str(), &FindFileData);
	
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	
	while (true)
	{
		string findFile;
		wstrToUtf8(findFile,FindFileData.cFileName);
		if ((FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) && (FindFileData.dwFileAttributes!=48))
		{
			if (MakeLower(findFile).rfind("." + MakeLower(strExtension)) != string::npos)
				fileList.push_back(GetAbsolutePath(strFolder,findFile));
		}
		else if (bRecursive)
		{
			if ((findFile != ".") && (findFile != ".."))	
				FindFilesInFolderByExtension (fileList,GetAbsolutePath(strFolder,findFile),strExtension,TRUE);
		}
		if (!FindNextFile(hFind,&FindFileData)) break;
	}

	return TRUE;
}


BOOL writeWLDFile (string rasterFile, double dULx, double dULy, double dRes)
{
	if (rasterFile.length()>1)
	{
		rasterFile[rasterFile.length()-2]=rasterFile[rasterFile.length()-1];
		rasterFile[rasterFile.length()-1]='w';
	}
	
	FILE *fp = OpenFile(rasterFile,"w");
	if (!fp) return FALSE;

	fprintf(fp,"%.10lf\n",dRes);
	fprintf(fp,"%lf\n",0.0);
	fprintf(fp,"%lf\n",0.0);
	fprintf(fp,"%.10lf\n",-dRes);
	fprintf(fp,"%.10lf\n",dULx);
	fprintf(fp,"%.10lf\n",dULy);
	
	fclose(fp);

	return TRUE;
}


FILE*		OpenFile(string	fileName, string mode)
{
	wstring	fileNameW, modeW;
	utf8toWStr(fileNameW,fileName);
	utf8toWStr(modeW,mode);

	return _wfopen(fileNameW.c_str(),modeW.c_str());
}


BOOL		RenameFile(string oldPath, string newPath)
{
	wstring oldPathW, newPathW;
	utf8toWStr(oldPathW,oldPath);
	utf8toWStr(newPathW,newPath);
	return _wrename(oldPathW.c_str(),newPathW.c_str());
}


BOOL		DeleteFile(string path)
{
	wstring	pathW;
	utf8toWStr(pathW,path);
	return ::DeleteFile(pathW.c_str());
}


BOOL		CreateDirectory(string path)
{
	
	wstring	pathW;
	utf8toWStr(pathW,path);
	return ::CreateDirectory(pathW.c_str(),NULL);
}


BOOL	SaveDataToFile(string fileName, void *pData, int size)
{
	FILE *fp;
	if (!(fp = OpenFile(fileName,"wb"))) return FALSE;
	fwrite(pData,sizeof(BYTE),size,fp);
	fclose(fp);
	
	return TRUE;
}


string		GetExtension (string path)
{
	int n = path.rfind(L'.');
	if (n<0) return "";
	else return path.substr(n+1,path.size()-n-1);	
}


BOOL ReadDataFromFile(string fileName, BYTE *&pData, unsigned int &size)
{
	FILE *fp = OpenFile(fileName,"rb");
	if (!fp) return FALSE;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	pData = new BYTE[size];
	fread(pData,sizeof(BYTE),size,fp);

	fclose(fp);
	return TRUE;
}


}