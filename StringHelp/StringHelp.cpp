// Note: needs Shlwapi.lib
#define WINVER 0x0400
#define _WIN32_IE 0x0400
#include <windows.h>
#include <Shlwapi.h>
#include "StringHelp.h"

void CopyPrintableChars(LPSTR szDest, LPCSTR szSourceStart, LPCSTR szSourceEnd)
{
	for(;szSourceStart <= szSourceEnd; szSourceStart++) {
		if(*szSourceStart >= ' ')
			*szDest++ = *szSourceStart;
	}
	*szDest = 0;
}

LPCSTR StrCopy(LPCSTR szStart, LPCSTR szEnd, LPSTR szBuf, unsigned int nBufLen)
{
	nBufLen--;
	for(unsigned int i = 0; szStart <= szEnd && i < nBufLen; szStart++, i++, szBuf++)
		*szBuf = *szStart;
	*szBuf = 0;
	return szBuf;
}

LPCSTR FindFilename(LPCSTR szStart, LPCSTR szEnd)
{
	LPCSTR szFile = NULL;
	for(LPCSTR sz = szEnd; sz >= szStart; sz--) {
		if(*sz == '/' || *sz == '\\')
			break;
		szFile = sz;
	}
	return szFile;
}

// returns the last slash found in the range
LPCSTR FindPathEnd(LPCSTR szStart, LPCSTR szEnd)
{
	for(LPCSTR sz = szEnd; sz >= szStart; sz--) {
		if(*sz == '/' || *sz == '\\')
			return sz;
	}
	return NULL;
}

BOOL FindQuotedValueInRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR *pszStart, LPCSTR *pszEnd)
{
	szStart = FindCharInRange(szStart, szEnd, '\"');
	if(szStart) {
		++szStart;
		szEnd = FindCharInRange(szStart, szEnd, '\"');
		if(szEnd) {
			--szEnd;
			if(szEnd > szStart) {
				*pszStart = szStart;
				*pszEnd = szEnd;
				return TRUE;
			}
		}
	}
	return FALSE;
}

LPCSTR FindCharInRange(LPCSTR szStart, LPCSTR szEnd, CHAR c)
{
	for(;szStart <= szEnd; szStart++)
		if(*szStart == c)
			return szStart;
	return NULL;
}

LPCSTR FindNewLineInRange(LPCSTR szStart, LPCSTR szEnd)
{
	for(;szStart <= szEnd; szStart++)
		if(*szStart == 13 || *szStart == 10)
			return szStart;
	return NULL;
}

LPCSTR FindNewLineOrEndInRange(LPCSTR szStart, LPCSTR szEnd)
{
	LPCSTR sz = FindNewLineInRange(szStart, szEnd);
	if(!sz)
		sz = szEnd;
	return sz;
}

LPCSTR FindNonNewLineInRange(LPCSTR szStart, LPCSTR szEnd)
{
	if(szStart) {
		for(;szStart <= szEnd; szStart++)
			if(*szStart != 13 && *szStart != 10)
				return szStart;
	}
	return NULL;
}

LPCSTR FindNonWhiteSpaceInRange(LPCSTR szStart, LPCSTR szEnd)
{
	if(szStart) {
		for(;szStart <= szEnd; szStart++)
			if(*szStart > ' ')
				return szStart;
	}
	return NULL;
}

LPCSTR FindNonWhiteSpaceInRangeR(LPCSTR szStart, LPCSTR szEnd)
{
	if(szStart) {
		for(;szStart <= szEnd; szEnd--)
			if(*szEnd > ' ')
				return szEnd;
	}
	return NULL;
}

BOOL LegalFilename(LPCSTR sz)
{
	return strpbrk(sz, "\\/:*\"<>|") == NULL ? TRUE : FALSE;
}

LPCSTR StrStrRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSub)
{
	size_t nSubLen = strlen(szSub);
	if(szStart > szEnd || nSubLen < 1)
		return NULL;
	szEnd -= nSubLen - 1;
	while(memcmp(szStart, szSub, nSubLen) && szStart <= szEnd)
		++szStart;
	if(szStart <= szEnd)
		return szStart;
	return NULL;
}

