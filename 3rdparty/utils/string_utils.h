#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h> // for tolower


#ifdef _DEBUG
#include <assert.h>
#endif

inline void StringFree(char* s)
{
	free(s);
}
inline void StringFree(wchar_t* s)
{
	free(s);
}

inline char* StringDup(const char* s)
{
	if( ! s )
		return NULL;

	int size = (int)strlen(s) + 1;
	char* ss = (char*)malloc(size);
	memcpy(ss, s, size);
	return ss;
}

inline wchar_t* StringDup(const wchar_t* s)
{
	if( ! s )
		return NULL;

	int size = (int)wcslen(s) + 1;
	wchar_t* ss = (wchar_t*)malloc(size * sizeof(wchar_t));
	memcpy(ss, s, size * sizeof(wchar_t));
	return ss;
}

inline char* StringDupSafe(const char* s)
{
	if( s == NULL)
		return NULL;
	else
		return StringDup(s);
}

inline wchar_t* StringDupSafe(const wchar_t* s)
{
	if( s == NULL )
		return NULL;
	else
		return StringDup(s);
}

// this function assume that szPath and szLowerPath are valid pointers
inline int StringToLower(const char* const szPath, char* szLowerPath, int nLen)
{
	int i, l = (int)strlen(szPath);
	if(l > (nLen - 1))
		l = nLen - 1;
	for(i = 0; i < l; ++i)
		szLowerPath[i] = tolower(szPath[i]);
	szLowerPath[i] = 0;
	
	return l;
}

inline int StringToLowerSafe(const char* const szPath, char* szLowerPath, int nLen)
{
	if(szPath == NULL || szLowerPath == NULL)
		return -1;
	else
		return StringToLower(szPath, szLowerPath, nLen);
}
/*
inline char* WStringToString(const wchar_t* wstring, char* string, int stringMaxLen)
{
	if(wstring == NULL || string == NULL)
		return NULL;

	WideCharToMultiByte(CP_UTF8, 0, wstring, -1, string, stringMaxLen, NULL, NULL);

	return string;
}

inline char* WStringToString(const wchar_t* wstring)
{
	if(wstring == NULL)
		return NULL;

	int size = (int)wcslen(wstring) + 1;
	char* string = (char*)malloc(size * sizeof(char));

	WideCharToMultiByte(CP_UTF8, 0, wstring, size - 1, string, size, NULL, NULL);

	return string;
}

inline wchar_t* StringToWString(const char* string, wchar_t* wstring, int wstringMaxLen)
{
	if(string == NULL)
		return NULL;

	MultiByteToWideChar(CP_UTF8, 0, string, -1, wstring, wstringMaxLen);
	
	return wstring;
}

inline wchar_t* StringToWString(const char* string)
{
	if(string == NULL)
		return NULL;

	int size = (int)strlen(string) + 1;
	wchar_t* wstring = (wchar_t*)malloc(size * sizeof(wchar_t));

	MultiByteToWideChar(CP_UTF8, 0, string, size - 1, wstring, size);
	wstring[size - 1] = L'\0';

	return wstring;
}

inline BOOL CopyToStaticString(char *szStaticString, int32 iStaticStringLen, const char *szSource)
{
	if(!szStaticString)
		return false;

	szStaticString[0] = 0;//empty the string
	if(!szSource || !szSource[0])
		return true;

	int iLen = (int)strlen(szSource) + 1;//string terminator included
	if(iLen > iStaticStringLen)
		return false;

	memcpy(szStaticString, szSource, iLen);

	return true;
};
*/

inline const char* StringGetExtention(const char* pstr)
{
    assert(pstr);
    const char* ext = strrchr(pstr, '.');
    size_t len = strlen(ext);
    if(len > 1)
        return ++ext;
    return 0;
}

#endif //_STRING_UTILS_H_