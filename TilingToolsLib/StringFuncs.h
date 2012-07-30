#pragma once
#include "stdafx.h"

wstring		MakeLower		(wstring str);
void		ReplaceAll		(wstring	&str, const wstring	&from, const wstring	&to);
BOOL		ConvertColorFromStringToRGB	(wstring strColor, int rgb[3]);			
wstring		ConvertInt	(int number);