LPCSTR StrStrRRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSub)
{
	size_t nSubLen = strlen(szSub);
	if(szStart > szEnd || nSubLen < 1)
		return NULL;
	szEnd -= nSubLen - 1;
	while(memcmp(szEnd, szSub, nSubLen) && szStart <= szEnd)
		--szEnd;
	if(szStart <= szEnd)
		return szEnd;
	return NULL;
}

LPCSTR StrStrIRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSub)
{
	int nSubLen = (int)strlen(szSub);
	if(szStart > szEnd || nSubLen < 1)
		return NULL;
	szEnd -= nSubLen - 1;
	while(StrCmpNI(szStart, szSub, nSubLen) && szStart <= szEnd)
		++szStart;
	if(szStart <= szEnd)
		return szStart;
	return NULL;
}

LPCSTR StrStrIRRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSub)
{
	int nSubLen = (int)strlen(szSub);
	if(szStart > szEnd || nSubLen < 1)
		return NULL;
	szEnd -= nSubLen - 1;
	while(StrCmpNI(szEnd, szSub, nSubLen) && szStart <= szEnd)
		--szEnd;
	if(szStart <= szEnd)
		return szEnd;
	return NULL;
}

// INI File helper functions

// find the start of a INI file section name
// returns a pointer to the first letter in the name
LPCSTR INIFindSectionNameStart(LPCSTR szStart, LPCSTR szEnd)
{
	szStart = FindNonWhiteSpaceInRange(szStart, szEnd);
	while(szStart && szStart <= szEnd) {
		if(szStart && *szStart == '[' && szStart < szEnd)
			return szStart + 1;
		szStart = FindNonWhiteSpaceInRange(FindNewLineOrEndInRange(szStart, szEnd) + 1, szEnd);
	}
	return NULL;
}

// given szStart as a pointer to the start of a section name
// return a pointer to the last character in the section name
LPCSTR INIFindSectionNameEnd(LPCSTR szStart, LPCSTR szEnd)
{
	LPCSTR sz = FindCharInRange(szStart, FindNewLineOrEndInRange(szStart, szEnd), ']');
	if(sz && sz - 1 >= szStart)
		return sz - 1;
	return NULL;
}

// find the key name and value in a given line
// note: if key exists but there is no value, NULL is returned for the value pointers
BOOL INIFindFirstKeyAndValue(LPCSTR szStart, LPCSTR szEnd, LPCSTR *pszNameStart, LPCSTR *pszNameEnd, LPCSTR *pszValueStart, LPCSTR *pszValueEnd, LPCSTR *pLineEnd)
{
	szStart = FindNonWhiteSpaceInRange(szStart, szEnd);
	while(szStart && szStart < szEnd) {
		*pLineEnd = FindNewLineOrEndInRange(szStart, szEnd);
		LPCSTR szNameEnd = FindCharInRange(szStart, *pLineEnd, '=');
		// if '=' found and it's past the start character
		if(szNameEnd && szNameEnd > szStart) {
			*pszValueStart = szNameEnd + 1;
			// first char of the value must be a printable char
			if(*pszValueStart > szEnd || **pszValueStart < ' ') {
				*pszValueStart = NULL;
				*pszValueEnd = NULL;
			} else
				*pszValueEnd = (*pLineEnd != szEnd) ? *pLineEnd - 1 : *pLineEnd;
			*pszNameStart = szStart;
			*pszNameEnd = FindNonWhiteSpaceInRangeR(szStart, szNameEnd - 1);
			if(*pszNameEnd >= *pszNameStart)
				return TRUE;
		}
		szStart = FindNonWhiteSpaceInRange(*pLineEnd, szEnd);
	}
	return FALSE;
}

// INI File functions

// return a pointer to the first character of the line after the section name
// returns NULL if section not found or end of string after section name
LPCSTR INIFindSectionStart(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSection)
{
	int nSectionLen = (int)strlen(szSection);
	szStart = INIFindSectionNameStart(szStart, szEnd);
	while(szStart && szStart <= szEnd) {
		LPCSTR szNameEnd = INIFindSectionNameEnd(szStart, szEnd);
		if(szNameEnd && szNameEnd - szStart + 1 == nSectionLen) {
			if(!StrCmpNI(szStart, szSection, nSectionLen))
				return FindNonNewLineInRange(FindNewLineInRange(szNameEnd, szEnd), szEnd);
		}
		szStart = INIFindSectionNameStart(szNameEnd + 1, szEnd);
	}
	return NULL;
}

