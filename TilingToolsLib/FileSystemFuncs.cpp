#include "StringFuncs.h"
#include "FileSystemFuncs.h"

namespace gmx
{


bool	WriteToTextFile (string strFileName, string strText)
{
	FILE *fp = OpenFile(strFileName,"w");
	if (!fp) return FALSE;
	fprintf(fp,"%s",strText.data());
	fclose(fp);
	return TRUE;
}


string	GetPath (string strFileName)
{
	if (strFileName.find_last_of("/")!=string::npos) return strFileName.substr(0,strFileName.find_last_of("/")+1);
	else return "";
}


bool FileExists (string strFileName)
{  
#ifdef _WIN32
	wstring strFileNameW;
	utf8toWStr(strFileNameW,strFileName);

	return !(GetFileAttributesW(strFileNameW.c_str()) == INVALID_FILE_ATTRIBUTES); //GetFileAttributes ->stat
#else
  if (FILE* fp = fopen(strFileName.c_str(),"r"))
  {
    fclose(fp);
    return true;
  }
  else if (DIR* psDIR = opendir(strFileName.c_str()))
  {
    closedir(psDIR);
    return true;
  }
  else return false;
#endif
}


bool IsDirectory (string strPath)
{
	if (!FileExists(strPath)) return false;
#ifdef _WIN32
	wstring strPathW;
	utf8toWStr(strPathW,strPath);
	return (GetFileAttributesW(strPathW.c_str()) & FILE_ATTRIBUTE_DIRECTORY);
#else
  if (DIR* psDIR = opendir(strFileName.c_str()))
  {
    closedir(psDIR);
    return true;
  }
  else return false;
#endif
}


string	RemoveEndingSlash(string	strFolderName)
{		
	return	(strFolderName == "") ? "" :
			(strFolderName[strFolderName.length()-1]== '/') ? strFolderName.substr(0,strFolderName.length()-1) : strFolderName;
}


string		GetAbsolutePath (string strBasePath, string strRelativePath)
{
	strBasePath = RemoveEndingSlash(strBasePath);
  regex regUpLevel("^(\\.| )*\\/.+");
  
	while (regex_match(strRelativePath,regUpLevel))
	{
		if (strBasePath.find('/')==string::npos) break;
		strRelativePath	= strRelativePath.substr(strRelativePath.find('/')+1);
		strBasePath		= strBasePath.substr(0,strBasePath.rfind('/'));
	}

	return strBasePath == "" ? strRelativePath : strBasePath+ "/" + strRelativePath;
}


string RemovePath(string strFileName)
{
 	return  (strFileName.find_last_of("/") != string::npos) ?
          strFileName.substr(strFileName.find_last_of("/")+1) :
          strFileName;
}


string RemoveExtension (string strFileName)
{
	int nPointPos = strFileName.rfind(L'.');
  int nSlashPos = strFileName.rfind(L'/');
  return (nPointPos>nSlashPos) ? strFileName.substr(0,nPointPos) : strFileName;	
}



bool FindFilesByPattern (list<string> &listFiles, string strSearchPattern)
{
#ifdef _WIN32
	WIN32_FIND_DATAW owinFindFileData;
	HANDLE powinFind ;

	wstring strSearchPatternW;
  utf8toWStr(strSearchPatternW, strSearchPattern);

	powinFind = FindFirstFileW(strSearchPatternW.data(), &owinFindFileData);
    
  if (powinFind == INVALID_HANDLE_VALUE)
  {
    if (powinFind) FindClose(FindClose);
    return FALSE;
  }
	
	while (true)
	{
		if (owinFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
		{
			string strFoundFile;
			wstrToUtf8(strFoundFile,owinFindFileData.cFileName);
			listFiles.push_back(GetAbsolutePath(GetPath(strSearchPattern),strFoundFile));
		}
		if (!FindNextFileW(powinFind,&owinFindFileData)) break;
	}

  if (powinFind) FindClose(powinFind);
	return TRUE;
#else
  string strBasePath = GetPath(strSearchPattern);
  string strFileNamePattern = RemovePath(strSearchPattern);
  ReplaceAll(strFileNamePattern,"*",".*");
  regex regFileNamePattern(strFileNamePattern);

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (strBasePath.c_str())) !=0) 
  {
    while ((ent = readdir (dir)) != NULL) 
    {
      if ((regex_match(ent->d_name,regFileNamePattern))&&(!IsDirectory(ent->d_name)))
        listFiles.push_back(GetAbsolutePath(strBasePath,ent->d_name));
    }
    closedir (dir);
  } 
  else return false;

  return true;
#endif
}


