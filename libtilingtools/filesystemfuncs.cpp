#include "stringfuncs.h"
#include "filesystemfuncs.h"

#ifdef _WIN32
#define GMXFSWIN32
#endif

bool	GMXFileSys::WriteToTextFile (string strFileName, string strText)
{
	FILE *fp = GMXFileSys::OpenFile(strFileName,"w");
	if (!fp) return false;
	fprintf(fp,"%s",strText.data());
	fclose(fp);
	return true;
}


string	GMXFileSys::GetPath (string strFileName)
{
	if (strFileName.find_last_of("/")!=string::npos) return strFileName.substr(0,strFileName.find_last_of("/")+1);
	else return "";
}


bool GMXFileSys::FileExists (string strFileName)
{  
#ifdef GMXFSWIN32
	wstring strFileNameW;
	GMXString::utf8toWStr(strFileNameW,strFileName);

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


bool GMXFileSys::IsDirectory (string strPath)
{
	if (!GMXFileSys::FileExists(strPath)) return false;
#ifdef GMXFSWIN32
	wstring strPathW;
	GMXString::utf8toWStr(strPathW,strPath);
	return (GetFileAttributesW(strPathW.c_str()) & FILE_ATTRIBUTE_DIRECTORY);
#else
  if (DIR* psDIR = opendir(strPath.c_str()))
  {
    closedir(psDIR);
    return true;
  }
  else return false;
#endif
}

string GMXFileSys::GetRuntimeModulePath()
{
#ifdef GMXFSWIN32
  wchar_t exe_filename_w[_MAX_PATH + 1];
  GetModuleFileNameW(NULL, exe_filename_w, _MAX_PATH);
  string exe_filename = GMXString::wstrToUtf8(exe_filename_w);
  GMXString::ReplaceAll(exe_filename,"\\","/");
  return GMXFileSys::GetPath(exe_filename);
#else
  return "";
#endif
}



string	GMXFileSys::RemoveEndingSlash(string	strFolderName)
{		
	return	(strFolderName == "") ? "" :
			(strFolderName[strFolderName.length()-1]== '/') ? strFolderName.substr(0,strFolderName.length()-1) : strFolderName;
}


string		GMXFileSys::GetAbsolutePath (string strBasePath, string strRelativePath)
{
	strBasePath = GMXFileSys::RemoveEndingSlash(strBasePath);
  regex regUpLevel("^(\\.| )*\\/.+");
  
	while (regex_match(strRelativePath,regUpLevel))
	{
		if (strBasePath.find('/')==string::npos) break;
		strRelativePath	= strRelativePath.substr(strRelativePath.find('/')+1);
		strBasePath		= strBasePath.substr(0,strBasePath.rfind('/'));
	}

	return strBasePath == "" ? strRelativePath : strBasePath+ "/" + strRelativePath;
}


string GMXFileSys::RemovePath(string strFileName)
{
 	return  (strFileName.find_last_of("/") != string::npos) ?
          strFileName.substr(strFileName.find_last_of("/")+1) :
          strFileName;
}


string GMXFileSys::RemoveExtension (string strFileName)
{
	int nPointPos = strFileName.rfind(L'.');
  int nSlashPos = strFileName.rfind(L'/');
  return (nPointPos>nSlashPos) ? strFileName.substr(0,nPointPos) : strFileName;	
}



int GMXFileSys::FindFilesByPattern (list<string> &listFiles, string strSearchPattern)
{
#ifdef GMXFSWIN32
	WIN32_FIND_DATAW owinFindFileData;
	HANDLE powinFind ;

	wstring strSearchPatternW;
  GMXString::utf8toWStr(strSearchPatternW, strSearchPattern);

	powinFind = FindFirstFileW(strSearchPatternW.data(), &owinFindFileData);
    
  if (powinFind == INVALID_HANDLE_VALUE)
  {
    if (powinFind) FindClose(FindClose);
    return 0;
  }
	
	while (true)
	{
		if (owinFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
		{
			string strFoundFile = GMXString::wstrToUtf8(owinFindFileData.cFileName);
			listFiles.push_back(GMXFileSys::GetAbsolutePath(GMXFileSys::GetPath(strSearchPattern),strFoundFile));
		}
		if (!FindNextFileW(powinFind,&owinFindFileData)) break;
	}

  if (powinFind) FindClose(powinFind);
  return listFiles.size();
#else
  string strBasePath = GMXFileSys::GetPath(strSearchPattern);
  string strFileNamePattern = GMXFileSys::RemovePath(strSearchPattern);
  GMXString::ReplaceAll(strFileNamePattern,"*",".*");
  regex regFileNamePattern(strFileNamePattern);

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(strBasePath == "" ? "." : strBasePath.c_str())) != 0)
  {
    while ((ent = readdir (dir)) != NULL) 
    {
      if ((regex_match(ent->d_name,regFileNamePattern))&&(!GMXFileSys::IsDirectory(ent->d_name)))
        listFiles.push_back(GMXFileSys::GetAbsolutePath(strBasePath,ent->d_name));
    }
    closedir (dir);
  } 
  else return 0;

  return listFiles.size();
#endif
}


