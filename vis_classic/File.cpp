#include <windows.h>
#include <stdio.h>
#include "File.h"
#include <stdlib.h>
#include <loader/loader/utils.h>

// fixed length for temp strings
#define TEMP_STRING_LENGTH 64

//
// Win32 INI file write functions
//

void WritePrivateProfileBool(const wchar_t *cszSection, const wchar_t *cszKey, bool bValue, const wchar_t *cszFilename)
{
	WritePrivateProfileString(cszSection, cszKey, bValue ? L"1" : L"0", cszFilename) ? 0 : 1;
}

void WritePrivateProfileFloat(const wchar_t *cszSection, const wchar_t *cszKey, float fValue, const wchar_t *cszFilename)
{
	const int n = (int)(fValue * 100.00001);
	WritePrivateProfileInt(cszSection, cszKey, n, cszFilename);
}

void WritePrivateProfileInt(const wchar_t *cszSection, const wchar_t *cszKey, int nValue, const wchar_t *cszFilename)
{
	wchar_t szTemp[TEMP_STRING_LENGTH]/* = {0}*/;
	WritePrivateProfileString(cszSection, cszKey, I2WStr(nValue, szTemp, ARRAYSIZE(szTemp)), cszFilename) ? 0 : 1;
}

void WritePrivateProfileIntArray(const wchar_t *cszSection, int *pnValues, unsigned int nSize, const wchar_t *cszFilename)
{
	wchar_t szKey[TEMP_STRING_LENGTH]/* = { 0 }*/;
	for(unsigned int i = 0; i < nSize; i++)
	{
		WritePrivateProfileInt(cszSection, I2WStr(i, szKey, ARRAYSIZE(szKey)), pnValues[i], cszFilename);
	}
}

void WritePrivateProfileColour(const wchar_t *cszSection, const wchar_t *cszKey, COLORREF crValue, const wchar_t *cszFilename)
{
	wchar_t szTemp[TEMP_STRING_LENGTH]/* = {0}*/;
	_snwprintf(szTemp, ARRAYSIZE(szTemp), L"%d %d %d", (int)GetBValue(crValue), (int)GetGValue(crValue), (int)GetRValue(crValue));
	WritePrivateProfileString(cszSection, cszKey, szTemp, cszFilename) ? 0 : 1;
}

void WritePrivateProfileColourArray(const wchar_t *cszSection, COLORREF *pcrValues, unsigned int nSize, const wchar_t *cszFilename)
{
	wchar_t szKey[TEMP_STRING_LENGTH]/* = { 0 }*/;
	for(unsigned int i = 0; i < nSize; i++)
	{
		WritePrivateProfileColour(cszSection, I2WStr(i, szKey, ARRAYSIZE(szKey)), pcrValues[i], cszFilename);
	}
}

//
// Win32 INI file read functions
//

// read a boolean from a profile and verifies the value
bool ReadPrivateProfileBool(const wchar_t *cszSection, const wchar_t *cszKey, bool bDefault, const wchar_t *cszFilename)
{
	wchar_t szBuf[TEMP_STRING_LENGTH]/* = {0}*/;
	if(GetPrivateProfileString(cszSection, cszKey, (bDefault ? L"1" : L"0"), szBuf, TEMP_STRING_LENGTH, cszFilename))
		return WStr2I(szBuf) == 0 ? false : true;
	return bDefault;
}

// read a float from a profile and verifies the value
float ReadPrivateProfileFloat(const wchar_t *cszSection, const wchar_t *cszKey, float fDefault, const wchar_t *cszFilename)
{
	wchar_t szBuf[TEMP_STRING_LENGTH]/* = {0}*/;
	if(GetPrivateProfileString(cszSection, cszKey, L"", szBuf, TEMP_STRING_LENGTH, cszFilename))
		return WStr2I(szBuf) / 99.999990f;
	return fDefault;
}

// read an integer from a profile and verifies the value
int ReadPrivateProfileInt(const wchar_t *cszSection, const wchar_t *cszKey, int nDefault, int min, int max, const wchar_t *cszFilename)
{
	int num = GetPrivateProfileInt(cszSection, cszKey, nDefault, cszFilename);
	if(num < min)
		num = min;
	else if(num > max)
		num = max;
	return num;
}

void ReadPrivateProfileIntArray(const wchar_t *cszSection, int *pnValues, unsigned int nSize, const wchar_t *cszFilename)
{
	wchar_t szKey[TEMP_STRING_LENGTH]/* = { 0 }*/;
	for(unsigned int i = 0; i < nSize; i++)
	{
		pnValues[i] = ReadPrivateProfileInt(cszSection, I2WStr(i, szKey, ARRAYSIZE(szKey)), 0, -1, 0x7fffffff, cszFilename);
	}
}

// read a colour from a profile and verifies the value
COLORREF ReadPrivateProfileColour(const wchar_t *cszSection, const wchar_t *cszKey, COLORREF crDefault, const wchar_t *cszFilename)
{
	wchar_t szBuf[TEMP_STRING_LENGTH]/* = {0}*/;
	if(GetPrivateProfileString(cszSection, cszKey, L"0 0 0", szBuf, TEMP_STRING_LENGTH, cszFilename)) {
		int r = 0, g = 0, b = 0;
		swscanf(szBuf, L"%d %d %d", &r, &g, &b);
		crDefault = RGB(b, g, r);
	}
	return crDefault;
}

void ReadPrivateProfileColourArray(const wchar_t *cszSection, COLORREF *pcrValues, unsigned int nSize, const wchar_t *cszFilename)
{
	wchar_t szKey[TEMP_STRING_LENGTH]/* = {0}*/;
	for(unsigned int i = 0; i < nSize; i++)
	{
		pcrValues[i] = ReadPrivateProfileColour(cszSection, I2WStr(i, szKey, ARRAYSIZE(szKey)), 0, cszFilename);
	}
}