// given a pointer to the start of a section, find the end of it
// the end will be the end of the string or the CR/LF before the next section name
LPCSTR INIFindSectionEnd(LPCSTR szStart, LPCSTR szEnd)
{
	LPCSTR szNameStart = INIFindSectionNameStart(szStart, szEnd);
	if(!szNameStart)
		return szEnd;
	for(; szNameStart > szStart; szNameStart--)
		if(*szNameStart == 10 || *szNameStart == 13)
			return szNameStart;
	return NULL;
}

// returns TRUE if the section is found
BOOL INIFindSection(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSection, LPCSTR *pszStart, LPCSTR *pszEnd)
{
	if(pszStart && pszEnd) {
		*pszStart = INIFindSectionStart(szStart, szEnd, szSection);
		if(*pszStart) {
			*pszEnd = INIFindSectionEnd(*pszStart, szEnd);
			if(*pszEnd > *pszStart)
				return TRUE;
		}
	}
	return FALSE;
}

// Get the value for a given key
// Assumes szStart and szEnd are a section - INIFindSection() to find a section
BOOL INIFindKey(LPCSTR szStart, LPCSTR szEnd, LPCSTR szKey, LPCSTR *pszValueStart, LPCSTR *pszValueEnd)
{
	int nKeyLen = (int)strlen(szKey);
	while(szStart && szStart < szEnd) {
		LPCSTR szNameStart, szNameEnd, pLineEnd;
		if(INIFindFirstKeyAndValue(szStart, szEnd, &szNameStart, &szNameEnd, pszValueStart, pszValueEnd, &pLineEnd)) {
			if(nKeyLen == szNameEnd - szNameStart + 1) {
				if(!StrCmpNI(szNameStart, szKey, nKeyLen))
					return TRUE;
			}
			szStart = pLineEnd;
		}
		else
			break;
	}
	return FALSE;
}


// StringHelpString functions

// alloc a string with the given length
BOOL AllocString(StringHelpString *pString, UINT nLength)
{
	if(pString) {
		pString->szAllocStart = (LPSTR)HeapAlloc(GetProcessHeap(), 0, nLength);
		if(pString->szAllocStart) {
			*pString->szAllocStart = 0;
			pString->szEnd = pString->szAllocStart - 1;
			pString->szAllocEnd = pString->szEnd + HeapSize(GetProcessHeap(), 0, pString->szAllocStart);
			return TRUE;
		}
	}
	return FALSE;
}

// re-alloc a string with the given length, length must be larger than the current string length
BOOL ReAllocString(StringHelpString *pString, UINT nLength)
{
	if(pString && (INT)nLength > pString->szAllocEnd - pString->szAllocStart + 1) {
		StringHelpString shsNew;
		if(AllocString(&shsNew, nLength)) {
			StrCopy(pString->szAllocStart, pString->szAllocEnd, shsNew.szAllocStart, shsNew.szAllocEnd - shsNew.szAllocStart + 1);
			shsNew.szEnd = shsNew.szAllocStart + (pString->szEnd - pString->szAllocStart);
			FreeString(pString);
			CopyMemory(pString, &shsNew, sizeof(StringHelpString));
			return TRUE;
		}
	}
	return FALSE;
}

// free a string
void FreeString(StringHelpString *pString)
{
	HeapFree(GetProcessHeap(), 0, pString->szAllocStart);
}

// re-alloc to ensure nLength number of chars can be added/inserted
BOOL AllocAdditionalChars(StringHelpString *pString, INT nLength)
{
	if(pString->szEnd <= pString->szAllocEnd && pString->szAllocEnd - pString->szEnd >= nLength)
		return TRUE;
	return ReAllocString(pString, (UINT)(pString->szAllocEnd - pString->szAllocStart + 1 + nLength));
}

