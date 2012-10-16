#pragma once
#include "stdafx.h"

wstring		MakeLower			(wstring str);
void		ReplaceAll			(wstring	&str, const wstring	&from, const wstring	&to);
BOOL		ConvertStringToRGB	(wstring strColor, BYTE rgb[3]);			
wstring		ConvertInt			(int number);



