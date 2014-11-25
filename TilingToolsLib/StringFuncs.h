#pragma once
#include "stdafx.h"




namespace gmx
{

string			MakeLower				(string str);
void			  ReplaceAll				(string	&str, const string	&from, const string	&to);
BOOL			  ConvertStringToRGB		(string str_color, BYTE rgb[3]);			
string			ConvertIntToString		(int number, BOOL hexadecimal = FALSE, int adjust_len=0);
/*string    ConvertIntToHexadecimalString (int number, int adjust_len = 0);*/
int				  StrLen					(const unsigned char *str);
int			    ParseCommaSeparatedArray	(string input_str, int *&p_arr, bool is_missing_vals=false, int nodata_val=0);		



typedef std::string Str;
typedef std::wstring WStr;

std::ostream&	operator<<				(std::ostream& f, const WStr& s);
std::istream&	operator>>				(std::istream& f, WStr& s);
void			utf8toWStr				(WStr& dest, const Str& input);
void			wstrToUtf8				(Str& dest, const WStr& input);

}


