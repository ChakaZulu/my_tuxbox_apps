#include "estring.h"
#include <stdlib.h>

///////////////////////////////////////// eString sprintf /////////////////////////////////////////////////
eString& eString::sprintf(char *fmt, ...)
{
// Implements the normal sprintf method, to use format strings with eString
// The max length of the result string is 1024 char.
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	assign(buf);
	return *this;
}

///////////////////////////////////////// eString setNum(uint, uint) ///////////////////////////////////////
eString& eString::setNum(int val, int sys)
{
//	Returns a string that contain the value val as string
//	if sys == 16 than hexadezimal if sys == 10 than decimal
	char buf[16];
	
	if (sys == 10)
		snprintf(buf, 16, "%i", val);
	else if (sys == 16)
		snprintf(buf, 16, "%X", val);		
	
	assign(buf);
	return *this;
}

///////////////////////////////////////// eString replaceChars(char, char) /////////////////////////////
eString& eString::removeChars(char fchar)
{
//	Remove all chars that equal to fchar, and returns a reference to itself
	int index=0;

	while ( index = find(fchar, index) )
		erase(index, 1);

	return *this;
}

/////////////////////////////////////// eString upper() ////////////////////////////////////////////////
eString& eString::upper()
{
//	convert all lowercase characters to uppercase, and returns a reference to itself
	for (int i=0; i < length(); i++)
		switch(*this[i])
		{
			case 'a' ... 'z' :
				at(i) = at(i) - 32;
			break;

			case '�' :
				at(i) = '�';
			break;
			
			case '�' :
				at(i) = '�';
			break;
			
			case '�' :
				at(i) = '�';
			break;
		}

	return *this;
}

eString& eString::strReplace(const char* fstr, const eString& rstr)
{
//	replace all occurrence of fstr with rstr and, and returns a reference to itself
	int index=0;
	int fstrlen = strlen(fstr);

	while ( index = find(fstr, index) )
		replace(index++, fstrlen, rstr);
	
	return *this;
}
