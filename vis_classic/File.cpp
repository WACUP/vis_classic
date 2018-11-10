#define _WIN32_WINDOWS 0x0400
#define WINVER 0x0400

#include <windows.h>
#include <stdio.h>
#include "File.h"
#include "..\StringHelp\StringHelp.h"

// fixed length for temp strings
#define TEMP_STRING_LENGTH 256

//
// Win32 INI file read functions
//
/*
// read a boolean from a profile and verifies the value
bool ReadPrivateProfileBool(const char *cszFile, const char *cszSection, const char *cszKey, bool bDefault)
{
	return GetPrivateProfileInt(cszSection, cszKey, bDefault ? 1 : 0, cszFile) ? true : false;
}

// read an float from a profile and verifies the value
float ReadPrivateProfileFloat(const char *cszFile, const char *cszSection, const char *cszKey, float fDefault)
{
	int n = GetPrivateProfileInt(cszSection, cszKey, (int)(fDefault * 100.0), cszFile);
	return (float)n / 99.999990f;
}

// read an integer from a profile and verifies the value
int ReadPrivateProfileInt(const char *cszFile, const char *cszSection, const char *cszKey, int nDefault, int min, int max)
{
	int num = GetPrivateProfileInt(cszSection, cszKey, nDefault, cszFile);
	if(num < min)
		num = min;
	else if(num > max)
		num = max;
	return num;
}

void ReadPrivateProfileIntArray(const char *cszFile, const char *cszSection, int *pnValues, unsigned int nSize)
{
	char szKey[TEMP_STRING_LENGTH];
	for(unsigned int i = 0; i < nSize; i++)
		pnValues[i] = GetPrivateProfileInt(cszSection, itoa(i, szKey, 10), 0, cszFile);
}

// read a colour from a profile and verifies the value
COLORREF ReadPrivateProfileColour(const char *cszFile, const char *cszSection, const char *cszKey, COLORREF crDefault)
{
	char szTemp[TEMP_STRING_LENGTH];
	int num = GetPrivateProfileString(cszSection, cszKey, "0 0 0", szTemp, TEMP_STRING_LENGTH, cszFile);
	if(num > 0) {
		int r = 0, g = 0, b = 0;
		sscanf(szTemp, "%d %d %d", &r, &g, &b);
		crDefault = RGB(b, g, r);
	}
	return crDefault;
}

void ReadPrivateProfileColourArray(const char *cszFile, const char *cszSection, COLORREF *pcrValues, unsigned int nSize)
{
	char szKey[TEMP_STRING_LENGTH];
	for(unsigned int i = 0; i < nSize; i++)
		pcrValues[i] = ReadPrivateProfileColour(cszFile, cszSection, itoa(i, szKey, 10), 0);
}*/

//
// Win32 INI file write functions
//

int WritePrivateProfileBool(const char *cszSection, const char *cszKey, bool bValue, const char *cszFilename)
{
	const char *cszTrue = "1";
	const char *cszFalse = "0";
	return WritePrivateProfileString(cszSection, cszKey, bValue ? cszTrue : cszFalse, cszFilename) ? 0 : 1;
}

int WritePrivateProfileFloat(const char *cszSection, const char *cszKey, float fValue, const char *cszFilename)
{
	int n = (int)(fValue * 100.00001);
	return WritePrivateProfileInt(cszSection, cszKey, n, cszFilename) ? 0 : 1;
}

int WritePrivateProfileInt(const char *cszSection, const char *cszKey, int nValue, const char *cszFilename)
{
	char szTemp[TEMP_STRING_LENGTH];
	return WritePrivateProfileString(cszSection, cszKey, itoa(nValue, szTemp, 10), cszFilename) ? 0 : 1;
}

int WritePrivateProfileIntArray(const char *cszSection, int *pnValues, unsigned int nSize, const char *cszFilename)
{
	int e = 0;
	char szKey[TEMP_STRING_LENGTH];
	for(unsigned int i = 0; i < nSize; i++)
		e |= WritePrivateProfileInt(cszSection, itoa(i, szKey, 10), pnValues[i], cszFilename);
	return e;
}

int WritePrivateProfileColour(const char *cszSection, const char *cszKey, COLORREF crValue, const char *cszFilename)
{
	char szTemp[TEMP_STRING_LENGTH];
	wsprintf(szTemp, "%d %d %d", (int)GetBValue(crValue), (int)GetGValue(crValue), (int)GetRValue(crValue));
	return WritePrivateProfileString(cszSection, cszKey, szTemp, cszFilename) ? 0 : 1;
}

