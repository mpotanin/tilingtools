#include "StringFuncs.h"


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


BOOL	ConvertColorFromStringToRGB (wstring strColor, int rgb[3])
{
	if (strColor.length()!=6) return FALSE;
	for (int i=0;i<3;i++)
	{
		rgb[i] = 0;
		for (int j=0;j<2;j++)
		{
			switch (strColor[i*2+j])
			{
				case '0':
					break;
				case '1':
					rgb[i]+= 16*(1-j) +j;
					break;
				case '2':
					rgb[i]+=2*(16*(1-j) +j);
					break;
				case '3':
					rgb[i]+=3*(16*(1-j) +j);
					break;
				case '4':
					rgb[i]+=4*(16*(1-j) +j);
					break;
				case '5':
					rgb[i]+=5*(16*(1-j) +j);
					break;
				case '6':
					rgb[i]+=6*(16*(1-j) +j);
					break;
				case '7':
					rgb[i]+=7*(16*(1-j) +j);
					break;
				case '8':
					rgb[i]+=8*(16*(1-j) +j);
					break;
				case '9':
					rgb[i]+=9*(16*(1-j) +j);
					break;
				case 'a':
					rgb[i]+=10*(16*(1-j) +j);
					break;
				case 'b':
					rgb[i]+=11*(16*(1-j) +j);
					break;
				case 'c':
					rgb[i]+=12*(16*(1-j) +j);
					break;
				case 'd':
					rgb[i]+=13*(16*(1-j) +j);
					break;
				case 'e':
					rgb[i]+=14*(16*(1-j) +j);
					break;
				case 'f':
					rgb[i]+=15*(16*(1-j) +j);
					break;
				default:
					return FALSE;
			}
		}
	}
	return TRUE;
}