bool FindFilesByExtensionRecursive (list<string> &listFiles, string strFolder, string	strExtension)
{
  strExtension = (strExtension[0] == '.') ? strExtension.substr(1,strExtension.length()-1) : strExtension;
  regex regFileNamePattern((strExtension=="") ? ".*" : (".*\\."+strExtension+"$"));

#ifdef _WIN32
	WIN32_FIND_DATAW owinFindFileData;
	HANDLE powinFind;
    
  wstring strPathW;
	utf8toWStr(strPathW,GetAbsolutePath(strFolder,"*"));
  powinFind = FindFirstFileW(strPathW.c_str(), &owinFindFileData);
	
	if (powinFind == INVALID_HANDLE_VALUE)
  {
    FindClose(powinFind);
    return FALSE;
  }
	
	while (true)
	{
		string strFoundFile;
		wstrToUtf8(strFoundFile,owinFindFileData.cFileName);
		if ((owinFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) && (owinFindFileData.dwFileAttributes!=48))
		{
			if (regex_match(strFoundFile,regFileNamePattern))
        listFiles.push_back(GetAbsolutePath(strFolder,strFoundFile));
		}
		else
		{
			if ((strFoundFile != ".") && (strFoundFile != ".."))	
				FindFilesByExtensionRecursive (listFiles,GetAbsolutePath(strFolder,strFoundFile),strExtension);
		}
		if (!FindNextFileW(powinFind,&owinFindFileData)) break;
	}

  if (powinFind) FindClose(powinFind);
	return TRUE;
#else

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (strFolder.c_str())) !=0) 
  {
    while ((ent = readdir (dir)) != NULL) 
    {
      if ((ent->d_name[0]=='.')&&((ent->d_name[1]==0)||(ent->d_name[2]==0))) continue; 
      if (IsDirectory(GetAbsolutePath(strFolder,ent->d_name)))
        FindFilesByExtensionRecursive(listFiles,GetAbsolutePath(strFolder,ent->d_name),strExtension);
      else if (regex_match(ent->d_name,regFileNamePattern))
        listFiles.push_back(GetAbsolutePath(strFolder,ent->d_name));
    }
    closedir (dir);
  } 
  else return false;


  return true;
#endif
}


bool WriteWLDFile (string strRasterFile, double dblULX, double dblULY, double dblRes)
{
	if (strRasterFile.length()>1)
	{
		strRasterFile[strRasterFile.length()-2]=strRasterFile[strRasterFile.length()-1];
		strRasterFile[strRasterFile.length()-1]='w';
	}
	
	FILE *fp = OpenFile(strRasterFile,"w");
	if (!fp) return FALSE;

	fprintf(fp,"%.10lf\n",dblRes);
	fprintf(fp,"%lf\n",0.0);
	fprintf(fp,"%lf\n",0.0);
	fprintf(fp,"%.10lf\n",-dblRes);
	fprintf(fp,"%.10lf\n",dblULX+0.5*dblRes);
	fprintf(fp,"%.10lf\n",dblULY-0.5*dblRes);
	
	fclose(fp);

	return TRUE;
}


FILE*		OpenFile(string	strFileName, string strMode)
{
#ifdef _WIN32
	wstring	strFileName_w, strModeW;
	utf8toWStr(strFileName_w,strFileName);
	utf8toWStr(strModeW,strMode);
  return _wfopen(strFileName_w.c_str(),strModeW.c_str()); //_wfopen
#else
  return fopen(strFileName.c_str(),strMode.c_str());
#endif
}


bool		RenameFile(string strOldPath, string strNewPath)
{
#ifdef _WIN32
	wstring old_strPathW, new_strPathW;
	utf8toWStr(old_strPathW,strOldPath);
	utf8toWStr(new_strPathW,strNewPath);
  return _wrename(old_strPathW.c_str(),new_strPathW.c_str()); //check _wrename
#else
  return 0==rename(strOldPath.c_str(),strNewPath.c_str());
#endif
}


bool		GMXDeleteFile(string strPath)
{
#ifdef _WIN32
	wstring	strPathW;
	utf8toWStr(strPathW,strPath);
	return ::DeleteFileW(strPathW.c_str());
#else
  return (0==remove(strPath.c_str()));
#endif
}


bool		GMXCreateDirectory(string strPath)
{
#ifdef WIN32
 	  wstring	strPathW;
	  utf8toWStr(strPathW,strPath);
    return (_wmkdir(strPathW.c_str())==0);
#else
    return (mkdir(path.c_str())==0);
#endif
}


bool	SaveDataToFile(string strFileName, void *pabData, int nSize)
{
	FILE *fp;
	if (!(fp = OpenFile(strFileName,"wb"))) return FALSE;
	fwrite(pabData,sizeof(BYTE),nSize,fp);
	fclose(fp);
	
	return TRUE;
}


string		GetExtension (string strPath)
{
	int n1 = strPath.rfind('.');
  if (n1 == string::npos) return "";
  int n2 = strPath.rfind('/');

  if (n2==string::npos || n1>n2) return strPath.substr(n1+1);
	else return "";
}

string  ReadTextFile(string strFileName)
{
  FILE *fp = OpenFile(strFileName,"r");
  if (!fp) return "";

  string strFile;  
	char c;
	while (1==fscanf(fp,"%c",&c))
		strFile+=c;
	fclose(fp);

	return strFile;
}


bool ReadBinaryFile(string strFileName, void *&pabData, int &nSize)
{
	FILE *fp = OpenFile(strFileName,"rb");
	if (!fp) return FALSE;
	fseek(fp, 0, SEEK_END);
	nSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	pabData = new BYTE[nSize];
	fread(pabData,sizeof(BYTE),nSize,fp);

	fclose(fp);
	return TRUE;
}


}