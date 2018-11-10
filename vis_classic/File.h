// Win32 INI file read functions
/*bool ReadPrivateProfileBool(const char *cszFile, const char *cszSection, const char *cszKey, bool bDefault);
float ReadPrivateProfileFloat(const char *cszFile, const char *cszSection, const char *cszKey, float fDefault);
int ReadPrivateProfileInt(const char *cszFile, const char *cszSection, const char *cszKey, int nDefault, int min, int max);
COLORREF ReadPrivateProfileColour(const char *cszFile, const char *cszSection, const char *cszKey, COLORREF crDefault);
void ReadPrivateProfileIntArray(const char *cszFile, const char *cszSection, int *pnValues, unsigned int nSize);
void ReadPrivateProfileColourArray(const char *cszFile, const char *cszSection, COLORREF *pcrValues, unsigned int nSize);*/

// Win32 INI file write functions
int WritePrivateProfileBool(const char *cszSection, const char *cszKey, bool bValue, const char *cszFilename);
int WritePrivateProfileFloat(const char *cszSection, const char *cszKey, float fValue, const char *cszFilename);
int WritePrivateProfileInt(const char *cszSection, const char *cszKey, int nValue, const char *cszFilename);
int WritePrivateProfileIntArray(const char *cszSection, int *pnValues, unsigned int nSize, const char *cszFilename);
int WritePrivateProfileColour(const char *cszSection, const char *cszKey, COLORREF crValue, const char *cszFilename);
int WritePrivateProfileColourArray(const char *cszSection, COLORREF *pcrValues, unsigned int nSize, const char *cszFilename);

// My fast INI file read functions
int ReadFileToBuffer(const char *cszFilename, char *cBuf, DWORD *pdwBufLen);
bool ReadPrivateProfileString(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, char *szBuf, unsigned int nBufLen);
bool ReadPrivateProfileBool(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, bool bDefault);
float ReadPrivateProfileFloat(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, float fDefault);
int ReadPrivateProfileInt(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, int nDefault, int min, int max);
void ReadPrivateProfileIntArray(const char *cszStart, const char *cszEnd, const char *cszSection, int *pnValues, unsigned int nSize);
COLORREF ReadPrivateProfileColour(const char *cszStart, const char *cszEnd, const char *cszSection, const char *cszKey, COLORREF crDefault);
void ReadPrivateProfileColourArray(const char *cszStart, const char *cszEnd, const char *cszSection, COLORREF *pcrValues, unsigned int nSize);
