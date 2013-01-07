#pragma once
#include "stdafx.h"



namespace GMT
{

wstring			MakeLower				(wstring str);
void			ReplaceAll				(wstring	&str, const wstring	&from, const wstring	&to);
BOOL			ConvertStringToRGB		(wstring strColor, BYTE rgb[3]);			
wstring			ConvertIntToWString		(int number);
string			ConvertIntToString		(int number);
int				Ustrlen					(const unsigned char *str);


typedef std::string Str;
typedef std::wstring WStr;

std::ostream&	operator<<				(std::ostream& f, const WStr& s);
std::istream&	operator>>				(std::istream& f, WStr& s);
void			utf8toWStr				(WStr& dest, const Str& input);
void			wstrToUtf8				(Str& dest, const WStr& input);

}


