#define PLUGIN_VERSION " v2.0.3"
#define CS_MODULE_TITLE "Classic Spectrum Analyzer" PLUGIN_VERSION

void FFTInit(unsigned int nNewFft);
void ConfigStereo(struct winampVisModule *);
void OpenConfigStereoWindow();
int AtAnStInit(struct winampVisModule *);
void AtAnQuit(struct winampVisModule *);
int AtAnStDirectRender(struct winampVisModule *);
void DefaultSettings();
void CalculateCommonVariables();

//void (*CalculateVariables)();
void CalculateVariablesStereo();
void CalculateVariablesNull();

//int (*LevelCalc)(int low, int high, int NumCh, unsigned char SpecData[2][576]);
//int AverageLevelCalc(int low, int high, int NumCh, unsigned char SpecData[2][576]);
//int UnionLevelCalc(int low, int high, int NumCh, unsigned char SpecData[2][576]);
//int AverageLevelCalcStereo(int low, int high, int NumCh, unsigned char SpecData[2][576]);
//int UnionLevelCalcStereo(int low, int high, int NumCh, unsigned char SpecData[2][576]);

void EraseWindow(HDC hdc);
void EraseWindow();

//void (*BackgroundDraw)(unsigned char);
void BackgroundBlack(unsigned char);
void BackgroundBlackFast(unsigned char);
void BackgroundFade(unsigned char);
void BackgroundFadeMirror(unsigned char);
void BackgroundFadeReflection(unsigned char);
void BackgroundFlash(unsigned char);
void BackgroundGrid(unsigned char);
void BackgroundGridMirror(unsigned char);
void BackgroundGridReflection(unsigned char);
void BackgroundSolid(unsigned char);

void CalculateAuxColours(void);
void CalculateDarkMirrorAuxColours(void);
void CalculateWaveyReflectionAuxColours(void);
void CalculateShadowAuxColours(void);
void CalculateFadeShadowAuxColours(void);
void CalculateDoubleShadowAuxColours(void);
void CalculateSmokeAuxColours(void);

//void (*RenderBars)();
void RenderSingleBars();
void RenderWideBars();
void RenderSingleBarsMirror();
void RenderWideBarsMirror();
void RenderSingleBarsDarkMirror();
void RenderWideBarsDarkMirror();
void RenderSingleBarsWaveyReflection();
void RenderWideBarsWaveyReflection();
void RenderSingleBarsShadow();
void RenderWideBarsShadow();
void RenderSingleBarsFadeShadow();
void RenderWideBarsFadeShadow();
void RenderSingleBarsDoubleShadow();
void RenderWideBarsDoubleShadow();
void RenderSingleBarsSmoke();
void RenderWideBarsSmoke();

//void (*PeakLevelEffect)();
void PeakLevelNormal();
void PeakLevelFall();
void PeakLevelRise();
void PeakLevelFallAndRise();
void PeakLevelRiseFall();
void PeakLevelSparks();

void ClearBackground();
void DrawLevelGraph(HWND, int *);
void DrawColourRamp(HWND, COLORREF *);
void DrawSolidColour(HWND, COLORREF, int);
void FadeColours(COLORREF *, int, int);
void CreateRandomRamp(COLORREF *);
void UpdateColourLookup();
void UpdatePeakColourLookup();
void RandomColourLookup();
void UpdateAllColours(void);
void CalculateAndUpdate(void);

// File functions
void GetProfileINIFilename(wchar_t *szBuf, const wchar_t *cszProfile);
int CreateProfileDirectory(void);
int FileExists(const wchar_t *cszFilename);
void DeleteProfile(const wchar_t *cszProfile);
HANDLE FindProfileFiles(LPWIN32_FIND_DATAA pfd);
void EnumProfilesToControl(HWND hDlg, int nIDDlgItem, UINT nMsg, UINT nSelMsg);
int GetRelativeProfiles(const wchar_t *szCurrent, wchar_t *szPrevious, wchar_t *szNext);
unsigned int CountProfileFiles(void);
int LoadProfileIni(const wchar_t *cszProfile);
int LoadProfile(const wchar_t *cszProfile);
int LoadTempProfile(const wchar_t *cszProfile);
int LoadCurrentProfile(void);
int LoadCurrentProfileOrCreateDefault(void);
void LoadNextProfile(void);
void LoadPreviousProfile(void);
int LoadProfileNumber(unsigned int nProfileNumber);
void LoadUserColours(void);
void LoadWindowPostion(RECT *pr);
void SaveMainIniSettings(void);
int SaveProfileIni(const wchar_t *cszProfile);
void SaveProfileNameEntry(const wchar_t *cszProfile);
int SaveTempCurrentSettings(void);
int SaveProfile(const wchar_t *cszProfile);
void ValidateRectPosition(int nMax, LONG *pnLeft, LONG *pnRight);
void AboutMessage(void);

LRESULT CALLBACK AtAnWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL ConfigDialog_Notify(HWND hwndDlg, LPARAM lParam);
BOOL CALLBACK ConfigDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM);
BOOL CALLBACK LevelDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM);
BOOL CALLBACK StyleDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM);
BOOL CALLBACK ColourDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM);
void SaveDialog_UpdateProfilesProperty(void);
void SaveDialog_LoadProfile(HWND hwndDlg);
BOOL CALLBACK SaveDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM);
BOOL CALLBACK ProfileSelectDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM);
//BOOL CALLBACK StereoDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM);

/*winampVisModule Osc_Vis_mod, AtAn_Vis_mod, AtAnSt_Vis_mod;
int *level, *peaklevel, peakchange[576 * 2];
short int peakreferencelevel[576 * 2];
int levelbuffer[3][576 * 2], peaklevelbuffer[3][576 * 2];
int falloffrate, peakchangerate, requested_band_width, x_spacing, y_spacing, high_freq, low_freq;
int win_height, win_width, draw_x_start, draw_y_start, positioning, location, image_width,
          left_border, right_border, top_border, bottom_border, x_position, y_position,
          saved_win_width;
int draw_height, draw_width, band_width, bands, total_width;
int levelbase, backgrounddraw, barcolourstyle, peakcolourstyle, effect, peakleveleffect;
bool reverseleft, reverseright, force_redraw;
double height_scale, draw_height_scaler, peak_fade_scaler;
float analysis_step, level_height_scaler, left_analysis_step, left_starting_value,
            right_analysis_step, right_starting_value;
int level_func[256], volume_func[256];
unsigned char *colour_lookup[256], *peak_lookup[256];
short int peak_level_lookup[256][256], peak_level_length[256];
char *module_text, *module_title, *ini_file_mainsection;
HINSTANCE ModulehDllInstance;
HDC memDC;
HBITMAP memBM;
COLORREF FreqBarColour[256], VolumeColour[256], PeakColour[256], BorderColour[3];
COLORREF *rgbbuffer, AuxColour[3][256], *SmallAuxColour[6], *colourplane[256];
BITMAPINFO bmi;
WNDCLASS wc_smallwin;
HWND hatan, config_win;
HMENU popupmenu;
*/

extern winampVisModule AtAnSt_Vis_mod;
extern winampVisModule AtAn_Vis_mod;
