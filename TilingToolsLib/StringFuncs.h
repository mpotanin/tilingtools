#pragma once
#include "stdafx.h"




namespace GMX
{

string			MakeLower				(string str);
void			ReplaceAll				(string	&str, const string	&from, const string	&to);
BOOL			ConvertStringToRGB		(string strColor, BYTE rgb[3]);			
string			ConvertIntToString		(int number);
int				Ustrlen					(const unsigned char *str);


typedef std::string Str;
typedef std::wstring WStr;

std::ostream&	operator<<				(std::ostream& f, const WStr& s);
std::istream&	operator>>				(std::istream& f, WStr& s);
void			utf8toWStr				(WStr& dest, const Str& input);
void			wstrToUtf8				(Str& dest, const WStr& input);

}


