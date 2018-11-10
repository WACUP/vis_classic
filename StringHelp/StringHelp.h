void CopyPrintableChars(LPSTR szDest, LPCSTR szSourceStart, LPCSTR szSourceEnd);
LPCSTR StrCopy(LPCSTR szStart, LPCSTR szEnd, LPSTR szBuf, unsigned int nBufLen);
LPCSTR FindFilename(LPCSTR szStart, LPCSTR szEnd);
LPCSTR FindPathEnd(LPCSTR szStart, LPCSTR szEnd);
BOOL FindQuotedValueInRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR *pszStart, LPCSTR *pszEnd);
LPCSTR FindCharInRange(LPCSTR szStart, LPCSTR szEnd, CHAR c);
LPCSTR FindNewLineInRange(LPCSTR szStart, LPCSTR szEnd);
LPCSTR FindNewLineOrEndInRange(LPCSTR szStart, LPCSTR szEnd);
LPCSTR FindNonNewLineInRange(LPCSTR szStart, LPCSTR szEnd);
LPCSTR FindNonWhiteSpaceInRange(LPCSTR szStart, LPCSTR szEnd);
LPCSTR FindNonWhiteSpaceInRangeR(LPCSTR szStart, LPCSTR szEnd);
BOOL LegalFilename(LPCSTR sz);
LPCSTR StrStrRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSub);
LPCSTR StrStrRRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSub);
LPCSTR StrStrIRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSub);
LPCSTR StrStrIRRange(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSub);

// INI File helper functions (internal)
LPCSTR INIFindSectionNameStart(LPCSTR szStart, LPCSTR szEnd);
LPCSTR INIFindSectionNameEnd(LPCSTR szStart, LPCSTR szEnd);
BOOL INIFindFirstKeyAndValue(LPCSTR szStart, LPCSTR szEnd, LPCSTR *pszNameStart, LPCSTR *pszNameEnd, LPCSTR *pszValueStart, LPCSTR *pszValueEnd, LPCSTR *pLineEnd);

// INI File functions
LPCSTR INIFindSectionStart(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSection);
LPCSTR INIFindSectionEnd(LPCSTR szStart, LPCSTR szEnd);
BOOL INIFindSection(LPCSTR szStart, LPCSTR szEnd, LPCSTR szSection, LPCSTR *pszStart, LPCSTR *pszEnd);
BOOL INIFindKey(LPCSTR szStart, LPCSTR szEnd, LPCSTR szKey, LPCSTR *pszValueStart, LPCSTR *pszValueEnd);

typedef struct StringHelpString
{
	LPSTR szAllocStart;
	LPSTR szAllocEnd;
	LPSTR szEnd;
} StringHelpString;

BOOL AllocString(StringHelpString *pString, UINT nLength);
BOOL ReAllocString(StringHelpString *pString, UINT nLength);
void FreeString(StringHelpString *pString);
BOOL AllocAdditionalChars(StringHelpString *pString, INT nLength);

BOOL INISetKeyValue(StringHelpString *pString, LPCSTR szSection, LPCSTR szKey, LPCSTR szValue);
BOOL INIAppendSection(StringHelpString *pString, LPCSTR szSection);
BOOL INIAppendKey(StringHelpString *pString, LPCSTR szKey, LPCSTR szValue);
BOOL INIInsertKey(StringHelpString *pString, INT nInsertAt, LPCSTR szKey, LPCSTR szValue);
