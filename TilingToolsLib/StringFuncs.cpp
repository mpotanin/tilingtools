#include "StringFuncs.h"


namespace GMT
{

wstring MakeLower(wstring str)
{
	wstring strLower = str;
	for (int i=0;i<str.length();i++)
		strLower[i]=tolower(str[i]);
	return strLower;

}

int				Ustrlen	(const unsigned char *str)
{
	if (str == 0) return -1;
	int len = 0;
	while ((str[len] != 0) && (len<1000))
		len++;
	return (len<1000) ? len : -1;
}

void ReplaceAll(wstring	&str, const wstring	&from, const wstring	&to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        //size_t end_pos = start_pos + from.length();
		str = str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

wstring ConvertIntToWString(int number)
{
  	_TCHAR buf[10];
	swprintf(buf,L"%d",number);
	return buf;
}

string ConvertIntToString(int number)
{
  	char buf[10];
	sprintf(buf,"%d",number);
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


void utf8toWStr(WStr& dest, const Str& input){
	dest.clear();
	wchar_t w = 0;
	int bytes = 0;
	wchar_t err = L'�';
	for (size_t i = 0; i < input.size(); i++){
		unsigned char c = (unsigned char)input[i];
		if (c <= 0x7f){//first byte
			if (bytes){
				dest.push_back(err);
				bytes = 0;
			}
			dest.push_back((wchar_t)c);
		}
		else if (c <= 0xbf){//second/third/etc byte
			if (bytes){
				w = ((w << 6)|(c & 0x3f));
				bytes--;
				if (bytes == 0)
					dest.push_back(w);
			}
			else
				dest.push_back(err);
		}
		else if (c <= 0xdf){//2byte sequence start
			bytes = 1;
			w = c & 0x1f;
		}
		else if (c <= 0xef){//3byte sequence start
			bytes = 2;
			w = c & 0x0f;
		}
		else if (c <= 0xf7){//3byte sequence start
			bytes = 3;
			w = c & 0x07;
		}
		else{
			dest.push_back(err);
			bytes = 0;
		}
	}
	if (bytes)
		dest.push_back(err);
}

void wstrToUtf8(Str& dest, const WStr& input){
	dest.clear();
	for (size_t i = 0; i < input.size(); i++){
		wchar_t w = input[i];
		if (w <= 0x7f)
			dest.push_back((char)w);
		else if (w <= 0x7ff){
			dest.push_back(0xc0 | ((w >> 6)& 0x1f));
			dest.push_back(0x80| (w & 0x3f));
		}
		else if (w <= 0xffff){
			dest.push_back(0xe0 | ((w >> 12)& 0x0f));
			dest.push_back(0x80| ((w >> 6) & 0x3f));
			dest.push_back(0x80| (w & 0x3f));
		}
		else if (w <= 0x10ffff){
			dest.push_back(0xf0 | ((w >> 18)& 0x07));
			dest.push_back(0x80| ((w >> 12) & 0x3f));
			dest.push_back(0x80| ((w >> 6) & 0x3f));
			dest.push_back(0x80| (w & 0x3f));
		}
		else
			dest.push_back('?');
	}
}

Str wstrToUtf8(const WStr& str){
	Str result;
	wstrToUtf8(result, str);
	return result;
}

WStr utf8toWStr(const Str& str){
	WStr result;
	utf8toWStr(result, str);
	return result;
}

std::ostream& operator<<(std::ostream& f, const WStr& s){
	Str s1;
	wstrToUtf8(s1, s);
	f << s1;
	return f;
}

std::istream& operator>>(std::istream& f, WStr& s){
	Str s1;
	f >> s1;
	utf8toWStr(s, s1);
	return f;
}

}