int GMXFileSys::FindFilesByExtensionRecursive (list<string> &listFiles, string strFolder, string	strExtension)
{
  strExtension = (strExtension[0] == '.') ? strExtension.substr(1,strExtension.length()-1) : strExtension;
  regex regFileNamePattern((strExtension=="") ? ".*" : (".*\\."+strExtension+"$"));

#ifdef GMXFSWIN32
	WIN32_FIND_DATAW owinFindFileData;
	HANDLE powinFind;
    
  wstring strPathW;
	GMXString::utf8toWStr(strPathW,GMXFileSys::GetAbsolutePath(strFolder,"*"));
  powinFind = FindFirstFileW(strPathW.c_str(), &owinFindFileData);
	
	if (powinFind == INVALID_HANDLE_VALUE)
  {
    FindClose(powinFind);
    return 0;
  }
	
	while (true)
	{
		string strFoundFile = GMXString::wstrToUtf8(owinFindFileData.cFileName);
		if ((owinFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) && (owinFindFileData.dwFileAttributes!=48))
		{
			if (regex_match(strFoundFile,regFileNamePattern))
        listFiles.push_back(GMXFileSys::GetAbsolutePath(strFolder,strFoundFile));
		}
		else
		{
			if ((strFoundFile != ".") && (strFoundFile != ".."))	
				GMXFileSys::FindFilesByExtensionRecursive (listFiles,GMXFileSys::GetAbsolutePath(strFolder,strFoundFile),strExtension);
		}
		if (!FindNextFileW(powinFind,&owinFindFileData)) break;
	}

  if (powinFind) FindClose(powinFind);
  return listFiles.size();
#else

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(strFolder == "" ? "." : strFolder.c_str())) != 0)
  {
    while ((ent = readdir (dir)) != NULL) 
    {
      if ((ent->d_name[0]=='.')&&((ent->d_name[1]==0)||(ent->d_name[2]==0))) continue; 
      if (GMXFileSys::IsDirectory(GMXFileSys::GetAbsolutePath(strFolder,ent->d_name)))
        GMXFileSys::FindFilesByExtensionRecursive(listFiles,GMXFileSys::GetAbsolutePath(strFolder,ent->d_name),strExtension);
      else if (regex_match(ent->d_name,regFileNamePattern))
        listFiles.push_back(GMXFileSys::GetAbsolutePath(strFolder,ent->d_name));
    }
    closedir (dir);
  } 
  else return 0;

  return listFiles.size();
#endif
}


bool GMXFileSys::WriteWLDFile (string strRasterFile, double dblULX, double dblULY, double dblRes)
{
	if (strRasterFile.length()>1)
	{
		strRasterFile[strRasterFile.length()-2]=strRasterFile[strRasterFile.length()-1];
		strRasterFile[strRasterFile.length()-1]='w';
	}
	
	FILE *fp = GMXFileSys::OpenFile(strRasterFile,"w");
	if (!fp) return false;

	fprintf(fp,"%.10lf\n",dblRes);
	fprintf(fp,"%lf\n",0.0);
	fprintf(fp,"%lf\n",0.0);
	fprintf(fp,"%.10lf\n",-dblRes);
	fprintf(fp,"%.10lf\n",dblULX+0.5*dblRes);
	fprintf(fp,"%.10lf\n",dblULY-0.5*dblRes);
	
	fclose(fp);

	return true;
}


FILE*		GMXFileSys::OpenFile(string	strFileName, string strMode)
{
#ifdef GMXFSWIN32
  return _wfopen(GMXString::utf8toWStr(strFileName).c_str(),
                 GMXString::utf8toWStr(strMode).c_str());
#else
  return fopen(strFileName.c_str(),strMode.c_str());
#endif
}


bool		GMXFileSys::RenameFile(string strOldPath, string strNewPath)
{
#ifdef GMXFSWIN32
  return 0==_wrename(GMXString::utf8toWStr(strOldPath).c_str(),
                    GMXString::utf8toWStr(strNewPath).c_str());
#else
  return 0==rename(strOldPath.c_str(),strNewPath.c_str());
#endif
}


bool		GMXFileSys::FileDelete(string strPath)
{
#ifdef GMXFSWIN32
  return ::DeleteFileW(GMXString::utf8toWStr(strPath).c_str());
#else
  return (0==remove(strPath.c_str()));
#endif
}


bool		GMXFileSys::CreateDir(string strPath)
{
#ifdef WIN32
  return (_wmkdir(GMXString::utf8toWStr(strPath).c_str()) == 0);
#else
  return (mkdir(strPath.c_str(),0777) == 0);
#endif
}


bool	GMXFileSys::SaveDataToFile(string strFileName, void *pabData, int nSize)
{
	FILE *fp;
	if (!(fp = GMXFileSys::OpenFile(strFileName,"wb"))) return false;
	fwrite(pabData,sizeof(char),nSize,fp);
	fclose(fp);
	
	return true;
}


string		GMXFileSys::GetExtension (string strPath)
{
	int n1 = strPath.rfind('.');
  if (n1 == string::npos) return "";
  int n2 = strPath.rfind('/');

  if (n2==string::npos || n1>n2) return strPath.substr(n1+1);
	else return "";
}

string  GMXFileSys::ReadTextFile(string strFileName)
{
  FILE *fp = GMXFileSys::OpenFile(strFileName,"r");
  if (!fp) return "";

  string strFileContent;  
	char c;
	while (1==fscanf(fp,"%c",&c))
    strFileContent += c;
	fclose(fp);

  return strFileContent;
}


bool GMXFileSys::ReadBinaryFile(string strFileName, void *&pabData, int &nSize)
{
	FILE *fp = GMXFileSys::OpenFile(strFileName,"rb");
	if (!fp) return false;
	fseek(fp, 0, SEEK_END);
	nSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	pabData = new char[nSize];
	fread(pabData,sizeof(char),nSize,fp);

	fclose(fp);
	return true;
}


