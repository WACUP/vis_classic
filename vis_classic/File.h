// Win32 INI file write functions
int WritePrivateProfileBool(const wchar_t *cszSection, const wchar_t *cszKey, bool bValue, const wchar_t *cszFilename);
int WritePrivateProfileFloat(const wchar_t *cszSection, const wchar_t *cszKey, float fValue, const wchar_t *cszFilename);
int WritePrivateProfileInt(const wchar_t *cszSection, const wchar_t *cszKey, int nValue, const wchar_t *cszFilename);
int WritePrivateProfileIntArray(const wchar_t *cszSection, int *pnValues, unsigned int nSize, const wchar_t *cszFilename);
int WritePrivateProfileColour(const wchar_t *cszSection, const char *cszKey, COLORREF crValue, const wchar_t *cszFilename);
int WritePrivateProfileColourArray(const wchar_t *cszSection, COLORREF *pcrValues, unsigned int nSize, const wchar_t *cszFilename);

// Win32 INI file read functions
int ReadFileToBuffer(const wchar_t *cszFilename, wchar_t *cBuf, DWORD *pdwBufLen);
bool ReadPrivateProfileString(const wchar_t *cszSection, const wchar_t *cszKey, wchar_t *szBuf, unsigned int nBufLen, const wchar_t *cszFilename);
bool ReadPrivateProfileBool(const wchar_t *cszSection, const wchar_t *cszKey, bool bDefault, const wchar_t *cszFilename);
float ReadPrivateProfileFloat(const wchar_t *cszSection, const wchar_t *cszKey, float fDefault, const wchar_t *cszFilename);
int ReadPrivateProfileInt(const wchar_t *cszSection, const wchar_t *cszKey, int nDefault, int min, int max, const wchar_t *cszFilename);
void ReadPrivateProfileIntArray(const wchar_t *cszSection, int *pnValues, unsigned int nSize, const wchar_t *cszFilename);
COLORREF ReadPrivateProfileColour(const wchar_t *cszSection, const wchar_t *cszKey, COLORREF crDefault, const wchar_t *cszFilename);
void ReadPrivateProfileColourArray(const wchar_t *cszSection, COLORREF *pcrValues, unsigned int nSize, const wchar_t *cszFilename);
