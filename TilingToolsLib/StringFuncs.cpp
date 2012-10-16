#include "StringFuncs.h"
#include "str.h"


wstring MakeLower(wstring str)
{
	wstring strLower = str;
	for (int i=0;i<str.length();i++)
		strLower[i]=tolower(str[i]);
	return strLower;

}


void ReplaceAll(wstring	&str, const wstring	&from, const wstring	&to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        //size_t end_pos = start_pos + from.length();
		str = str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

wstring ConvertInt(int number)
{
  	_TCHAR buf[10];
	swprintf(buf,L"%d",number);
	return buf;
}


BOOL	ConvertStringToRGB (wstring strColor, BYTE rgb[3])
{
	strColor = MakeLower(strColor);
	wregex rgbDecPattern(L"([0-9]{1,3}) ([0-9]{1,3}) ([0-9]{1,3})");
	wregex rgbHexPattern(L"[0-9,a,b,c,d,e,f]{6}");
	
	if (regex_match(strColor,rgbDecPattern))
	{
		match_results<wstring::const_iterator> mr;
		regex_search(strColor, mr, rgbDecPattern);

		for (int i=1;i<4;i++)
			rgb[i-1] = (int)_wtof(mr[i].str().c_str());
		return TRUE;
	}
	else if (regex_match(strColor,rgbHexPattern))
	{
		char * p;
		string		strColorUTF8;
		wstrToUtf8(strColorUTF8,strColor);
		unsigned int nColor =  strtol( strColorUTF8.c_str(), & p, 16 );
		rgb[0] = nColor>>16;
		rgb[1] = (nColor>>8)%256;
		rgb[2] = nColor%256;
		return TRUE;
	}
	return FALSE;

}