int WritePrivateProfileColourArray(const char *cszSection, COLORREF *pcrValues, unsigned int nSize, const char *cszFilename)
{
	int e = 0;
	char szKey[TEMP_STRING_LENGTH];
	for(unsigned int i = 0; i < nSize; i++)
		e |= WritePrivateProfileColour(cszSection, itoa(i, szKey, 10), pcrValues[i], cszFilename);
	return e;
}

//
// My fast INI file read functions
//

int ReadFileToBuffer(const char *cszFilename, char *cBuf, DWORD *pdwBufLen)
{
	int e = ERROR_SUCCESS;
	if(pdwBufLen) {
		HANDLE h = CreateFile(cszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(h != INVALID_HANDLE_VALUE) {
			DWORD dwSize = GetFileSize(h, NULL);
			if(dwSize <= *pdwBufLen) {
				DWORD dwRead = 0;
				if(!ReadFile(h, cBuf, dwSize, pdwBufLen, NULL))
					e = GetLastError();
			} else
				e = ERROR_MESSAGE_EXCEEDS_MAX_SIZE;
			CloseHandle(h);
		} else
			e = GetLastError();
	}
	else
		e = ERROR_INVALID_PARAMETER;
	return e;
}

bool ReadPrivateProfileString(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, char *szBuf, unsigned int nBufLen)
{
	LPCSTR pszSecStart = NULL, pszSecEnd = NULL;
	if(INIFindSection(cszStart, cszEnd, cszSection, &pszSecStart, &pszSecEnd)) {
		LPCSTR pszValStart = NULL, pszValEnd = NULL;
		if(INIFindKey(pszSecStart, pszSecEnd, cszKey, &pszValStart, &pszValEnd)) {
			StrCopy(pszValStart, pszValEnd, szBuf, nBufLen);
			return true;
		}
	}
	return false;
}

// read a boolean from a profile and verifies the value
bool ReadPrivateProfileBool(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, bool bDefault)
{
	char szBuf[TEMP_STRING_LENGTH];
	if(ReadPrivateProfileString(cszStart, cszEnd, cszSection, cszKey, szBuf, TEMP_STRING_LENGTH))
		return atoi(szBuf) == 0 ? false : true;
	return bDefault;
}

// read a float from a profile and verifies the value
float ReadPrivateProfileFloat(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, float fDefault)
{
	char szBuf[TEMP_STRING_LENGTH];
	if(ReadPrivateProfileString(cszStart, cszEnd, cszSection, cszKey, szBuf, TEMP_STRING_LENGTH))
		return atoi(szBuf) / 99.999990f;
	return fDefault;
}

// read an integer from a profile and verifies the value
int ReadPrivateProfileInt(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, int nDefault, int min, int max)
{
	char szBuf[TEMP_STRING_LENGTH];
	if(ReadPrivateProfileString(cszStart, cszEnd, cszSection, cszKey, szBuf, TEMP_STRING_LENGTH)) {
		nDefault = atoi(szBuf);
		if(nDefault < min)
			nDefault = min;
		else if(nDefault > max)
			nDefault = max;
	}
	return nDefault;
}

void ReadPrivateProfileIntArray(const char *cszStart, const char *cszEnd, const char *cszSection, int *pnValues, unsigned int nSize)
{
	char szKey[TEMP_STRING_LENGTH];
	for(unsigned int i = 0; i < nSize; i++)
		pnValues[i] = ReadPrivateProfileInt(cszStart, cszEnd, cszSection, itoa(i, szKey, 10), 0, -1, 0x7fffffff);
}

// read a colour from a profile and verifies the value
COLORREF ReadPrivateProfileColour(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, COLORREF crDefault)
{
	char szBuf[TEMP_STRING_LENGTH];
	if(ReadPrivateProfileString(cszStart, cszEnd, cszSection, cszKey, szBuf, TEMP_STRING_LENGTH)) {
		int r = 0, g = 0, b = 0;
		sscanf(szBuf, "%d %d %d", &r, &g, &b);
		crDefault = RGB(b, g, r);
	}
	return crDefault;
}

void ReadPrivateProfileColourArray(const char *cszStart, const char *cszEnd, const char *cszSection, COLORREF *pcrValues, unsigned int nSize)
{
	char szKey[TEMP_STRING_LENGTH];
	for(unsigned int i = 0; i < nSize; i++)
		pcrValues[i] = ReadPrivateProfileColour(cszStart, cszEnd, cszSection, itoa(i, szKey, 10), 0);
}

