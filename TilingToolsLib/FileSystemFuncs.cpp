#include "StringFuncs.h"
#include "FileSystemFuncs.h"

BOOL	WriteStringToFile (wstring strFileName, wstring strText)
{
	FILE *fp = NULL;
	string strTextUTF8;
	wstrToUtf8(strTextUTF8, strText);
	if (!(fp=_wfopen(strFileName.data(),L"w"))) return FALSE;
	fprintf(fp,"%s",strTextUTF8.data());



	/*
	if (!(fp=_wfopen(strFileName.data(),L"w"))) return FALSE;
	fwprintf(fp,L"%ws",strText.data());
	*/	
	
	fclose(fp);
	return TRUE;
}

wstring	GetPath (wstring strFileName)
{
	return strFileName.substr(0,strFileName.find_last_of(L"/\\")+1);	
}

BOOL FileExists (wstring strFile)
{
	if (GetFileAttributes(strFile.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL IsDirectory (wstring strPath)
{
	if (!FileExists(strPath)) return FALSE;
	if (!(GetFileAttributes(strPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
	else return TRUE;
}

wstring	RemoveEndingSlash(wstring	folderName)
{		
	if ((folderName[folderName.length()-1]==L'\\')||(folderName[folderName.length()-1]==L'/')) return folderName.substr(0,folderName.length()-1);
	else return folderName;
}


wstring CombinePathAndFile (wstring strPath, wstring strFile)
{
	if (strPath.length()==0) return strFile;
	
	return RemoveEndingSlash(strPath) + L"\\"+strFile;
}

wstring RemovePath(const wstring& strFileName)
{
	return strFileName.substr(strFileName.find_last_of(L"/\\")+1);	
}

wstring RemoveExtension (wstring &strFileName)
{
	int n = strFileName.rfind(L'.');
	return strFileName.substr(0,n);	
}


BOOL FindFilesInFolderByPattern (list<wstring> &oFilesList, wstring searchPattern)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	

	hFind = FindFirstFile(searchPattern.data(), &FindFileData);
	
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}
	
	while (true)
	{
		if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
		{
			oFilesList.push_back(CombinePathAndFile(GetPath(searchPattern),FindFileData.cFileName));
		}
		if (!FindNextFile(hFind,&FindFileData)) break;
	}

	return TRUE;
}

BOOL FindFilesInFolderByExtension (list<wstring> &oFilesList, wstring strFolder, wstring	strExtension, BOOL bRecursive)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	

	hFind = FindFirstFile(CombinePathAndFile(strFolder,L"*").data(), &FindFileData);
	
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}
	
	while (true)
	{
		if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
		{
			int n = MakeLower(FindFileData.cFileName).rfind(L"." + MakeLower(strExtension));
			if (n>0)
			{
				oFilesList.push_back(CombinePathAndFile(strFolder,FindFileData.cFileName));
			}
		}
		else if (bRecursive)
		{
			wstring strTemp(FindFileData.cFileName);
			if ((strTemp!=L".")&&(strTemp!=L".."))	FindFilesInFolderByExtension (oFilesList,CombinePathAndFile(strFolder,FindFileData.cFileName),strExtension,TRUE);
		}
		if (!FindNextFile(hFind,&FindFileData)) break;
	}

	return TRUE;
}


BOOL writeWLDFile (wstring strRasterFile, double dULx, double dULy, double dRes)
{
	if (strRasterFile.length()>1)
	{
		strRasterFile[strRasterFile.length()-2]=strRasterFile[strRasterFile.length()-1];
		strRasterFile[strRasterFile.length()-1]='w';
	}
	

	FILE *fp;

	if (!(fp=_wfopen(strRasterFile.c_str(),L"w"))) return FALSE;

	fwprintf(fp,L"%.10lf\n",dRes);
	fwprintf(fp,L"%lf\n",0.0);
	fwprintf(fp,L"%lf\n",0.0);
	fwprintf(fp,L"%.10lf\n",-dRes);
	fwprintf(fp,L"%.10lf\n",dULx);
	fwprintf(fp,L"%.10lf\n",dULy);

	fclose(fp);

	return TRUE;
}


BOOL	SaveDataToFile(wstring strFileName, void *pData, int size)
{
	FILE *fp;
	if (!(fp = _wfopen(strFileName.c_str(),L"wb"))) return FALSE;
	fwrite(pData,sizeof(BYTE),size,fp);
	fclose(fp);
	
	return TRUE;
}


BOOL readDataFromFile(wstring fileName, BYTE *&pData, unsigned int &size)
{
	FILE *fp;
	if (!(fp = _wfopen(fileName.c_str(),L"rb"))) return FALSE;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	pData = new BYTE[size];
	fread(pData,sizeof(BYTE),size,fp);

	//fread(pData,sizeof(BYTE),size,fp);
	//long result;
	//FILE *fh = _wfopen(fileName.c_str(), L"rb");
	//fseek(fh, 0, SEEK_END);
	//result = ftell(fh);
	//fclose(fh);
	
	fclose(fp);
	return TRUE;
}