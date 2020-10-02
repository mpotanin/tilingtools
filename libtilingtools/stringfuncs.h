#pragma once
#include "stdafx.h"





class GMXString
{
public:

  static string			MakeLower				(string str);
  static string			  ReplaceAll				(const string	&strInput, string from, string to);
  static bool			  ConvertStringToRGB		(string str_color, unsigned char rgb[3]);			
  static string			ConvertIntToString		(int number, bool hexadecimal = FALSE, int adjust_len=0);
  //static string    ConvertIntToHexadecimalString (int number, int adjust_len = 0);
  static int				StrLen					(const unsigned char *str);
  static list<string>			  SplitCommaSeparatedText(string input_str);

  static void			utf8toWStr				(wstring& dest, const string& input);
  static void			wstrToUtf8				(string& dest, const wstring& input);
  static wstring  utf8toWStr(const string& str);
  static string   wstrToUtf8(const wstring& str);




};