// set the value for a given key in a given section
BOOL INISetKeyValue(StringHelpString *pString, LPCSTR szSection, LPCSTR szKey, LPCSTR szNewValue)
{
	LPSTR szSectionStart, szSectionEnd;
	if(INIFindSection(pString->szAllocStart, pString->szEnd, szSection, (LPCSTR*)&szSectionStart, (LPCSTR*)&szSectionEnd)) {
		LPSTR szValueStart, szValueEnd;
		if(INIFindKey(szSectionStart, szSectionEnd, szKey, (LPCSTR*)&szValueStart, (LPCSTR*)&szValueEnd)) {
			int nValueLen = szValueEnd - szValueStart + 1;
			int nNewValueLen = lstrlen(szNewValue);
			if(nValueLen == nNewValueLen)
				CopyMemory(szValueStart, szNewValue, nNewValueLen);
			else if(nNewValueLen < nValueLen) {
				CopyMemory(szValueStart, szNewValue, nNewValueLen);
				MoveMemory(szValueStart + nNewValueLen, szValueEnd + 1, pString->szEnd - szValueEnd);
				pString->szEnd -= nValueLen - nNewValueLen;
			} else {
				int nValueStartOffset = szValueStart - pString->szAllocStart;
				int nValueEndtOffset = szValueEnd - pString->szAllocStart;
				int nCharsAdded = nNewValueLen - nValueLen;
				if(AllocAdditionalChars(pString, nCharsAdded)) {
					szValueStart = pString->szAllocStart + nValueStartOffset;
					szValueEnd = pString->szAllocStart + nValueEndtOffset;
					MoveMemory(szValueEnd + nCharsAdded + 1, szValueEnd + 1, pString->szEnd - szValueEnd);
					CopyMemory(szValueStart, szNewValue, nNewValueLen);
					pString->szEnd += nNewValueLen - nValueLen;
					return TRUE;
				}
				return FALSE;
			}
			return TRUE;
		} else
			return INIInsertKey(pString, szSectionEnd - pString->szAllocStart + 1, szKey, szNewValue);
	} else {
		// 4 + 3 == section alloc: '[' ']' CR/LF and key/value alloc: '=' CR/LF
		INT nAlloc = 4 + 3;
		// if ini doesn't end with a new line, then append that first
		if(pString->szEnd > pString->szAllocStart && (*pString->szEnd != 10) && (*pString->szEnd != 13))
			nAlloc += 2;
		if(AllocAdditionalChars(pString, nAlloc + lstrlen(szSection) + lstrlen(szNewValue))) {
			if(nAlloc > 7) {
				pString->szEnd++;
				*pString->szEnd++ = 13;
				*pString->szEnd = 10;
			}
			if(INIAppendSection(pString, szSection))
				return INIAppendKey(pString, szKey, szNewValue);
		}
	}
	return FALSE;
}

BOOL INIAppendSection(StringHelpString *pString, LPCSTR szSection)
{
	INT nSectionLen = lstrlen(szSection);
	if(AllocAdditionalChars(pString, 4 + nSectionLen)) {
		pString->szEnd++;
		*pString->szEnd++ = '[';
		CopyMemory(pString->szEnd, szSection, nSectionLen);
		pString->szEnd += nSectionLen;
		*pString->szEnd++ = ']';
		*pString->szEnd++ = 13;
		*pString->szEnd = 10;
		return TRUE;
	}
	return FALSE;
}

BOOL INIAppendKey(StringHelpString *pString, LPCSTR szKey, LPCSTR szValue)
{
	return INIInsertKey(pString, pString->szEnd - pString->szAllocStart + 1, szKey, szValue);
}

BOOL INIInsertKey(StringHelpString *pString, INT nInsertAt, LPCSTR szKey, LPCSTR szValue)
{
	if(pString->szAllocStart + nInsertAt <= pString->szAllocEnd + 1) {
		INT nKeyLen = lstrlen(szKey);
		INT nValueLen = lstrlen(szValue);
		INT nTotalLen = 3 + nKeyLen + nValueLen;
		if(AllocAdditionalChars(pString, nTotalLen)) {
			LPSTR szInsert = pString->szAllocStart + nInsertAt;
			if(szInsert <= pString->szEnd)
				MoveMemory(szInsert + nTotalLen, szInsert, pString->szEnd - szInsert + 1);
			CopyMemory(szInsert, szKey, nKeyLen);
			szInsert += nKeyLen;
			*szInsert++ = '=';
			CopyMemory(szInsert, szValue, nValueLen);
			szInsert += nValueLen;
			*szInsert++ = 13;
			*szInsert = 10;
			pString->szEnd += nTotalLen;
			return TRUE;
		}
	}
	return FALSE;
}
