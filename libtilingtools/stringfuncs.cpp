#include "stringfuncs.h"


string GMXString::MakeLower(string str)
{
	string str_lower = str;
	for (int i=0;i<str.length();i++)
		str_lower[i]=tolower(str[i]);
	return str_lower;

}

int	GMXString::StrLen	(const unsigned char *str)
{
	if (str == 0) return -1;
	int len = 0;
  int max_len = 1000000;
	while ((str[len] != 0) && (len<max_len))
		len++;
	return (len<max_len) ? len : -1;
}

void GMXString::ReplaceAll(string	&strInput, string	from, string	to)
{
  size_t start_pos = 0;
  while((start_pos = strInput.find(from, start_pos)) != std::string::npos) {
		strInput = strInput.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}


string GMXString::ConvertIntToString(int number, bool hexadecimal, int adjust_len)
{
  char buf[100];
  string str;
  if (hexadecimal) sprintf(buf,"%x",number);
  else sprintf(buf,"%d",number);
  if (adjust_len==0) return buf;

  string str_buf(buf);
  if (str_buf.size()>adjust_len) return "";
  else
  {
    string str_res(adjust_len,'0');
    return str_res.replace(adjust_len-str_buf.size(),str_buf.size(),str_buf);
  }
}

list<string> GMXString::SplitCommaSeparatedText(string input_str)
{
  int pos=0;
  input_str += ",";
  list<string> listSplit;
  while ((pos=input_str.find(',', 0)) != string::npos)
  {
    if (pos == 0) listSplit.push_back("");
    else listSplit.push_back(input_str.substr(0,pos));    //listSplit.push_back()
    input_str = input_str.substr(pos+1);
  }
  
  return listSplit;
}


bool	GMXString::ConvertStringToRGB (string str_color, unsigned char rgb[3])
{
	str_color = GMXString::MakeLower(str_color);
	regex rgb_dec_pattern("([0-9]{1,3}) ([0-9]{1,3}) ([0-9]{1,3})");
	regex rgb_hex_pattern("[0-9,a,b,c,d,e,f]{6}");
  regex single_value_pattern("[0-9]{1,5}");

	if (regex_match(str_color,rgb_dec_pattern))
	{
		match_results<string::const_iterator> mr;
		regex_search(str_color, mr, rgb_dec_pattern);
    for (int i=1;i<4;i++)
			rgb[i-1] = atoi(mr[i].str().c_str());
	}
	else if (regex_match(str_color,rgb_hex_pattern))
	{
		unsigned int nColor =  strtol( str_color.c_str(),0,16);
		rgb[0] = nColor>>16;
		rgb[1] = (nColor>>8)%256;
		rgb[2] = nColor%256;
  }
  else if (regex_match(str_color,single_value_pattern))
  {
    rgb[0] = atoi(str_color.c_str());
    rgb[1]=(rgb[2]=0);
  }
  else return FALSE;

  return TRUE;
}

	
void GMXString::utf8toWStr(wstring& dest, const string& input){
	dest.clear();
	wchar_t w = 0;
	int bytes = 0;
	wchar_t err = L'�';

	for (size_t i = 0; i < input.size(); i++){
		unsigned char c = (unsigned char)input[i];
		if (c <= 0x7f){//first char
			if (bytes){
				dest.push_back(err);
				bytes = 0;
			}
			dest.push_back((wchar_t)c);
		}
		else if (c <= 0xbf){//second/third/etc bytes
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

void GMXString::wstrToUtf8(string& dest, const wstring& input){
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

string GMXString::wstrToUtf8(const wstring& str){
	string result;
	wstrToUtf8(result, str);
	return result;
}

wstring GMXString::utf8toWStr(const string& str){
	wstring result;
	utf8toWStr(result, str);
	return result;
}

/*
std::ostream& operator<<(std::ostream& f, const wstring& s){
	string s1;
	GMXString::wstrToUtf8(s1, s);
	f << s1;
	return f;
}

std::istream& operator>>(std::istream& f, wstring& s){
	string s1;
	f >> s1;
	GMXString::utf8toWStr(s, s1);
	return f;
}
*/
