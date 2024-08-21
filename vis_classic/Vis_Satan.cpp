/*  Vis Satan version history
1.0 - initial release with Classic Analyzer module
1.01 - fixed up problem when saving a profile with the same name only different
       capitalization.  No longer can end up with 2 profiles with same case
       insensitive name.
1.05 - fixed bug in calculating analysis_step (was actually doing an int divide)
1.10 - added selectable top/bottom position relative to WinAmp or Screen
1.11 - fixed bug that would cause WinAmp position to jump if placed near the top
       of the screen when plugin is launched.  Had to do with loading the
       default values, redrawing the window, loading Current Settings and
       redrawing the window again.
1.20 - Added Flash Grid Background style
1.30 - Added Classic Stereo Analyzer module
1.31 - Made the config window disapear when using Pick Colour (when plugin is
       in 'always on top' ChooseColour would appear behind it.
1.32 - Fixed bug when using the stereo module and a channel was reversed, if the
       low end of the frequency range was anything but 0, the wrong frequency
       range would be analyzed.
1.40 - Setting the Peak Indicator Change Rate to 0 shuts them off
     - Added Drawing Styles: Mirror, Reflection, Reflection Waves, Shadow,
       Double Shadow, and Smoke
1.41 - Wrong bar width was being calculated for the Stereo module
1.42 - Corrected memory allocation (should fix crashing problems)
     - Peaks weren't really shutting off all the time when set to 0
     - Mirror backgrounds would sometimes have wrong colour at the bottom
1.50 - Peaks in the wavey reflection would sometimes draw 'out of bounds'
       causing Winamp to crash.
     - added peak indicator motion
1.60 - added free positioning (attach to nothing)
     - added Fade Shadow drawing effect
     - fixed Win NT redrawing when editing Properties
*/
/* generate note about rounding errors:  in a few places small values are
added or subtracted in order to obtain to proper integer value.
draw_height_scaler is used in drawing the peak indicators, without the
adjustment a value of 255 will sometimes be scaled too low if draw_height is
an odd value.

A few variables are named with the word 'band' when 'bar' would have been better
*/
/*
TODO: faster file saving would be nice
*/

#define WACUP_BUILD

#include <windows.h>
#include <windowsx.h>
#include <crtdbg.h>
#include <commdlg.h>
#include <commctrl.h>
#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <random>
#ifdef WACUP_BUILD
#include <winamp/wa_cup.h>
#include <loader/loader/utils.h>
#include <loader/loader/paths.h>
#endif

#include <Winamp/vis.h>
#include "..\FFTNullsoft\fft.h"

#include "vis_satan.h"
#include "BarColour.h"
#include "File.h"
#include "LevelCalc.h"
#include "LevelFunc.h"
#include "LogBarTable.h"
#include "PeakColour.h"
#include "resource.h"
#include "api.h"

// this is used to identify the skinned frame to allow for embedding/control by modern skins if needed
// note: this is taken from vis_milk2 so that by having a GUID on the window, the embedding of ths vis
//		 window will work which if we don't set a guid is fine but that prevents other aspects of the
//		 skin being able to work out / interact with the window correctly (gen_ff is horrible at times)
// {0000000A-000C-0010-FF7B-01014263450C}
static const GUID embed_guid = 
{ 10, 12, 16, { 255, 123, 1, 1, 66, 99, 69, 12 } };

// bitmaps store RGB components in the opposite of the COLORREF definition
#define bmpRGB(r, g ,b) ((DWORD) (((BYTE) (b) | ((WORD) (g) << 8)) | (((DWORD) (BYTE) (r)) << 16)))

// swap the Red and Blue components
#define FixCOLORREF(colour) RGB(GetBValue(colour), GetGValue(colour), GetRValue(colour))

// maximum draw height
#define MAX_WIN_HEIGHT 255

// maximum length for a profile name
#define MAX_PROFILE_NAME_LENGTH 64

// fixed length for profile message
#define PROFILE_MESSAGE_STRING_LENGTH 384

// custom message to close properties window
// WPARAM will be one of the PROP_WIN constants
#define WM_CLOSE_PROPERTYWIN WM_USER
#define PROP_WIN_OK 0
#define PROP_WIN_CANCEL 1
#define PROP_WIN_EXIT 2

// custom message to close properties window
// WPARAM will be one of the PROP_WIN constants
#define WM_UPDATE_PROFILE_LIST WM_USER + 1

// custom message to open properties window (on the vis thread)
#define WM_OPEN_PROPERTYWIN WM_USER + 2

// display a message box that the properties is open and the vis wanted to start
#define WM_CONFIG_OPEN_ERROR WM_USER + 3

// constants used for loading/saving function pointers in ini file
#define LEVEL_MIN 0
#define LEVEL_UNION 0
#define LEVEL_AVERAGE 1
#define LEVEL_MAX 1

#define BACKGROUND_MIN 0
#define BACKGROUND_BLACK 0
#define BACKGROUND_FLASH 1
#define BACKGROUND_SOLID 2
#define BACKGROUND_GRID 3
#define BACKGROUND_FLASHGRID 4
#define BACKGROUND_MAX 4

#define BAR_MIN 0
#define BAR_CLASSIC 0
#define BAR_FIRE 1
#define BAR_LINES 2
#define BAR_WINAMPFIRE 3
#define BAR_ELEVATOR 4
#define BAR_MAX 4

#define PEAK_MIN 0
#define PEAK_FADE 0
#define PEAK_LEVEL 1
#define PEAK_LEVELFADE 2
#define PEAK_MAX 2

#define EFFECT_MIN 0
#define EFFECT_NONE 0
#define EFFECT_MIRROR 1
#define EFFECT_REFLECTION 2
#define EFFECT_SHADOW 3
#define EFFECT_DOUBLESHADOW 4
#define EFFECT_SMOKE 5
#define EFFECT_REFLECTIONWAVES 6
#define EFFECT_FADESHADOW 7
#define EFFECT_MAX 7

#define PEAK_EFFECT_MIN 0
#define PEAK_EFFECT_NORMAL 0
#define PEAK_EFFECT_FALL 1
#define PEAK_EFFECT_RISE 2
#define PEAK_EFFECT_FALLANDRISE 3
#define PEAK_EFFECT_RISEFALL 4
#define PEAK_EFFECT_SPARKS 5
#define PEAK_EFFECT_MAX 5

#define FILE_ERROR_GENERAL 1
#define FILE_ERROR_HIGHERVERSION 2
#define FILE_ERROR_DATFILESHORT 3
#define FILE_ERROR_GENERALWRITE 4

#define TEXT_ENCRYPT_KEY 542

const wchar_t *cszClassName = L"ClassicSpectrum";
const wchar_t *cszIniMainSection = L"Classic Analyzer";
const wchar_t *cszIniFilename = L"Plugins\\vis_classic.ini";
const wchar_t *cszCurrentSettings = L"Current Settings";
#ifdef WACUP_BUILD
const wchar_t *cszDefaultSettingsName = L"Classic";
#else
const wchar_t *cszDefaultSettingsName = L"Default Red & Yellow";
#endif
const wchar_t *cszDefaultProfileMessage = L"Click on a profile to load it (any changes not saved will be "
										  L"lost).\nClick OK to save the Current Settings.\nClick Cancel "\
										  L"to re-load the Current Settings (any changes will be discarded).";
const int cnProfileNameBufLen = MAX_PROFILE_NAME_LENGTH + 1;

api_service* WASABI_API_SVC = NULL;

SETUP_API_LNG_VARS;

winampVisModule AtAnSt_Vis_mod = {
	CS_MODULE_TITLE,
	NULL,	// hwndParent
	NULL,	// hDllInstance
	0,	// sRate
	0,	// nCh
	0,	// latencyMS
	15,	// delayMS
	0,	// spectrumNch
	2,	// waveformNch
	{ 0 },	// spectrumData
	{ 0 },	// waveformData
	ConfigStereo,
	AtAnStInit,
	AtAnStDirectRender,
	AtAnQuit
};

// note that the magic value 576 is used everywhere, it's really a bar count limit
#define MAX_BARS 576

// analyzer
int falloffrate = 12;
int peakchangerate = 80;
int requested_band_width = 3, x_spacing = 1, y_spacing = 2;
int levelbase = LEVEL_AVERAGE;
bool reverseleft = true, reverseright = false, mono = true;

// flash function
int volume_func[256] = {0};

// style
int backgrounddraw = 0, barcolourstyle = 0, peakcolourstyle = 0,
	effect = 0, peakleveleffect = 0, peakfalloffmode = 0;

int win_height = 20, win_width = 100, draw_x_start = 0, draw_y_start = 0;
int image_width = 0;
int draw_height = 0, draw_width = 0, band_width = 0, bands = 0, total_width = 0;
//bool force_redraw = true;
bool bPostCloseConfig = true;
double height_scale = 1.0f, draw_height_scaler = 1.0f, peak_fade_scaler = 1.0f;
float level_height_scaler = 1.0f;
int peakchange[576 * 2] = {0};
short int peakreferencelevel[576 * 2] = {0};
int levelbuffer[3][576 * 4] = {0};
int *level = levelbuffer[0];
int peaklevelbuffer[3][576 * 2] = {0};
int *peaklevel = peaklevelbuffer[0];
//int level_func[256];
unsigned char *colour_lookup[256] = {0};
unsigned char *peak_lookup[256] = {0};
short int peak_level_lookup[256][256] = {0};
short int peak_level_length[256] = {0};
COLORREF *rgbbuffer = NULL;
COLORREF AuxColour[3][256] = {0};
COLORREF *SmallAuxColour[6] = {0};
COLORREF FreqBarColour[256] = {0}, VolumeColour[256] = {0}, PeakColour[256] = {0};
COLORREF UserColours[16] = {0};  // user defined colours for Pick Colour
BITMAPINFO bmi = {0};
void (*CalculateVariables)(void) = CalculateVariablesStereo;
int (*LevelCalcStereo)(const int low, const int high, const unsigned char *spectrumData) = AverageLevelCalcStereo;
int (*LevelCalcMono)(const int low, const int high, const unsigned char *spectrumDataLeft, const unsigned char *spectrumDataRight) = AverageLevelCalcMono;
void (*BackgroundDraw)(unsigned char) = BackgroundBlack;
void (*RenderBars)(void) = RenderSingleBars;
void (*PeakLevelEffect)(void) = PeakLevelNormal;
unsigned char (*BarColourScheme)(int level, int y) = BarColourClassic;
unsigned char (*PeakColourScheme)(const int level, const int y) = PeakColourFade;
HWND hatan = NULL, config_win = NULL, hwndCurrentConfigProperty = NULL;
HMENU hmenu = NULL, popupmenu = NULL;
embedWindowState myWindowState = {0};
wchar_t szMainIniFilename[MAX_PATH] = {0};
wchar_t szCurrentProfile[MAX_PATH] = {0};
wchar_t szTempProfile[MAX_PATH] = {0};
wchar_t szProfileMessage[PROFILE_MESSAGE_STRING_LENGTH] = {0};

std::mt19937 m_rand;

// FFT
FFT m_fft;
unsigned int nFftFrequencies = 0;
unsigned char *caSpectrumData[2] = {0};
unsigned int naBarTable[MAX_BARS] = {0};
bool bFftEqualize = true, bFlip = false;
float fFftEnvelope = 0.2f;
float fFftScale = 2.0f;
float fWaveform[576] = {0};
float *fSpectrum = 0;

void FFTInit(unsigned int nNewFft)
{
	if(nNewFft > 0 && nNewFft != nFftFrequencies) {
		nFftFrequencies = nNewFft;
		delete[] caSpectrumData[0];
		delete[] caSpectrumData[1];
		delete[] fSpectrum;
		caSpectrumData[0] = new unsigned char[nFftFrequencies];
		caSpectrumData[1] = new unsigned char[nFftFrequencies];
        fSpectrum = new float[nFftFrequencies];
	}
	m_fft.Init(576, nFftFrequencies, bFftEqualize ? 1 : 0, fFftEnvelope, true);
}

void ConfigStereo(winampVisModule * /*this_mod*/)
{
	// if plugin is open, post a message to open properties in the window's thread
	if(IsWindow(hatan))
		PostMessage(hatan, WM_OPEN_PROPERTYWIN, 0, 0);
	// else make sure properties is not already open
	else if(!IsWindow(hwndCurrentConfigProperty)) {
		// use the Null variable calcualtion
		CalculateVariables = CalculateVariablesNull;
		// if settings can't be loaded, save the default
		LoadCurrentProfileOrCreateDefault();
		// open the properties
		OpenConfigStereoWindow();
	} else {
		HWND hParent = GetParent(hwndCurrentConfigProperty);
		if(IsWindow(hParent))
			BringWindowToTop(hParent);
	}
}

int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    if (uMsg == PSCB_INITIALIZED)
    {
        DarkModeSetup(hwndDlg);
    }
    return 0;
}

// Config for Stereo Classic Analyzer
void OpenConfigStereoWindow()
{
	// make sure WM_CLOSE_PROPERTYWIN will be posted
	bPostCloseConfig = true;

	// show the property page
	//if(!IsWindow(config_win)) {
	if(!IsWindow(config_win)) {
		// winamp does this for us already
		/*INITCOMMONCONTROLSEX icce = {sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};
		InitCommonControlsEx(&icce);*/

		PROPSHEETHEADER psh = {0};
		PROPSHEETPAGE page[5] = {0};

		page[0].dwSize      = sizeof(PROPSHEETPAGE);
		page[0].dwFlags     = PSP_DEFAULT;
		page[0].hInstance   = WASABI_API_LNG_HINST;
		page[0].pszTemplate = MAKEINTRESOURCE(IDD_ANALYZER);
		page[0].pfnDlgProc  = (DLGPROC) ConfigDialogProc;

		page[1].dwSize      = sizeof(PROPSHEETPAGE);
		page[1].dwFlags     = PSP_DEFAULT;
		page[1].hInstance   = WASABI_API_LNG_HINST;
		page[1].pszTemplate = MAKEINTRESOURCE(IDD_FUNCTION);
		page[1].pfnDlgProc  = (DLGPROC) LevelDialogProc;

		page[2].dwSize      = sizeof(PROPSHEETPAGE);
		page[2].dwFlags     = PSP_DEFAULT;
		page[2].hInstance   = WASABI_API_LNG_HINST;
		page[2].pszTemplate = MAKEINTRESOURCE(IDD_STYLE);
		page[2].pfnDlgProc  = (DLGPROC) StyleDialogProc;

		page[3].dwSize      = sizeof(PROPSHEETPAGE);
		page[3].dwFlags     = PSP_DEFAULT;
		page[3].hInstance   = WASABI_API_LNG_HINST;
		page[3].pszTemplate = MAKEINTRESOURCE(IDD_COLOUR);
		page[3].pfnDlgProc  = (DLGPROC) ColourDialogProc;

		/*page[4].dwSize      = sizeof(PROPSHEETPAGE);
		page[4].dwFlags     = PSP_DEFAULT;
		page[4].hInstance   = WASABI_API_LNG_HINST;
		page[4].pszTemplate = MAKEINTRESOURCE(IDD_STEREO);
		page[4].pfnDlgProc  = (DLGPROC) StereoDialogProc;*/

		page[4].dwSize      = sizeof(PROPSHEETPAGE);
		page[4].dwFlags     = PSP_DEFAULT;
		page[4].hInstance   = WASABI_API_LNG_HINST;
		page[4].pszTemplate = MAKEINTRESOURCE(IDD_PROFILESELECT);
		page[4].pfnDlgProc  = (DLGPROC) ProfileSelectDialogProc;

		psh.dwSize        = sizeof (PROPSHEETHEADER);
		psh.dwFlags       = PSH_NOAPPLYNOW | PSH_PROPTITLE | PSH_PROPSHEETPAGE |
                                             PSH_NOCONTEXTHELP | PSH_USECALLBACK;
		psh.hInstance     = WASABI_API_LNG_HINST;
		psh.hwndParent    = AtAnSt_Vis_mod.hwndParent;

		// TODO localise
		psh.pszCaption    = L"Classic Spectrum Analyzer";
		psh.nStartPage    = 0;
		psh.nPages        = 5;
		psh.ppsp          = page;
        psh.pfnCallback = PropSheetProc;

		if(IsWindow(hatan)) {
			psh.dwFlags = psh.dwFlags | PSH_MODELESS;
			psh.hwndParent = hatan;
			config_win = (HWND)CreatePropSheets(&psh);
		} else {
			if(CreatePropSheets(&psh) == 1)
				SaveTempCurrentSettings();
			hwndCurrentConfigProperty = NULL;
		}
	} else
		BringWindowToTop(config_win);
}

int CALLBACK EmbedWndCallback(embedWindowState *windowState, INT eventId, LPARAM param)
{
	if (eventId == 2)
	{
        ShowHideEmbeddedWindow(windowState->me, !param, FALSE);
	}
	return 0;
}

// init for the stereo attached analyser
int AtAnStInit(winampVisModule *this_mod)
{
	if(IsWindow(hwndCurrentConfigProperty)) {
		PostMessage(hwndCurrentConfigProperty, WM_CONFIG_OPEN_ERROR, 0, 0);
		return -1;
	}

	for(int i = 0; i < 6; i += 2) {
		SmallAuxColour[i] = AuxColour[i / 2];
		SmallAuxColour[i + 1] = &AuxColour[i / 2][128];
	}

	m_rand.seed(TEXT_ENCRYPT_KEY);

	// makes an absolute path to the ini file
#ifdef WACUP_BUILD
    CombinePath(szMainIniFilename, GetPaths()->settings_dir, cszIniFilename);
#else
	PathCombine(szMainIniFilename, (wchar_t*)SendMessage(this_mod->hwndParent, WM_WA_IPC, 0, IPC_GETINIDIRECTORYW), cszIniFilename);
#endif

	// default to under Winamp
	RECT rWinamp = {0};
	GetWindowRect(this_mod->hwndParent, &rWinamp);
	myWindowState.r.left = rWinamp.left;
	myWindowState.r.top = rWinamp.bottom;
	myWindowState.r.right = rWinamp.right;
	myWindowState.r.bottom = rWinamp.bottom + 100;

	// load saved window position
	LoadWindowPostion(&myWindowState.r);

	// this sets a GUID which can be used in a modern skin / other parts of Winamp to
	// indentify the embedded window frame such as allowing it to activated in a skin
	SET_EMBED_GUID((&myWindowState), embed_guid);
	myWindowState.flags = EMBED_FLAGS_SCALEABLE_WND |	// double-size support!
						  EMBED_FLAGS_NOWINDOWMENU |
						  EMBED_FLAGS_NOTRANSPARENCY;

    // to avoid issues, the skinned window has to be created on the
    // thread that it's being run from otherwise it'll fail create
    // or it'll do weird things when trying to close it (aka crash)
    // otherwise have to call IPC_GET_EMBEDIF (slow) & then use the
    // returned method from it which sometimes doesn't work nicely
    HWND parent = CreateEmbedWindow(&myWindowState, L"Classic Spectrum Analyzer");		// TODO localise
	if (IsWindow(parent)) {
		// if the parent is minimised then this will
		// help to prevent it appearing on screen as
		// it otherwise looks odd showing on its own
		if (IsIconic(this_mod->hwndParent) &&
			(myWindowState.wasabi_window == NULL))
		{
			myWindowState.callback = EmbedWndCallback;
        }

        //register window class
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX), 0 };
        wcex.lpfnWndProc = AtAnWndProc;
        wcex.hInstance = this_mod->hDllInstance;
        wcex.lpszClassName = cszClassName;
        wcex.hCursor = GetArrowCursor(false);

		if(RegisterClassEx(&wcex)) {
			hatan = CreateWindow(cszClassName, L"Classic Spectrum Analyzer", WS_VISIBLE |
								 WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP,
								 0, 0, 0, 0, parent, NULL, this_mod->hDllInstance, 0);

			if(!IsWindow(hatan)) {
				UnregisterClass(cszClassName, this_mod->hDllInstance);
				return 1;
			}

			// assign popup menu
			hmenu = LoadMenu(this_mod->hDllInstance, MAKEINTRESOURCE(IDM_POPUP));
			popupmenu = GetSubMenu(hmenu, 0);

			// create the drawing buffer and initialize to 0
			image_width = GetSystemMetrics(SM_CXSCREEN);
			rgbbuffer = (COLORREF *)calloc(image_width * MAX_WIN_HEIGHT, sizeof(COLORREF));

			// allocate bar colour lookup and peak table memory
			colour_lookup[0] = 0; // no colour painted for a level of zero
			colour_lookup[1] = 0;
			peak_lookup[0] = 0;
			peak_lookup[1] = 0;
			for(int i = 1; i < 256; i++) {
				colour_lookup[i] = new unsigned char[i];
				peak_lookup[i] = new unsigned char[256];
			}

			// use the Stereo variable calculation
			CalculateVariables = CalculateVariablesStereo;

			// load or make default settings
			LoadCurrentProfileOrCreateDefault();

			m_rand.seed(GetTickCount());

            // description for the DIBbits buffer (rgbbuffer)
            InitBitmapForARGB32(&bmi, image_width, MAX_WIN_HEIGHT);
            if (!bFlip)
            {
                bmi.bmiHeader.biHeight = MAX_WIN_HEIGHT;
            }

			/*SendMessage(this_mod->hwndParent, WM_WA_IPC, (WPARAM)hatan, IPC_SETVISWND);/*/
		    SendMessageTimeout(this_mod->hwndParent, WM_WA_IPC, (WPARAM)hatan, IPC_SETVISWND,
				                    SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 2000, NULL);/**/

			// to better match the current visible state of WACUP it's
			// necessary to avoid showing our window when minimised :)
			if (!IsIconic(this_mod->hwndParent))
			{
                ShowHideEmbeddedWindow(parent, TRUE, FALSE);
			}
			return 0;
		}
	}
	return -1;
}

void AtAnQuit(winampVisModule *this_mod)
{
	// unless we're closing as a classic skin then we'll
	// skip saving the current window position otherwise
	// we have the issue with windows being in the wrong
	// places after modern -> exit -> modern -> classic
	if (!myWindowState.wasabi_window)
	{
		WritePrivateProfileInt(cszIniMainSection, L"WindowPosLeft", myWindowState.r.left, szMainIniFilename);
		WritePrivateProfileInt(cszIniMainSection, L"WindowPosRight", myWindowState.r.right, szMainIniFilename);
		WritePrivateProfileInt(cszIniMainSection, L"WindowPosTop", myWindowState.r.top, szMainIniFilename);
		WritePrivateProfileInt(cszIniMainSection, L"WindowPosBottom", myWindowState.r.bottom, szMainIniFilename);
	}

	// close the config window with property exit code (changes will be lost)
	if(IsWindow(config_win))
		SendMessage(hatan, WM_CLOSE_PROPERTYWIN, PROP_WIN_EXIT, 0);

	// remove vis window, NOTE: THIS IS BAD AND NOT NEEDED EVIDENTLY
	if(IsWindow(this_mod->hwndParent))
		PostMessage(this_mod->hwndParent, WM_WA_IPC, 0, IPC_SETVISWND);

	if (IsMenu(hmenu)) {
		DestroyMenu(hmenu);
		hmenu = NULL;
	}

	if(IsWindow(myWindowState.me)) {
		DestroyWindow(myWindowState.me);
		myWindowState.me = NULL;
	}

	if (IsWindow(hatan)) {
		DestroyWindow(hatan);
		hatan = NULL;
	}

	UnregisterClass(cszClassName, this_mod->hDllInstance);

	m_fft.CleanUp();

	if(rgbbuffer) {
		free(rgbbuffer);
		rgbbuffer = NULL;
	}

	for(int i = 1; i < 256; i++) {
		delete[] colour_lookup[i];
		delete[] peak_lookup[i];
	}
	if(caSpectrumData[0])
		delete[] caSpectrumData[0];
	if(caSpectrumData[1])
		delete[] caSpectrumData[1];
}

// fast rendering of DIBbits (stereo)
int AtAnStDirectRender(winampVisModule *this_mod)
{
	// we assume we get 576 samples in from the wacup core.
	// the output of the fft has 'num_frequencies' samples,
	// and represents the frequency range 0 hz - 22,050 hz.
	for(int c = 0; c < this_mod->waveformNch; c++) {
		for(int i = 0; i < 576; i++)
			fWaveform[i] = (float)((this_mod->waveformData[c][i] ^ 128) - 128);

		m_fft.time_to_frequency_domain(fWaveform, fSpectrum);

		for(unsigned int i = 0; i < nFftFrequencies; i++) {
			unsigned int h = (unsigned int)(fSpectrum[i] / fFftScale);
			caSpectrumData[c][i] = (unsigned char)((h > 255) ? 255 : h);
		}
	}

	int volume = 0;

	if(!mono) {
		// left channel
		for(int i = 0, nLow = 0, x = reverseleft ? bands / 2 - 1: 0, nDir = reverseleft ? -1 : 1; i < bands / 2; i++, x += nDir) {
			int nHigh = nLow + naBarTable[i];
			_ASSERT((unsigned int)nLow < nFftFrequencies);
			_ASSERT((unsigned int)nHigh <= nFftFrequencies);
            const int newlevel = LevelCalcStereo(nLow, nHigh, caSpectrumData[0]);
			nLow = nHigh;
			volume += volume_func[newlevel];

			if(newlevel > (level[x] -= falloffrate))
				level[x] = newlevel;

			if(!(peakchange[x]--) || peaklevel[x] <= level[x]) {
				peakchange[x] = peak_level_length[level[x]];
				if(peakchangerate) {
					peakreferencelevel[x] = (short)level[x];
					peaklevel[x] = peak_level_lookup[level[x]][peakchange[x]];
				} else
					peakreferencelevel[x] = (short)0;
			} else
			  peaklevel[x] = peak_level_lookup[peakreferencelevel[x]][peakchange[x]];
		}

		// right channel
		for(int i = bands / 2, nLow = 0, x = reverseright ? bands - 1: bands / 2, nDir = reverseright ? -1 : 1; i < bands; i++, x += nDir) {
			int nHigh = nLow + naBarTable[i];
			_ASSERT((unsigned int)nLow < nFftFrequencies);
			_ASSERT((unsigned int)nHigh <= nFftFrequencies);
            const int newlevel = LevelCalcStereo(nLow, nHigh, caSpectrumData[1]);
			nLow = nHigh;
			volume += volume_func[newlevel];

			if(newlevel > (level[x] -= falloffrate))
				level[x] = newlevel;

			if(!(peakchange[x]--) || peaklevel[x] <= level[x]) {
				peakchange[x] = peak_level_length[level[x]];
				if(peakchangerate) {
					peakreferencelevel[x] = (short)level[x];
					peaklevel[x] = peak_level_lookup[level[x]][peakchange[x]];
				} else
					peakreferencelevel[x] = (short)0;
			}
			else
				peaklevel[x] = peak_level_lookup[peakreferencelevel[x]][peakchange[x]];
		}
	} else {
		for(int i = 0, nLow = 0, x = reverseright ? bands - 1: 0, nDir = reverseright ? -1 : 1; i < bands; i++, x += nDir) {
			int nHigh = nLow + naBarTable[i];
			_ASSERT((unsigned int)nLow < nFftFrequencies);
			_ASSERT((unsigned int)nHigh <= nFftFrequencies);
			int newlevel = LevelCalcMono(nLow, nHigh, caSpectrumData[0], caSpectrumData[1]);
			nLow = nHigh;
			volume += volume_func[newlevel];

			if(newlevel > (level[x] -= falloffrate))
				level[x] = newlevel;

			if(!(peakchange[x]--) || peaklevel[x] <= level[x]) {
				peakchange[x] = peak_level_length[level[x]];
				if(peakchangerate) {
					peakreferencelevel[x] = (short)level[x];
					peaklevel[x] = peak_level_lookup[level[x]][peakchange[x]];
				} else
					peakreferencelevel[x] = (short)0;
			} else
			  peaklevel[x] = peak_level_lookup[peakreferencelevel[x]][peakchange[x]];
		}
	}

	if (bands) {
		volume /= bands;  // make volume average of all bands calculated
	}

    BackgroundDraw((unsigned char)volume);  // clear (draw) the background

	RenderBars(); // call the drawing function

	// now blt the generated mess to the window
	HDC hdc = GetDC(hatan);
	if (hdc != NULL) {
		SetDIBitsToDevice(hdc, draw_x_start, draw_y_start, draw_width, draw_height,
                            0, 0, 0, draw_height, rgbbuffer, &bmi, DIB_RGB_COLORS);
		ReleaseDC(hatan,hdc);
	}
	return 0;
}

void CalculateAuxColours(void)
{
	// calculate aux colours
	switch(effect) {
	case EFFECT_REFLECTION:
		CalculateDarkMirrorAuxColours();
		break;
	case EFFECT_REFLECTIONWAVES:
		CalculateWaveyReflectionAuxColours();
		break;
	case EFFECT_SHADOW:
		CalculateShadowAuxColours();
		break;
	case EFFECT_FADESHADOW:
		CalculateFadeShadowAuxColours();
		break;
	case EFFECT_DOUBLESHADOW:
		CalculateDoubleShadowAuxColours();
		break;
	case EFFECT_SMOKE:
		CalculateSmokeAuxColours();
		break;
	}
}

void CalculateVariablesNull()
{
}

// recalculate the varibales common to all modules
// this gets called by one of the CalculateVariables functions
void CalculateCommonVariables()
{
	if(!peakchangerate)
		ZeroMemory(peaklevel, 576 * 2 * 4);

	// this will prevent crashing when switching between drawing effects
	ZeroMemory(levelbuffer[1], sizeof(int) * 576 * 2);
	ZeroMemory(levelbuffer[2], sizeof(int) * 576 * 2);

  if(band_width == 1) {// select the rendering engine
    switch(effect) {
      case EFFECT_MIRROR:
        RenderBars = RenderSingleBarsMirror;
        break;
      case EFFECT_REFLECTION:
        RenderBars = RenderSingleBarsDarkMirror;
        break;
      case EFFECT_REFLECTIONWAVES:
        RenderBars = RenderSingleBarsWaveyReflection;
        break;
      case EFFECT_SHADOW:
        RenderBars = RenderSingleBarsShadow;
        break;
      case EFFECT_DOUBLESHADOW:
        RenderBars = RenderSingleBarsDoubleShadow;
        break;
      case EFFECT_FADESHADOW:
        RenderBars = RenderSingleBarsFadeShadow;
        break;
      case EFFECT_SMOKE:
        RenderBars = RenderSingleBarsSmoke;
        break;
      case EFFECT_NONE:
      default:
        RenderBars = RenderSingleBars;
        break;
    }
  }
  else {
    switch(effect) {
      case EFFECT_MIRROR:
        RenderBars = RenderWideBarsMirror;
        break;
      case EFFECT_REFLECTION:
        RenderBars = RenderWideBarsDarkMirror;
        break;
      case EFFECT_REFLECTIONWAVES:
        RenderBars = RenderWideBarsWaveyReflection;
        break;
      case EFFECT_SHADOW:
        RenderBars = RenderWideBarsShadow;
        break;
      case EFFECT_DOUBLESHADOW:
        RenderBars = RenderWideBarsDoubleShadow;
        break;
      case EFFECT_FADESHADOW:
        RenderBars = RenderWideBarsFadeShadow;
        break;
      case EFFECT_SMOKE:
        RenderBars = RenderWideBarsSmoke;
        break;
      case EFFECT_NONE:
      default:
        RenderBars = RenderWideBars;
        break;
    }
  }

  // now update all the function pointers
  switch(backgrounddraw) {
    case BACKGROUND_FLASHGRID:
      switch(effect) {
        case EFFECT_MIRROR:
          BackgroundDraw = BackgroundFadeMirror;
          break;
        case EFFECT_REFLECTION:
          BackgroundDraw = BackgroundFadeReflection;
          break;
        case EFFECT_REFLECTIONWAVES:
          if(x_spacing)
            BackgroundDraw = BackgroundBlack;
          else
            BackgroundDraw = BackgroundFadeReflection;
          break;
        default:
          BackgroundDraw = BackgroundFade;
      }
      break;
    case BACKGROUND_FLASH:
      BackgroundDraw = BackgroundFlash;
      break;
    case BACKGROUND_SOLID:
      BackgroundDraw = BackgroundSolid;
      break;
    case BACKGROUND_GRID:
      switch(effect) {
        case EFFECT_MIRROR:
          BackgroundDraw = BackgroundGridMirror;
          break;
        case EFFECT_REFLECTIONWAVES:
          if(x_spacing)
            BackgroundDraw = BackgroundBlack;
          else
            BackgroundDraw = BackgroundGridReflection;
          break;
        case EFFECT_REFLECTION:
          BackgroundDraw = BackgroundGridReflection;
          break;
        default:
          BackgroundDraw = BackgroundGrid;
      }
      break;
    case BACKGROUND_BLACK:
    default:
      BackgroundDraw = BackgroundBlack;
  }

  switch(barcolourstyle) {
    case BAR_FIRE:
      BarColourScheme = BarColourFire;
      break;
    case BAR_LINES:
      BarColourScheme = BarColourLines;
      break;
    case BAR_WINAMPFIRE:
      BarColourScheme = BarColourWinampFire;
      break;
    case BAR_ELEVATOR:
      BarColourScheme = BarColourElevator;
      break;
    case BAR_CLASSIC:
    default:
      BarColourScheme = BarColourClassic;
  }

  switch(peakcolourstyle) {
    case PEAK_LEVEL:
      PeakColourScheme = PeakColourLevel;
      break;
    case PEAK_LEVELFADE:
      PeakColourScheme = PeakColourLevelFade;
      break;
    case PEAK_FADE:
    default:
      PeakColourScheme = PeakColourFade;
  }

  switch(peakleveleffect) {
    case PEAK_EFFECT_FALL:
      PeakLevelEffect = PeakLevelFall;
      break;
    case PEAK_EFFECT_RISE:
      PeakLevelEffect = PeakLevelRise;
      break;
    case PEAK_EFFECT_FALLANDRISE:
      PeakLevelEffect = PeakLevelFallAndRise;
      break;
    case PEAK_EFFECT_RISEFALL:
      PeakLevelEffect = PeakLevelRiseFall;
      break;
    case PEAK_EFFECT_SPARKS:
      PeakLevelEffect = PeakLevelSparks;
      break;
    case PEAK_EFFECT_NORMAL:
    default:
      PeakLevelEffect = PeakLevelNormal;
  }

	CalculateAuxColours();
}

void CalculateFFTVariables()
{
	unsigned int bar_usage = mono ? bands : (bands + 1) / 2;
	unsigned int nNewFft = 512;
	while(nNewFft < 0x10000 && bar_usage > nNewFft / 2)
		nNewFft <<= 1;
	
	// always re-init FFT since profile settings may have changed
	FFTInit(nNewFft);

	if(!mono) {
		// TODO: sample rate should update for each song change
		LogBarValueTable(nFftFrequencies, 44100, 16000, bands / 2, naBarTable);
		LogBarValueTable(nFftFrequencies, 44100, 16000, bands - (bands / 2), &naBarTable[bands / 2]);
	} else
		LogBarValueTable(nFftFrequencies, 44100, 16000, bands, naBarTable);

#ifdef _DEBUG
	if(!mono) {
		unsigned int nBinCount = 0;
		for(int i = 0; i < bands / 2; i++)
			nBinCount += naBarTable[i];
		_ASSERT(nBinCount == nFftFrequencies);
		nBinCount = 0;
		for(int i = bands / 2; i < bands; i++)
			nBinCount += naBarTable[i];
		_ASSERT(nBinCount == nFftFrequencies);
	} else {
		unsigned int nBinCount = 0;
		for(int i = 0; i < bands; i++)
			nBinCount += naBarTable[i];
		_ASSERT(nBinCount == nFftFrequencies);
	}
#endif
}

// recalculates commonly used variables (stereo module)
void CalculateVariablesStereo()
{
	static int old_draw_height = 0;
	static int old_peakchangerate = 0;
	static int old_band_width = 0;
	static int old_x_spacing = 0;
	static int old_y_spacing = 0;

	draw_width = win_width;
	if(draw_width > image_width)
		draw_width = image_width;
	if (draw_width <= 0)
		return;

	draw_height = win_height;
	if(draw_height > MAX_WIN_HEIGHT)
		draw_height = MAX_WIN_HEIGHT;
	if (draw_height <= 0)
		return;

	band_width = requested_band_width;  // bar width
	total_width = band_width + x_spacing;  // this is commonly used
	// bands = number of complete bars that can be drawn
	bands = (draw_width + x_spacing) / total_width;  // number of bars
	if(bands > MAX_BARS)
		bands = MAX_BARS;
	if(bands == MAX_BARS) {
		// calculate bar width to fit MAX_BARS in the draw width
		band_width = (draw_width - (MAX_BARS - 1) * x_spacing) / MAX_BARS;
		total_width = band_width + x_spacing;
		bands = (draw_width + x_spacing) / total_width;  // number of bars
		if(bands > MAX_BARS)
			bands = MAX_BARS;
		if((band_width + x_spacing) * bands < draw_width - band_width)  // adjust for optimal fill
			bands = (draw_width + x_spacing) / (++band_width + x_spacing);  // by increasing band_width
		total_width = band_width + x_spacing;
	}

	// after bars have been figured out, calculate the FFT
	CalculateFFTVariables();

	height_scale = 255.0 / (draw_height - 1);
	draw_height_scaler = height_scale - 0.0001;  // adjusts for rounding errors
	level_height_scaler = (float)(255.0 / draw_height); // scale level height to the draw height
	peak_fade_scaler = 255.0 / peakchangerate;

	// center the bars in the window
	draw_width = bands * total_width - x_spacing;
	draw_x_start = (win_width - draw_width) / 2;
	draw_y_start = (win_height - draw_height) / 2;

	// calculation is time consuming, so only perform it when height changes
	// this lets the other controls change closer to real-time.
	if(draw_height != old_draw_height) {
		old_draw_height = draw_height;
		UpdateColourLookup();
		ClearBackground();
		RedrawWindow(hatan, NULL, NULL, RDW_INVALIDATE);
	}

	// calculation is time consuming, so only perform it when peak change changes
	// this lets the other controls change closer to real-time.
	if(peakchangerate != old_peakchangerate) {
		old_peakchangerate = peakchangerate;
		UpdatePeakColourLookup();
	}

	// check if the background should get cleared
	if(band_width != old_band_width || x_spacing != old_x_spacing || y_spacing != old_y_spacing) {
		old_band_width = band_width;
		old_x_spacing = x_spacing;
		old_y_spacing = y_spacing;
		ClearBackground();
	}

	// select levebase function
	switch(levelbase) {
	case LEVEL_AVERAGE:
		LevelCalcStereo = AverageLevelCalcStereo;
		LevelCalcMono = AverageLevelCalcMono;
		break;
	case LEVEL_UNION:
	default:
		LevelCalcStereo = UnionLevelCalcStereo;
		LevelCalcMono = UnionLevelCalcMono;
	}

	CalculateCommonVariables();
}

void CalculateAndUpdate(void)
{
	CalculateVariables();
	UpdateColourLookup();
	UpdatePeakColourLookup();
	EraseWindow();
	ClearBackground();
	BackgroundDraw(0);
	if (IsWindow(hatan))
		InvalidateRect(hatan, NULL, TRUE);
}

void UpdateColourLookup()
{
	// calculate the colour lookup table for the frequncy bars
	if(colour_lookup[1]) {  // check to make sure memory is allocated
		for(int i = 1; i < 256; i++) {
			//for(int y = 0; y < (int)(i / (255.0f / draw_height)); y += y_spacing)
			//for(int y = 0; y < (int)(i / (255.0f / draw_height)); y++)
			for(int y = 0; y < (int)(i / level_height_scaler); y++) {
				//colour_lookup[i][y] = (unsigned char)((y * height_scale) + 255 - i); // Winamp fire style;
				//colour_lookup[i][y] = (unsigned char)((y * height_scale) * 255 / i);  // Faded Bars - my fire
				//colour_lookup[i][y] = (unsigned char)i;  // whole line fading
				//colour_lookup[i][y] = (unsigned char)(y * height_scale);  // classic
				// the +0.0035 corrects for any rounding errors.
				_ASSERT(y < i);
				colour_lookup[i][y] = BarColourScheme(i, (int)(y * (height_scale + 0.0035)));
			}
		}
	}
}

void UpdatePeakColourLookup()
{
  if(peakchangerate) {
    //int level_dur, fall_speed = 5;
    PeakLevelEffect();  // calculate Peak leveling effect table
    //for(int level = 1; level < 256; level++) {
      // makes the peaks fall
      /*if(peakchangerate < 256 - 255 / fall_speed)
        level_dur = peakchangerate;
      else
        level_dur = 255 - level / fall_speed;

      peak_level_length[level] = (short)(level_dur + level / fall_speed);

      for(int i = 0; i < level_dur; i++)
        peak_level_lookup[level][peak_level_length[level] - i] = (short)level;

      for(int i = level_dur, y = level - fall_speed; y >= 0; i++, y -= fall_speed)
        peak_level_lookup[level][peak_level_length[level] - i] = (short)y;
      */
      // makes the peaks rise
      /*if(peakchangerate < 256 - 255 / fall_speed)
        level_dur = peakchangerate;
      else
        level_dur = 255 - (255 - level) / fall_speed;

      peak_level_length[level] = (short)(level_dur + (255 - level) / fall_speed);

      for(int i = 0; i <= level_dur; i++)
        peak_level_lookup[level][peak_level_length[level] - i] = (short)level;

      for(int i = level_dur + 1, y = level + fall_speed; i <= peak_level_length[level]; i++, y += fall_speed)
        peak_level_lookup[level][peak_level_length[level] - i] = (short)y;
      */
      // normal, no movement
      //peak_level_length[level] = (short)peakchangerate;
      //for(int i = 0; i <= peakchangerate; i++)
      //  peak_level_lookup[level][i] = (short)level;
    //}

    // calculate the colour lookup table for the peaks
    if(peak_lookup[1]) {  // check to make sure memory is allocated
      for(int i = 1; i < 256; i++) {
        const int length = peak_level_length[i];
        for(int y = 0; y <= length; y++) {
          peak_lookup[i][y] = PeakColourScheme(peak_level_lookup[i][y], (int)(y * 255.0 / length));
        }
      }
    }
  }

  // calculate the colour lookup table for the peaks
  //if(peak_lookup[1])  // check to make sure memory is allocated
  //  for(int i = 1; i < 256; i++)
  //    //for(int y = 0; y < (int)(i / (255.0f / draw_height)); y += y_spacing)
  //    for(int y = 0; y <= peakchangerate; y++)
  //      peak_lookup[i][y] = PeakColourScheme(i, (int)(y * peak_fade_scaler));

}

void RandomColourLookup()
{
  unsigned char (*Scheme)(int level, int y);
  // calculate the colour lookup table for the frequncy bars
  if(colour_lookup[1]) {  // check to make sure memory is allocated
    for(int i = 1; i < 256; i++) {
      switch(m_rand() % 5) {
        case 4:
          Scheme = BarColourElevator;
          break;
        case 3:
          Scheme = BarColourClassic;
          break;
        case 2:
          Scheme = BarColourFire;
          break;
        case 1:
          Scheme = BarColourLines;
          break;
        default:
          Scheme = BarColourWinampFire;
          break;
      }
      //for(int y = 0; y < (int)(i / (255.0f / draw_height)); y++)
	  for(int y = 0; y < (int)(i / level_height_scaler); y++) {
	    _ASSERT(y < i);
        colour_lookup[i][y] = Scheme(i, (int)(y * height_scale));
	  }
    }
  }
}

// various default settings
void DefaultSettings()
{
	// profile name
	wcsncpy(szCurrentProfile, cszDefaultSettingsName, ARRAYSIZE(szCurrentProfile));

	// set colours
	for(int i = 0; i < 256; i++) {
		FreqBarColour[i] = bmpRGB(204 + i/5, i, 0);
		VolumeColour[i] = bmpRGB(i/1.8, i/1.6, i/1.45);
		if(i < 128)
			PeakColour[i] = bmpRGB(92 + (int)(i * 1.2835), i * 2, i * 2);
		else
			PeakColour[i] = bmpRGB(255,255,255);
	}

	// set up level functions... er lookup tables
	for(int i = 0; i < 256; i++) {
		//level_func[i] = (int)(log10(1.0 + (double)i / 28.3334) * 256.0); // log base 10
		volume_func[i] = 256 - 256 / (i + 1);  // 1/x function
	}

	falloffrate = 12; // falloff rate for frequency level bars
	peakchangerate = 80;  // changerate for peak level indicators val 1 to 255
	requested_band_width = 3; // thickness of bands in pixels
	x_spacing = 1;    // spacing between bands in pixels, lowest val = 0
	y_spacing = 2;  // lowest value = 1
	reverseleft = true;
	reverseright = false;
	mono = true;
	backgrounddraw = BACKGROUND_BLACK;
	barcolourstyle = BAR_FIRE;
	peakcolourstyle = PEAK_FADE;
	effect = EFFECT_FADESHADOW;
	peakleveleffect = PEAK_EFFECT_FALL;
	peakfalloffmode = 0;
	levelbase = LEVEL_AVERAGE;
	bFftEqualize = true;
	fFftEnvelope = 0.2f;
	fFftScale = 2.0f;
}

void EraseWindow(HDC hdc)
{
    if (hdc != NULL) {
        static HDC cache_dc = NULL;
        static HBITMAP cache_bmp = NULL;
        static COLORREF last_colour = RGB(0, 0, 0);
        static int last_win_width = -1, last_win_height = -1;

        COLORREF colour = RGB(0, 0, 0);
        if ((backgrounddraw == BACKGROUND_FLASH) ||
            (backgrounddraw == BACKGROUND_SOLID))
            colour = FixCOLORREF(VolumeColour[0]);

        // try to minimise regenerating the brush if
        // it's not been changed since the last call
        if ((last_colour != colour) ||
            (last_win_width != win_width) || 
            (last_win_height != win_height)) {
            last_colour = colour;

            if (cache_bmp != NULL) {
                DeleteObject(cache_bmp);
                cache_bmp = NULL;
            }

            if (cache_dc != NULL) {
                DeleteDC(cache_dc);
                cache_dc = NULL;
            }

            last_win_width = win_width;
            last_win_height = win_height;
        }

        if (cache_dc == NULL) {
            cache_dc = CreateCompatibleDC(hdc);
            if (cache_dc != NULL) {
                cache_bmp = CreateCompatibleBitmap(hdc, win_width, win_height);
                DeleteObject(SelectObject(cache_dc, cache_bmp));

                const int old_bk_colour = SetBkColor(cache_dc, colour);

                // top
                RECT r = { 0, 0, win_width, draw_y_start };
                ExtTextOut(cache_dc, 0, 0, ETO_OPAQUE, &r, NULL, 0, 0);

                // left
                r.top = r.bottom;
                r.bottom += draw_height;
                r.right = draw_x_start;
                ExtTextOut(cache_dc, 0, 0, ETO_OPAQUE, &r, NULL, 0, 0);

                // right
                r.left = r.right + draw_width;
                r.right = win_width;
                ExtTextOut(cache_dc, 0, 0, ETO_OPAQUE, &r, NULL, 0, 0);

                // bottom
                r.left = 0;
                r.top += draw_height;
                r.bottom = win_height;
                ExtTextOut(cache_dc, 0, 0, ETO_OPAQUE, &r, NULL, 0, 0);

                SetBkColor(cache_dc, old_bk_colour);
            }
        }

        if (cache_dc != NULL) {
            BitBlt(hdc, 0, 0, win_width, win_height, cache_dc, 0, 0, SRCCOPY);
        }
    }
}

void EraseWindow(void)
{
    if(IsWindow(hatan)) {
	    HDC hdc = GetDC(hatan);
	    EraseWindow(hdc);
	    ReleaseDC(hatan, hdc);
    }
}

// clear the background with zeros
void BackgroundBlack(unsigned char)
{
  if(rgbbuffer) {
    ZeroMemory(&rgbbuffer[0], draw_width * 4);
    for(int y = image_width * y_spacing; y < draw_height * image_width; y += image_width * y_spacing)
      ZeroMemory(&rgbbuffer[y], draw_width * 4);
  }
}

// attempt at a double buffer clearing for speed.  Not much faster than the
// above method.
void BackgroundBlackFast(unsigned char)
{
  if(rgbbuffer) {
    for(int bar = 0; bar < bands; bar++) {
      for(int y = (int)(level[bar] / level_height_scaler); y <= (int)(levelbuffer[1][bar] / level_height_scaler); y++)
        ZeroMemory(&rgbbuffer[y * image_width + bar * total_width], band_width * 4);
      //if(peaklevelbuffer[1][bar])
        ZeroMemory(&rgbbuffer[peaklevelbuffer[1][bar] * image_width + bar * total_width], band_width * 4);
    }
  }

  for(int i = 0; i < bands; i++) {
    levelbuffer[1][i] = level[i];
    peaklevelbuffer[1][i] = peaklevel[i];
  }

}

// clear the background drawing a grid based on background flash colours
void BackgroundFade(unsigned char)
{
  if(rgbbuffer) {
    for(int y = 0; y < draw_height; y += y_spacing) {
      // +0.0035 is used to correct for rounding errors
      int colouridx = (int)(y * (height_scale + 0.0035));
      for(int x = 0; x < draw_width; x += x_spacing + band_width) {
        for(int i = 0; i < band_width; i++)
          rgbbuffer[y * image_width + x + i] = VolumeColour[colouridx];
      }
      // no need to draw the inbetween colour (black) now
      //for(x = band_width; x < draw_width; x += x_spacing + band_width) {
      //  for(i = 0; i < x_spacing; i++)
      //    rgbbuffer[y * 576 + x + i] = 0;
      //}
    }
  }
}

// clear the background drawing a grid based on background flash colours
// for the mirror effect
void BackgroundFadeMirror(unsigned char)
{
  if(rgbbuffer) {
    const int middle = draw_height / 2 / y_spacing * y_spacing;

    for(int y = 0; y <= draw_height / 2; y += y_spacing) {
      // +0.0035 is used to correct for rounding errors
      //colouridx = (int)(y * 1.90f * (height_scale - 0.0035f));
      const int colouridx = (int)(y * 2.0 * level_height_scaler);
      for(int x = 0; x < draw_width; x += x_spacing + band_width) {
        for(int i = 0; i < band_width; i++) {
          rgbbuffer[(middle + y) * image_width + x + i] = VolumeColour[colouridx];
          rgbbuffer[(middle - y) * image_width + x + i] = VolumeColour[colouridx];
        }
      }
    }
  }
}

// clear the background drawing a grid based on background flash colours
// this is for the Reflection style
void BackgroundFadeReflection(unsigned char)
{
  if (rgbbuffer) {
    const int middle = draw_height / 3 / y_spacing * y_spacing;

    // draw top part of background
    for(int y = middle; y < draw_height; y += y_spacing) {
      // +0.0035 is used to correct for rounding errors
      const int colouridx = (int)((y - middle) * 1.49 * (height_scale + 0.0035));
      const COLORREF colour = VolumeColour[colouridx];
      for(int x = 0; x < draw_width; x += x_spacing + band_width) {
        for(int i = 0; i < band_width; i++)
          rgbbuffer[y * image_width + x + i] = colour;
          //rgbbuffer[(middle - y) * image_width + x + i] = colour;
      }
    }

    // now draw bottom (reflection) part
    for(int y = middle; y >= 0; y -= y_spacing) {
      // +0.0035 is used to correct for rounding errors
      const int colouridx = (int)((middle - y) * 2.975 * (height_scale + 0.0035));
      const COLORREF colour = bmpRGB(GetBValue(VolumeColour[colouridx]) / 1.45, GetGValue(VolumeColour[colouridx]) / 1.45, GetRValue(VolumeColour[colouridx]) / 1.45);
      for(int x = 0; x < draw_width; x += x_spacing + band_width) {
        for(int i = 0; i < band_width; i++)
          rgbbuffer[y * image_width + x + i] = colour;
          //rgbbuffer[(middle - y) * image_width + x + i] = colour;
      }
    }
  }
}

// clear the background using the volume to determine the colour
void BackgroundFlash(unsigned char volume)
{
  if (rgbbuffer) {
    // for loop and CopyMemory blank first line...
    for(int x = 0; x < draw_width; x++)
      rgbbuffer[x] = VolumeColour[volume];
      //CopyMemory(&rgbbuffer[draw_width / 16], &rgbbuffer[0], draw_width / 4);
      //CopyMemory(&rgbbuffer[draw_width / 8], &rgbbuffer[0], draw_width / 2);
      //CopyMemory(&rgbbuffer[draw_width / 4], &rgbbuffer[0], draw_width);
      //CopyMemory(&rgbbuffer[draw_width / 2], &rgbbuffer[0], draw_width * 2 + draw_width % 4);

      // ...now copy the first line to the rest of the lines
      for(int y = image_width; y < draw_height * image_width; y += image_width)
        CopyMemory(&rgbbuffer[y], &rgbbuffer[0], draw_width * 4);
  }
}

// clear the background drawing a grid based on dim Freq Bar colours
void BackgroundGrid(unsigned char)
{
  if (rgbbuffer) {
    for(int y = 0; y < draw_height; y += y_spacing) {
      // +0.0035 is used to correct for rounding errors
      const int colouridx = (int)(y * (height_scale + 0.0035));
      const COLORREF colour = bmpRGB(GetBValue(FreqBarColour[colouridx]) / 3, GetGValue(FreqBarColour[colouridx]) / 3, GetRValue(FreqBarColour[colouridx]) / 3);
      for(int x = 0; x < draw_width; x += x_spacing + band_width) {
        for(int i = 0; i < band_width; i++)
          rgbbuffer[y * image_width + x + i] = colour;
      }
      // no need to draw the inbetween colour (black) now
      //for(x = band_width; x < draw_width; x += x_spacing + band_width) {
      //  for(i = 0; i < x_spacing; i++)
      //    rgbbuffer[y * 576 + x + i] = 0;
      //}
    }
  }
}

// clear the background drawing a grid based on dim Freq Bar colours
// this is for the Mirror style
void BackgroundGridMirror(unsigned char)
{
  if (rgbbuffer) {
    const int middle = draw_height / 2 / y_spacing * y_spacing;

    for(int y = 0; y <= draw_height / 2; y += y_spacing) {
      // +0.0035 is used to correct for rounding errors
      //colouridx = (int)(y * 1.97f * (height_scale + 0.0035f));
      const int colouridx = (int)(y * 2.0 * level_height_scaler);
      const COLORREF colour = bmpRGB(GetBValue(FreqBarColour[colouridx]) / 3, GetGValue(FreqBarColour[colouridx]) / 3, GetRValue(FreqBarColour[colouridx]) / 3);
      for(int x = 0; x < draw_width; x += x_spacing + band_width) {
        for(int i = 0; i < band_width; i++) {
          rgbbuffer[(middle + y) * image_width + x + i] = colour;
          rgbbuffer[(middle - y) * image_width + x + i] = colour;
        }
      }
    }
  }
}

// clear the background drawing a grid based on dim Freq Bar colours
// this is for the Reflection style
void BackgroundGridReflection(unsigned char)
{
  if (rgbbuffer) {
    const int middle = draw_height / 3 / y_spacing * y_spacing;

    // draw top part of background
    for(int y = middle; y < draw_height; y += y_spacing) {
      // +0.0035 is used to correct for rounding errors
      const int colouridx = (int)((y - middle) * 1.49 * (height_scale + 0.0035));
      const COLORREF colour = bmpRGB(GetBValue(FreqBarColour[colouridx]) / 3, GetGValue(FreqBarColour[colouridx]) / 3, GetRValue(FreqBarColour[colouridx]) / 3);
      for(int x = 0; x < draw_width; x += x_spacing + band_width) {
        for(int i = 0; i < band_width; i++)
          rgbbuffer[y * image_width + x + i] = colour;
        //rgbbuffer[(middle - y) * image_width + x + i] = colour;
      }
    }

    // now draw bottom (reflection) part
    for(int y = middle; y >= 0; y -= y_spacing) {
      // +0.0035 is used to correct for rounding errors
      const int colouridx = (int)((middle - y) * 2.975 * (height_scale + 0.0035));
      const COLORREF colour = bmpRGB(GetBValue(FreqBarColour[colouridx]) / 4.5, GetGValue(FreqBarColour[colouridx]) / 4.5, GetRValue(FreqBarColour[colouridx]) / 4.5);
      for(int x = 0; x < draw_width; x += x_spacing + band_width) {
        for(int i = 0; i < band_width; i++)
          rgbbuffer[y * image_width + x + i] = colour;
          //rgbbuffer[(middle - y) * image_width + x + i] = colour;
      }
    }
  }
}

// clear the background with volume colour 0
void BackgroundSolid(unsigned char)
{
  if (rgbbuffer) {
    for(int x = 0; x < draw_width; x++)
      rgbbuffer[x] = VolumeColour[0];

    //CopyMemory(&rgbbuffer[draw_width / 16], &rgbbuffer[0], draw_width / 4);
    //CopyMemory(&rgbbuffer[draw_width / 8], &rgbbuffer[0], draw_width / 2);
    //CopyMemory(&rgbbuffer[draw_width / 4], &rgbbuffer[0], draw_width);
    //CopyMemory(&rgbbuffer[draw_width / 2], &rgbbuffer[0], draw_width * 2 + draw_width % 4);
    for(int y = image_width; y < draw_height * image_width; y += image_width)
      CopyMemory(&rgbbuffer[y], &rgbbuffer[0], draw_width * 4);
  }
}

void RenderSingleBars()
{
  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      //for(int y = 0; y < (int)(level[i] / (255.0f / (draw_height / y_spacing))); y++) {
      //for(int y = 0; y * height_scale < (int)(level[i]); y++) {
      //for(y = 0; y < (int)(level[i] / height_scale); y += y_spacing)
      //for(y = 0; y < (int)(level[i] / (255.0f / draw_height)); y += y_spacing)
	  for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        rgbbuffer[y * image_width + x] = FreqBarColour[colour_lookup[level[i]][y]];
	  }

      if(peaklevel[i])
        rgbbuffer[(int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
    }
  }
}

void RenderWideBars()
{
  if (rgbbuffer) {
    //static int trippycolourbase = 0, trippycolour;
    //trippycolour = trippycolourbase;

    for(int i = 0; i < bands; i++) {
      //for(x = i * (band_width + x_spacing); x < i * (band_width + x_spacing) + band_width; x++) {
      int x = i * total_width;
      const int x_stop = i * total_width + band_width;
      //for(int y = 0; y < (int)(level[i] / (255.0f / (draw_height / y_spacing))); y++) {
      //for(int y = 0; y * height_scale < (int)(level[i]); y++) {
      //for(y = 0; y < (int)(level[i] / (255.0f / draw_height)); y += y_spacing)
      for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        const COLORREF copycolour = FreqBarColour[colour_lookup[level[i]][y]];
        const int temp = y * image_width;
        for(int tx = x; tx < x_stop; tx++)
          rgbbuffer[temp + tx] = copycolour;
      }
      //trippycolour = (trippycolour + (576 / bands)) % 576;

      //for(tx = x + 1; tx < i * (band_width + x_spacing) + band_width; tx++)
      //  for(y = 0; y < (int)(level[i] / (255.0f / draw_height)); y += y_spacing)
      //    rgbbuffer[y * 576 + tx] = rgbbuffer[y * 576 + x];
      //    //CopyMemory(&rgbbuffer[y * 576 + tx], &rgbbuffer[y * 576 + x], 4);
      // reordering the for loops made a big difference in performance (suspect better cache hit)
      //for(y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
      //  copycolour = rgbbuffer[y * image_width + x];
      //  for(tx = x + 1; tx < i * (band_width + x_spacing) + band_width; tx++)
      //    rgbbuffer[y * image_width + tx] = copycolour;
      //    //CopyMemory(&rgbbuffer[y * 576 + tx], &rgbbuffer[y * 576 + x], 4);
      //}

      if(peaklevel[i]) {
        const COLORREF copycolour = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        const int temp = (int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width;
        do{
          //rgbbuffer[(int)(peaklevel[i] / height_scale / y_spacing) * y_spacing * 576 + x] = PeakColour[peak_lookup[peaklevel[i]][peakchange[i]]];
          rgbbuffer[temp + x] = copycolour;
          //rgbbuffer[(int)(peaklevel[i] / height_scale / y_spacing) * y_spacing * 576 + x] = FreqBarColour[peaklevel[i]]; // use Frequency Bar colour
          //rgbbuffer[(int)(peaklevel[i] / height_scale / y_spacing) * y_spacing * 576 + x] = FreqBarColour[(int)(peakchange[i] * peak_fade_scaler)]; // fade-out Frequency Bar colour
        }while(++x < x_stop);
      }
    }

    //++trippycolourbase %= 576;
  }
}

void RenderSingleBarsMirror()
{
  if (rgbbuffer) {
    const int middle = (draw_height / 2 / y_spacing) * y_spacing;
    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      //for(int y = 0; y < (int)(level[i] / (255.0f / (draw_height / y_spacing))); y++) {
      //for(int y = 0; y * height_scale < (int)(level[i]); y++) {
      //for(y = 0; y < (int)(level[i] / height_scale); y += y_spacing)
      //for(y = 0; y < (int)(level[i] / (255.0f / draw_height)); y += y_spacing)
      for(int y = 0; y < (int)(level[i] / level_height_scaler) / 2; y += y_spacing) {
	    _ASSERT(y * 2 < 255);
        rgbbuffer[(middle + y) * image_width + x] = FreqBarColour[colour_lookup[level[i]][y * 2]];
        rgbbuffer[(middle - y) * image_width + x] = FreqBarColour[colour_lookup[level[i]][y * 2]];
      }

      //temp = (middle + ((int)(peaklevel[i] / draw_height_scaler) / 2 / y_spacing) * y_spacing) * image_width;
      if(peaklevel[i]) {
        rgbbuffer[(middle + ((int)(peaklevel[i] / draw_height_scaler) / 2 / y_spacing) * y_spacing) * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        rgbbuffer[(middle - ((int)(peaklevel[i] / draw_height_scaler) / 2 / y_spacing) * y_spacing) * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
      }
    }
  }
}

void RenderWideBarsMirror()
{
  if (rgbbuffer) {
    //static int trippycolourbase = 0, trippycolour;
    const int middle = (draw_height / 2 / y_spacing) * y_spacing;
    int temp, temp2;
    //trippycolour = trippycolourbase;

    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      const int x_stop = i * total_width + band_width;
      for(int y = 0; y < (int)(level[i] / level_height_scaler) / 2; y += y_spacing) {
	    _ASSERT(y * 2 < 255);
        const COLORREF copycolour = FreqBarColour[colour_lookup[level[i]][y * 2]];
        temp = (middle + y) * image_width;
        temp2 = (middle - y) * image_width;
        for(int tx = x; tx < x_stop; tx++) {
          rgbbuffer[temp + tx] = copycolour;
          rgbbuffer[temp2 + tx] = copycolour;
        }
      }
      //trippycolour = (trippycolour + (576 / bands)) % 576;

      //for(tx = x + 1; tx < i * (band_width + x_spacing) + band_width; tx++)
      //  for(y = 0; y < (int)(level[i] / (255.0f / draw_height)); y += y_spacing)
      //    rgbbuffer[y * 576 + tx] = rgbbuffer[y * 576 + x];
      //    //CopyMemory(&rgbbuffer[y * 576 + tx], &rgbbuffer[y * 576 + x], 4);
      // reordering the for loops made a big difference in performance (suspect better cache hit)
      //for(y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
      //  copycolour = rgbbuffer[y * image_width + x];
      //  for(tx = x + 1; tx < i * (band_width + x_spacing) + band_width; tx++)
      //    rgbbuffer[y * image_width + tx] = copycolour;
      //    //CopyMemory(&rgbbuffer[y * 576 + tx], &rgbbuffer[y * 576 + x], 4);
      //}

      if(peaklevel[i]) {
        const COLORREF copycolour = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        temp = (middle + ((int)(peaklevel[i] / draw_height_scaler) / 2 / y_spacing) * y_spacing) * image_width;
        temp2 = (middle - ((int)(peaklevel[i] / draw_height_scaler) / 2 / y_spacing) * y_spacing) * image_width;
        do{
          //rgbbuffer[(int)(peaklevel[i] / height_scale / y_spacing) * y_spacing * 576 + x] = PeakColour[peak_lookup[peaklevel[i]][peakchange[i]]];
          rgbbuffer[temp + x] = copycolour;
          rgbbuffer[temp2 + x] = copycolour;
          //rgbbuffer[(int)(peaklevel[i] / height_scale / y_spacing) * y_spacing * 576 + x] = FreqBarColour[peaklevel[i]]; // use Frequency Bar colour
          //rgbbuffer[(int)(peaklevel[i] / height_scale / y_spacing) * y_spacing * 576 + x] = FreqBarColour[(int)(peakchange[i] * peak_fade_scaler)]; // fade-out Frequency Bar colour
        }while(++x < x_stop);
      }
    }

    //++trippycolourbase %= 576;
  }
}

void CalculateDarkMirrorAuxColours(void)
{
  for(int i = 0; i < 256; i++) {
    AuxColour[0][i] = bmpRGB(GetBValue(FreqBarColour[i]) / 1.75, GetGValue(FreqBarColour[i]) / 1.75, GetRValue(FreqBarColour[i]) / 1.75);
    AuxColour[1][i] = bmpRGB(GetBValue(PeakColour[i]) / 1.75, GetGValue(PeakColour[i]) / 1.75, GetRValue(PeakColour[i]) / 1.75);
  }
}

void RenderSingleBarsDarkMirror()
{
  if (rgbbuffer) {
    const int middle = (draw_height / 3 / y_spacing) * y_spacing;

    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      for(int y = 0; y < (int)(level[i] / level_height_scaler) * 2 / 3; y += y_spacing) {
        _ASSERT(y * 3 / 2 < 255);
        rgbbuffer[(middle - y / 2) / y_spacing * y_spacing * image_width + x] = AuxColour[0][colour_lookup[level[i]][y * 3 / 2]];
        rgbbuffer[(middle + y) * image_width + x] = FreqBarColour[colour_lookup[level[i]][y * 3 / 2]];
      }

      if(peaklevel[i]) {
        rgbbuffer[(middle - (int)(peaklevel[i] / level_height_scaler) * 2 / 6) / y_spacing * y_spacing * image_width + x] = AuxColour[1][peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        //rgbbuffer[(middle - ((int)(peaklevel[i] / level_height_scaler) * 2 / 6) / y_spacing * y_spacing) * image_width + x] = AuxColour[1][peak_lookup[peaklevel[i]][peakchange[i]]];
        rgbbuffer[(middle + ((int)(peaklevel[i] / draw_height_scaler) * 2 / 3) / y_spacing * y_spacing) * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
      }
    }
  }
}

void RenderWideBarsDarkMirror()
{
  if (rgbbuffer) {
    // note that middle is actually 1/3 of the screen.  The dark reflection is in the bottom
    // 1/3 of the screen and the top 2/3 is the full colour bars.
    const int middle = (draw_height / 3 / y_spacing) * y_spacing;

    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      const int x_stop = i * total_width + band_width;
      for(int y = 0; y < (int)(level[i] / level_height_scaler) * 2 / 3; y += y_spacing) {
	    _ASSERT(y * 3 / 2 < 255);
        const COLORREF copycolour = FreqBarColour[colour_lookup[level[i]][y * 3 / 2]];
        const COLORREF copycolour2 = AuxColour[0][colour_lookup[level[i]][y * 3 / 2]];
        int temp = (middle + y) * image_width;
        int temp2 = (middle - y / 2) / y_spacing * y_spacing * image_width;
        for(int tx = x; tx < x_stop; tx++) {
          rgbbuffer[temp2 + tx] = copycolour2;
          rgbbuffer[temp + tx] = copycolour;
        }
      }

      if(peaklevel[i]) {
        const COLORREF copycolour = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        const COLORREF copycolour2 = AuxColour[1][peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        int temp = (middle + ((int)(peaklevel[i] / draw_height_scaler) * 2 / 3) / y_spacing * y_spacing) * image_width;
        // note: *2/6 is used as a reduced *2/3*2 in order to calculate the location for the peak
        // if reduced to just /3 then with some window heights the bars will be drawn higher than
        // the peaks due to rounding errors.
        int temp2 = (middle - (int)(peaklevel[i] / level_height_scaler) * 2 / 6) / y_spacing * y_spacing * image_width;
        do{
          rgbbuffer[temp2 + x] = copycolour2;
          rgbbuffer[temp + x] = copycolour;
        }while(++x < x_stop);
      }
    }
  }
}

void CalculateWaveyReflectionAuxColours(void)
{
	for(int i = 0; i < 256; i++) {
		AuxColour[0][i] = bmpRGB(GetBValue(FreqBarColour[i]) / 1.65, GetGValue(FreqBarColour[i]) / 1.65, GetRValue(FreqBarColour[i]) / 1.65);
		//AuxColour[1][i] = bmpRGB(GetBValue(FreqBarColour[i]) / (1.85f + volume / 100.0f), GetGValue(FreqBarColour[i]) / (1.85f + volume / 100.0f), GetRValue(FreqBarColour[i]) / (1.85f + volume / 100.0f));
		AuxColour[2][i] = bmpRGB(GetBValue(PeakColour[i]) / 1.65, GetGValue(PeakColour[i]) / 1.65, GetRValue(PeakColour[i]) / 1.65);
	}
}

void RenderSingleBarsWaveyReflection()
{
  // wavedivisor - overall amplitude of the wave
  //static int waves[18] = {0,0,0,1,2,2,2,2,1,0,0,0,-1,-2,-2,-2,-2,-1};
  static const int waves[16] = {0,0,1,2,2,2,2,1,0,0,-1,-2,-2,-2,-2,-1};
  //static int waves[20] = {0,0,0,1,1,1,1,1,1,1,0,0,0,-1,-1,-1,-1,-1,-1,-1};
  static const int wavechange[4] = {0,18,32,0};
  static int wavestart = 2, wavedivisor = 3, oldvolume = 0; //, wavedelay = 0;
  bool draw_y = true;
  int middle = (draw_height / 3 / y_spacing) * y_spacing, volume = 0;

  //if(!(++wavedelay %= 5))
    //if(!(wavestart--))
    wavestart -= 2;
    if(!wavestart)
      wavestart = 14;

  for(int i = 0; i < bands; i += 2) // calculate the volume to see if wave
    volume += level[i];             // amplitude should change
  volume /= bands / 2;

  if(volume > oldvolume + 5) {  // if volume increases enough
    if(wavedivisor > 1)         // and divisor is not at 1, lower it
      wavedivisor--;
    oldvolume = volume;
  }
  else {  // otherwise see if volume decreased enough to be changed
    if(volume < oldvolume - wavechange[wavedivisor]) {
      if(wavedivisor < 3)
        wavedivisor++;
      oldvolume = volume;
    }
  }

  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      int wave = wavestart;   // restart the wave drawing offset
      for(int y = 0; y < (int)(level[i] / level_height_scaler) * 2 / 3; y += y_spacing) {
	    _ASSERT(y * 3 / 2 < 255);
	    if(x > 1 && x < draw_width - 2 && y / 2 > 0)
          rgbbuffer[(middle - y / 2) / y_spacing * y_spacing * image_width + x + waves[wave] / wavedivisor] = AuxColour[0][colour_lookup[level[i]][y * 3 / 2]];
        draw_y = !draw_y; // since y is halved, only update the wave index counter for
        if(draw_y)        // every second y value so that the full waves are always seen
          ++wave %= 16;
        rgbbuffer[(middle + y) * image_width + x] = FreqBarColour[colour_lookup[level[i]][y * 3 / 2]];
      }

      if(peaklevel[i]) {
        if(x > 1 && x < draw_width - 2) {
          wave = (peaklevel[i] / 6 + wavestart) % 16;
          rgbbuffer[(middle - (int)(peaklevel[i] / level_height_scaler) * 2 / 6) / y_spacing * y_spacing * image_width + x + waves[wave] / wavedivisor] = AuxColour[2][peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        }
        //rgbbuffer[(middle - ((int)(peaklevel[i] / level_height_scaler) * 2 / 6) / y_spacing * y_spacing) * image_width + x] = AuxColour[1][peak_lookup[peaklevel[i]][peakchange[i]]];
        rgbbuffer[(middle + ((int)(peaklevel[i] / draw_height_scaler) * 2 / 3) / y_spacing * y_spacing) * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
      }
    }
  }
}

void RenderWideBarsWaveyReflection()
{
  // note that middle is actually 1/3 of the screen.  The dark reflection is in the bottom
  // 1/3 of the screen and the top 2/3 is the full colour bars.
  static const int waves[16] = {0,0,1,2,2,2,2,1,0,0,-1,-2,-2,-2,-2,-1};
  //static int waves[20] = {0,0,0,1,1,1,1,1,1,1,0,0,0,-1,-1,-1,-1,-1,-1,-1};
  static const int wavechange[4] = {0,18,32,0};
  static int wavestart = 2, wavedivisor = 3, oldvolume = 0;
  bool draw_y = true;
  const int middle = (draw_height / 3 / y_spacing) * y_spacing;
  int volume = 0;

  //if(!(wavestart--))
  wavestart -= 2;
  if(!wavestart)
    wavestart = 14;

  for(int i = 0; i < bands; i += 2) // calculate the volume to see if wave
    volume += level[i];             // amplitude should change
  volume /= bands / 2;

  if(volume > oldvolume + 5) {  // if volume increases enough
    if(wavedivisor > 1)         // and divisor is not at 1, lower it
      wavedivisor--;
    oldvolume = volume;
  }
  else {  // otherwise see if volume decreased enough to be changed
    if(volume < oldvolume - wavechange[wavedivisor]) {
      if(wavedivisor < 3)
        wavedivisor++;
      oldvolume = volume;
    }
  }

  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      int x_stop = x + band_width;
      int wave = wavestart;
      for(int y = 0; y < (int)(level[i] / level_height_scaler) * 2 / 3; y += y_spacing) {
	    _ASSERT(y * 3 / 2 < 255);
        const COLORREF copycolour = FreqBarColour[colour_lookup[level[i]][y * 3 / 2]];
        const COLORREF copycolour2 = AuxColour[0][colour_lookup[level[i]][y * 3 / 2]];
        const int temp = (middle + y) * image_width;
        const int temp2 = (middle - y / 2) / y_spacing * y_spacing * image_width + waves[wave] / wavedivisor;
        for(int tx = x; tx < x_stop; tx++) {
          if(tx > 1 && tx < draw_width - 2 && y / 2 > 0)
            rgbbuffer[temp2 + tx] = copycolour2;
          rgbbuffer[temp + tx] = copycolour;
        }
        draw_y = !draw_y; // since y is halved, only update the wave index counter for
        if(draw_y)        // every second y value so that the full waves are always seen
          ++wave %= 16;
      }

      if(peaklevel[i]) {
        wave = (peaklevel[i] / 6 + wavestart) % 16;
        const COLORREF copycolour = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        const COLORREF copycolour2 = AuxColour[2][peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        const int temp = (middle + ((int)(peaklevel[i] / draw_height_scaler) * 2 / 3) / y_spacing * y_spacing) * image_width;
        // note: *2/6 is used as a reduced *2/3*2 in order to calculate the location for the peak
        // if reduced to just /3 then with some window heights the bars will be drawn higher than
        // the peaks due to rounding errors.
        const int temp2 = (middle - (int)(peaklevel[i] / level_height_scaler) * 2 / 6) / y_spacing * y_spacing * image_width + waves[wave] / wavedivisor;
        do{
          if(x > 1 && x < draw_width - 2)
            rgbbuffer[temp2 + x] = copycolour2;
          rgbbuffer[temp + x] = copycolour;
        }while(++x < x_stop);
      }
    }
  }
}

void CalculateShadowAuxColours(void)
{
  const unsigned int bgR = GetBValue(VolumeColour[0]);
  const unsigned int bgG = GetGValue(VolumeColour[0]);
  const unsigned int bgB = GetRValue(VolumeColour[0]);
  for(int i = 0; i < 256; i++)
    AuxColour[0][i] = bmpRGB((GetBValue(FreqBarColour[i]) + bgR) / 2, (GetGValue(FreqBarColour[i]) + bgG) / 2, (GetRValue(FreqBarColour[i]) + bgB) / 2);
}

void RenderSingleBarsShadow()
{
  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      int x = i * total_width;

      if(levelbuffer[1][i] < level[i])
        levelbuffer[1][i] = level[i];
      else
        levelbuffer[1][i]--;

	  for(int y = (int)(level[i] / level_height_scaler) / y_spacing * y_spacing; y < (int)(levelbuffer[1][i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        rgbbuffer[y * image_width + x] = AuxColour[0][colour_lookup[levelbuffer[1][i]][y]];
	  }

	  for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        rgbbuffer[y * image_width + x] = FreqBarColour[colour_lookup[level[i]][y]];
	  }

      if(peaklevel[i])
        rgbbuffer[(int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
    }
  }
}

void RenderWideBarsShadow()
{
  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      const int x_stop = i * total_width + band_width;

      if(levelbuffer[1][i] < level[i])
        levelbuffer[1][i] = level[i];
      else
        levelbuffer[1][i]--;

      for(int y = (int)(level[i] / level_height_scaler / y_spacing) * y_spacing; y < (int)(levelbuffer[1][i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        const COLORREF copycolour = AuxColour[0][colour_lookup[levelbuffer[1][i]][y]];
        for(int tx = x; tx < x_stop; tx++)
          rgbbuffer[y * image_width + tx] = copycolour;
      }

      for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        const COLORREF copycolour = FreqBarColour[colour_lookup[level[i]][y]];
        for(int tx = x; tx < x_stop; tx++)
          rgbbuffer[y * image_width + tx] = copycolour;
      }

      if(peaklevel[i]) {
        const COLORREF copycolour = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        const int temp = (int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width;
        do{
          rgbbuffer[temp + x] = copycolour;
        }while(++x < x_stop);
      }
    }
  }
}

void CalculateFadeShadowAuxColours(void)
{
    const unsigned int bgR = GetBValue(VolumeColour[0]);
    const unsigned int bgG = GetGValue(VolumeColour[0]);
    const unsigned int bgB = GetRValue(VolumeColour[0]);
	for(int i = 0; i < 128; i++) {
        const unsigned int r = GetBValue(FreqBarColour[i * 2]);
        const unsigned int g = GetGValue(FreqBarColour[i * 2]);
        const unsigned int b = GetRValue(FreqBarColour[i * 2]);
		SmallAuxColour[0][i] = bmpRGB((r + bgR * 7) / 8, (g + bgG * 7) / 8, (b + bgB * 7) / 8);
		SmallAuxColour[1][i] = bmpRGB((r + bgR * 3) / 4, (g + bgG * 3) / 4, (b + bgB * 3) / 4);
		SmallAuxColour[2][i] = bmpRGB((r * 3 + bgR * 5) / 8, (g * 3 + bgG * 5) / 8, (b * 3 + bgB * 5) / 8);
		SmallAuxColour[3][i] = bmpRGB((r + bgR) / 2, (g + bgG) / 2, (b + bgB) / 2);
		SmallAuxColour[4][i] = bmpRGB((r * 5 + bgR * 3) / 8, (g * 5 + bgG * 3) / 8, (b * 5 + bgB * 3) / 8);
		SmallAuxColour[5][i] = bmpRGB((r * 3 + bgR) / 4, (g * 3 + bgG) / 4, (b * 3 + bgB) / 4);
	}
}

void RenderSingleBarsFadeShadow()
{
  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      const int x = i * total_width;

      if(levelbuffer[1][i] < level[i] || !(levelbuffer[2][i]--)) {
        levelbuffer[1][i] = level[i];
        levelbuffer[2][i] = 23;
      }
      //else
      //  levelbuffer[1][i]--;

	  for(int y = (int)(level[i] / level_height_scaler) / y_spacing * y_spacing; y < (int)(levelbuffer[1][i] / level_height_scaler); y += y_spacing) {
 	    _ASSERT(y < 255);
        rgbbuffer[y * image_width + x] = SmallAuxColour[levelbuffer[2][i] / 4][colour_lookup[levelbuffer[1][i]][y] / 2];
	  }

	  for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        rgbbuffer[y * image_width + x] = FreqBarColour[colour_lookup[level[i]][y]];
	  }

      if(peaklevel[i])
        rgbbuffer[(int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
    }
  }
}

void RenderWideBarsFadeShadow()
{
  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      const int x_stop = i * total_width + band_width;

      if(levelbuffer[1][i] < level[i] || !(levelbuffer[2][i]--)) {
        levelbuffer[1][i] = level[i];
        levelbuffer[2][i] = 23;
      }
      //else
      //  levelbuffer[1][i]--;

      for(int y = (int)(level[i] / level_height_scaler / y_spacing) * y_spacing; y < (int)(levelbuffer[1][i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        const COLORREF copycolour = SmallAuxColour[levelbuffer[2][i] / 4][colour_lookup[levelbuffer[1][i]][y] / 2];
        for(int tx = x; tx < x_stop; tx++)
          rgbbuffer[y * image_width + tx] = copycolour;
      }

      for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        const COLORREF copycolour = FreqBarColour[colour_lookup[level[i]][y]];
        for(int tx = x; tx < x_stop; tx++)
          rgbbuffer[y * image_width + tx] = copycolour;
      }

      if(peaklevel[i]) {
        const COLORREF copycolour = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        const int temp = (int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width;
        do{
          rgbbuffer[temp + x] = copycolour;
        }while(++x < x_stop);
      }
    }
  }
}

void CalculateDoubleShadowAuxColours(void)
{
  const unsigned int bgR = GetBValue(VolumeColour[0]);
  const unsigned int bgG = GetGValue(VolumeColour[0]);
  const unsigned int bgB = GetRValue(VolumeColour[0]);
  for(int i = 0; i < 256; i++) {
    //AuxColour[0][i] = bmpRGB(GetBValue(FreqBarColour[i]) / 2.75f, GetGValue(FreqBarColour[i]) / 2.75f, GetRValue(FreqBarColour[i]) / 2.75f);
    //AuxColour[1][i] = bmpRGB(GetBValue(FreqBarColour[i]) / 1.75f, GetGValue(FreqBarColour[i]) / 1.75f, GetRValue(FreqBarColour[i]) / 1.75f);
    AuxColour[0][i] = bmpRGB((GetBValue(FreqBarColour[i]) * 2 + bgR * 5) / 7, (GetGValue(FreqBarColour[i]) * 2 + bgG * 5) / 7, (GetRValue(FreqBarColour[i]) * 2 + bgB * 5) / 7);
    AuxColour[1][i] = bmpRGB((GetBValue(FreqBarColour[i]) + bgR) / 2, (GetGValue(FreqBarColour[i]) + bgG) / 2, (GetRValue(FreqBarColour[i]) + bgB) / 2);
  }
}

void RenderSingleBarsDoubleShadow()
{
  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      const int x = i * total_width;

      if(levelbuffer[2][i] < level[i])
        levelbuffer[2][i] = level[i];
      else {
        levelbuffer[2][i] -= 2;
        if(levelbuffer[2][i] < 0)
          levelbuffer[2][i] = 0;
      }

      if(levelbuffer[1][i] < levelbuffer[2][i])
        levelbuffer[1][i] = levelbuffer[2][i];
      else
        levelbuffer[1][i]--;

	  for(int y = (int)(levelbuffer[2][i] / level_height_scaler) / y_spacing * y_spacing; y < (int)(levelbuffer[1][i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        rgbbuffer[y * image_width + x] = AuxColour[0][colour_lookup[levelbuffer[1][i]][y]];
	  }

	  for(int y = (int)(level[i] / level_height_scaler) / y_spacing * y_spacing; y < (int)(levelbuffer[2][i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        rgbbuffer[y * image_width + x] = AuxColour[1][colour_lookup[levelbuffer[2][i]][y]];
	  }

	  for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
	    _ASSERT(y < 255);
        rgbbuffer[y * image_width + x] = FreqBarColour[colour_lookup[level[i]][y]];
	  }

      if(peaklevel[i])
        rgbbuffer[(int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
    }
  }
}

void RenderWideBarsDoubleShadow()
{
  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      int x = i * total_width;
      const int x_stop = i * total_width + band_width;

      if(levelbuffer[2][i] < level[i])
        levelbuffer[2][i] = level[i];
      else {
        levelbuffer[2][i] -= 2;
        if(levelbuffer[2][i] < 0)
          levelbuffer[2][i] = 0;
      }

      if(levelbuffer[1][i] < levelbuffer[2][i])
        levelbuffer[1][i] = levelbuffer[2][i];
      else
        levelbuffer[1][i]--;

      for(int y = (int)(levelbuffer[2][i] / level_height_scaler) / y_spacing * y_spacing; y < (int)(levelbuffer[1][i] / level_height_scaler); y += y_spacing) {
        const COLORREF copycolour = AuxColour[0][colour_lookup[levelbuffer[1][i]][y]];
        for(int tx = x; tx < x_stop; tx++)
          rgbbuffer[y * image_width + tx] = copycolour;
      }

      for(int y = (int)(level[i] / level_height_scaler / y_spacing) * y_spacing; y < (int)(levelbuffer[2][i] / level_height_scaler); y += y_spacing) {
        const COLORREF copycolour = AuxColour[1][colour_lookup[levelbuffer[2][i]][y]];
        for(int tx = x; tx < x_stop; tx++)
          rgbbuffer[y * image_width + tx] = copycolour;
      }

      for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
        const COLORREF copycolour = FreqBarColour[colour_lookup[level[i]][y]];
        for(int tx = x; tx < x_stop; tx++)
          rgbbuffer[y * image_width + tx] = copycolour;
      }

      if(peaklevel[i]) {
        const COLORREF copycolour = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        const int temp = (int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width;
        do{
          rgbbuffer[temp + x] = copycolour;
        }while(++x < x_stop);
      }
    }
  }
}

void CalculateSmokeAuxColours(void)
{
    const unsigned int bgR = GetBValue(VolumeColour[0]);
    const unsigned int bgG = GetGValue(VolumeColour[0]);
    const unsigned int bgB = GetRValue(VolumeColour[0]);
	for(int j = 0, i = 0; j < 128; j++, i += 2) {
		SmallAuxColour[3][j] = bmpRGB((GetBValue(FreqBarColour[i]) + bgR) / 2, (GetGValue(FreqBarColour[i]) + bgG) / 2, (GetRValue(FreqBarColour[i]) + bgB) / 2);
		SmallAuxColour[2][j] = bmpRGB((GetBValue(FreqBarColour[i]) * 4 + bgR * 5) / 9, (GetGValue(FreqBarColour[i]) * 4 + bgG * 5) / 9, (GetRValue(FreqBarColour[i]) * 4 + bgB * 5) / 9);
		SmallAuxColour[1][j] = bmpRGB((GetBValue(FreqBarColour[i]) * 3 + bgR * 5) / 8, (GetGValue(FreqBarColour[i]) * 3 + bgG * 5) / 8, (GetRValue(FreqBarColour[i]) * 3 + bgB * 5) / 8);
		SmallAuxColour[0][j] = bmpRGB((GetBValue(FreqBarColour[i]) * 2 + bgR * 5) / 7, (GetGValue(FreqBarColour[i]) * 2 + bgG * 5) / 7, (GetRValue(FreqBarColour[i]) * 2 + bgB * 5) / 7);
	}
}

void RenderSingleBarsSmoke()
{
  static int movesmoke = 1;

  if(!(--movesmoke)) {
    for(int i = bands - 1; i > 0; i--)
      levelbuffer[1][i] = levelbuffer[1][i - 1];
    levelbuffer[1][0] = 0;
    movesmoke = total_width;
  }

  if (rgbbuffer) {
    for(int i = 0; i < bands; i++) {
      const int x = i * total_width;

      if(levelbuffer[1][i] < level[i])
        levelbuffer[1][i] = level[i];
      else
        levelbuffer[1][i] -= m_rand() % 4;

      for(int y = (int)(level[i] / level_height_scaler) / y_spacing * y_spacing; y < (int)(levelbuffer[1][i] / level_height_scaler); y += y_spacing)
        rgbbuffer[y * image_width + x] = SmallAuxColour[levelbuffer[1][i] / 72][colour_lookup[levelbuffer[1][i]][y] / 2];

      for(int y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing)
        rgbbuffer[y * image_width + x] = FreqBarColour[colour_lookup[level[i]][y]];

      if(peaklevel[i])
        rgbbuffer[(int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width + x] = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
    }
  }
}

void RenderWideBarsSmoke()
{
  // note: 54 is used to calculate the Aux Colour since 64 = 255/4
  //static int windchange = 10, winddirection = -1, windfalloff = 7, falloffchange = 5;
  static int movesmoke = 1;
  int i, x, y, tx, temp;
  const int smokebars = bands * band_width;

  //if(!(--windchange)) {
  //  windchange = random(30) + 1;
  //  winddirection = random(2) * 2 - 1;
  //}

  //if(winddirection == 1) {
  //  for(int i = 0; i < bands - 1; i++)
  //    levelbuffer[1][i] = levelbuffer[1][i + winddirection];
  //  levelbuffer[1][bands - 1] = 0;
  //}
  //else {
  if(--movesmoke < band_width) {
    //for(int i = bands - 1; i > 0; i--)
      //levelbuffer[1][i] = levelbuffer[1][i - 1];
    for(i = smokebars - 1; i > 0; i--)
      levelbuffer[1][i] = levelbuffer[1][i - 1];
    levelbuffer[1][0] = 0;
    //movesmoke = total_width;
    //movesmoke = x_spacing + 1;
    //movesmoke = x_spacing / band_width + 1;  // works but interger math no good
    if(!movesmoke)
      movesmoke = total_width;
  }
  //}

  //if(!(--falloffchange)) {
  //  falloffchange = random(30) + 1;
  //  windfalloff = random(7) + 1;
  //}

  if (rgbbuffer) {
    i = 0;
    for(x = 0; x < draw_width; x += total_width) {
      const int x_stop = x + band_width;
      for(tx = x; tx < x_stop; tx++, i++) {

        if(levelbuffer[1][i] < level[i / band_width])
          levelbuffer[1][i] = level[i / band_width];
        else
          levelbuffer[1][i] -= m_rand() % 4;

        for(y = (int)(level[i / band_width] / level_height_scaler / y_spacing) * y_spacing; y < (int)(levelbuffer[1][i] / level_height_scaler); y += y_spacing)
          rgbbuffer[y * image_width + tx] = SmallAuxColour[levelbuffer[1][i] / 72][colour_lookup[levelbuffer[1][i]][y] / 2];
          //copycolour = SmallAuxColour[levelbuffer[1][i] / 72][colour_lookup[levelbuffer[1][i]][y] / 2];
          //for(tx = x; tx < x_stop; tx++)
          //  rgbbuffer[y * image_width + tx] = copycolour;
      }
    }

    for(i = 0; i < bands; i++) {
      x = i * total_width;
      const int x_stop = i * total_width + band_width;

      for(y = 0; y < (int)(level[i] / level_height_scaler); y += y_spacing) {
        const COLORREF copycolour = FreqBarColour[colour_lookup[level[i]][y]];
        for(tx = x; tx < x_stop; tx++)
          rgbbuffer[y * image_width + tx] = copycolour;
      }

      if(peaklevel[i]) {
        const COLORREF copycolour = PeakColour[peak_lookup[peakreferencelevel[i]][peakchange[i]]];
        temp = (int)(peaklevel[i] / draw_height_scaler / y_spacing) * y_spacing * image_width;
        do{
          rgbbuffer[temp + x] = copycolour;
        }while(++x < x_stop);
      }
    }
  }
}

void PeakLevelNormal()
{
  for(int _level = 1; _level < 256; _level++) {
    peak_level_length[_level] = (short)peakchangerate;
    for(int i = 0; i <= peakchangerate; i++)
      peak_level_lookup[_level][i] = (short)_level;
  }
}

// makes the peaks fall
void PeakLevelFall()
{
  const int fall_speed = 5;
  int level_dur;

  for(int _level = 1; _level < 256; _level++) {
    if(peakchangerate < 256 - 255 / fall_speed)
      level_dur = peakchangerate;
    else
      level_dur = (255 - _level) / fall_speed;

    peak_level_length[_level] = (short)(level_dur + _level / fall_speed);

    for(int i = 0; i <= level_dur; i++)
      peak_level_lookup[_level][peak_level_length[_level] - i] = (short)_level;

    for (int i = level_dur + 1, y = _level - fall_speed; i <= peak_level_length[_level]; i++, y -= fall_speed) {
      if (!peakfalloffmode) {
        // original linear change
        peak_level_lookup[_level][peak_level_length[_level] - i] = (short)y;
      }
      else {
        // normalised log change which will be a slow to fast change
        peak_level_lookup[_level][peak_level_length[_level] - i] = (y > 0 ? (short)floor((log10(y) / log10(_level)) * _level) : 0);
      }
    }
  }
}

// makes the peaks rise
void PeakLevelRise()
{
  const int fall_speed = 5;
  int level_dur;

  for(int _level = 1; _level < 256; _level++) {
    if(peakchangerate < 256 - 255 / fall_speed)
      level_dur = peakchangerate;
    else
      level_dur = 255 - (255 - _level) / fall_speed;

    peak_level_length[_level] = (short)(level_dur + (255 - _level) / fall_speed);

    for(int i = 0; i <= level_dur; i++)
      peak_level_lookup[_level][peak_level_length[_level] - i] = (short)_level;

    for(int i = level_dur + 1, y = _level + fall_speed; i <= peak_level_length[_level]; i++, y += fall_speed)
      peak_level_lookup[_level][peak_level_length[_level] - i] = (short)y;
  }
}

// makes half the peaks fall and half rise
void PeakLevelFallAndRise()
{
  const int fall_speed = 5;
  int level_dur;

  for(int _level = 1; _level < 256; _level++) {
    if(_level % 2) { // make peak fall
      if(peakchangerate < 256 - 255 / fall_speed)
        level_dur = peakchangerate;
      else
        level_dur = 255 - _level / fall_speed;

      peak_level_length[_level] = (short)(level_dur + _level / fall_speed);

      for(int i = 0; i <= level_dur; i++)
        peak_level_lookup[_level][peak_level_length[_level] - i] = (short)_level;

      for(int i = level_dur + 1, y = _level - fall_speed; i <= peak_level_length[_level]; i++, y -= fall_speed)
        peak_level_lookup[_level][peak_level_length[_level] - i] = (short)y;
    }
    else {  // make peak rise
      if(peakchangerate < 256 - 255 / fall_speed)
        level_dur = peakchangerate;
      else
        level_dur = 255 - (255 - _level) / fall_speed;

      peak_level_length[_level] = (short)(level_dur + (255 - _level) / fall_speed);

      for(int i = 0; i <= level_dur; i++)
        peak_level_lookup[_level][peak_level_length[_level] - i] = (short)_level;

      for(int i = level_dur + 1, y = _level + fall_speed; i <= peak_level_length[_level]; i++, y += fall_speed)
        peak_level_lookup[_level][peak_level_length[_level] - i] = (short)y;
    }
  }
}

// makes the peaks rise a bit then fall
void PeakLevelRiseFall()
{
  const int fall_speed = 5;
  int level_dur;

  for(int _level = 1; _level < 256; _level++) {
    if(peakchangerate < 256 - 255 / fall_speed)
      level_dur = peakchangerate;
    else
      level_dur = 255 - _level / fall_speed;

    peak_level_length[_level] = (short)(level_dur + _level / fall_speed);

    float riseamount = 25.0f / (level_dur + 1);
    float rise = 0.0f;
    for(int i = 0; i <= level_dur; i++, rise += riseamount) {
      peak_level_lookup[_level][peak_level_length[_level] - i] = (short)(_level + rise);
      if(peak_level_lookup[_level][peak_level_length[_level] - i] > 255)
        peak_level_lookup[_level][peak_level_length[_level] - i] = 255;
    }

    for(int i = level_dur + 1, y = peak_level_lookup[_level][level_dur] - fall_speed; i <= peak_level_length[_level]; i++, y -= fall_speed)
      if(y >= 0)
        peak_level_lookup[_level][peak_level_length[_level] - i] = (short)y;
      else
        peak_level_lookup[_level][peak_level_length[_level] - i] = 0;
  }
}

// make the peaks instantly rise quickly like sparks
void PeakLevelSparks()
{
  for(int _level = 1; _level < 256; _level++) {
    if(peakchangerate < 200)
      peak_level_length[_level] = (short)(peakchangerate + (m_rand() % 56));
      //peak_level_length[_level] = (short)(peakchangerate / 2 + random(peakchangerate / 8 + 25));
    else
      peak_level_length[_level] = (short)(peakchangerate);

    int level_dur = peak_level_length[_level] / 2 + (m_rand() % peak_level_length[_level] / 3);
    for(int i = peak_level_length[_level]; i > level_dur; i--)
      peak_level_lookup[_level][i] = (short)(_level);

    int risevalue = (m_rand() % 5) + 3;
    for(int i = level_dur, rise = risevalue; i >= 0; i--, rise += risevalue) {
      if(_level + rise > 255)
        peak_level_lookup[_level][i] = 0;
      else
        peak_level_lookup[_level][i] = (short)(_level + rise);
    }
  }
}

// window procedure for our window
LRESULT CALLBACK AtAnWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
        case WM_ERASEBKGND:
            EraseWindow((HDC)wParam);
            return 1;
        case WM_NCPAINT:
            return 0;
		/*/case WM_PAINT:
		{
			PAINTSTRUCT ps = {0};
			HDC hdc = BeginPaint(hwnd,&ps);   // BeginPaint clips to only the
			EraseWindow(hdc);
			SetDIBitsToDevice(hdc, draw_x_start, draw_y_start, draw_width, draw_height,
                                0, 0, 0, draw_height, rgbbuffer, &bmi, DIB_RGB_COLORS);
			EndPaint(hwnd,&ps);
			return 0;
		}/**/
		case WM_CONTEXTMENU:
			{
				short xPos = GET_X_LPARAM(lParam);
				short yPos = GET_Y_LPARAM(lParam);

				// this will handle the menu being shown not via the mouse actions
				// so is positioned just below the header if no selection but there's a queue
				// or below the item selected (or the no files in queue entry)
				if (xPos == -1 || yPos == -1)
				{
					RECT rc = {0};
					GetWindowRect(hwnd, &rc);
					xPos = (short)rc.left;
					yPos = (short)rc.top;
				}

                CheckMenuItem(popupmenu, CM_FLIP, (bFlip ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

#ifdef WACUP_BUILD
				// we force the un-skinned version of the menu
				// as us being on a different thread doesn't
				// work with how the gen_ml skinning works
				TrackPopup(popupmenu, TPM_RIGHTBUTTON, xPos, yPos, hwnd);
#else
				TrackPopupMenu(popupmenu, TPM_RIGHTBUTTON, xPos, yPos, 0, hwnd, NULL);
#endif
			}
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:  // fall through
		case WM_KEYUP:
		case WM_CHAR:
		case WM_MOUSEWHEEL:
			PostMessage(AtAnSt_Vis_mod.hwndParent, message, wParam, lParam);
			return 0;
		case WM_WINDOWPOSCHANGED:
			{	// update the window size
				RECT r = {0};
				GetClientRect(hwnd, &r);
				win_width = r.right - r.left;
				win_height = r.bottom - r.top;
                if ((win_width > 2) && (win_height > 2))
                {
                    CalculateAndUpdate();
                }
			}
			return 0;
		case WM_COMMAND:
			// process selections for popup menu
			switch(LOWORD(wParam)) {
			case ID_VIS_CFG:
			case CM_CONFIG:
				ConfigStereo(NULL);
				return 0;
			case CM_PROFILES:
				DialogBox(AtAnSt_Vis_mod.hDllInstance, MAKEINTRESOURCE(IDD_SAVEOPTIONS), hatan, (DLGPROC)SaveDialogProc);
				return 0;
            case CM_FLIP:
                bFlip = !bFlip;
                bmi.bmiHeader.biHeight = (!bFlip ? MAX_WIN_HEIGHT : -MAX_WIN_HEIGHT);
                WritePrivateProfileBool(cszIniMainSection, L"Flip", bFlip, szMainIniFilename);
                return 0;
			case CM_CLOSE:
				DestroyWindow(hwnd);
				return 0;
			case CM_ABOUT:
				AboutMessage();
				return 0;
			case ID_VIS_NEXT:
				LoadNextProfile();
				SaveDialog_UpdateProfilesProperty();
				break;
			case ID_VIS_PREV:
				LoadPreviousProfile();
				SaveDialog_UpdateProfilesProperty();
				break;
			case ID_VIS_RANDOM:
				{	// keep random button unchecked
					SendMessage(AtAnSt_Vis_mod.hwndParent, WM_WA_IPC, 0, IPC_CB_VISRANDOM);
					// if not being asked about random state, load a random profile
					if(HIWORD(wParam) != 0xFFFF) {
						const unsigned int count = CountProfileFiles();
						LoadProfileNumber((count > 0 ? (m_rand() % count) : 0));
						SaveDialog_UpdateProfilesProperty();
					}
				}
				break;
			case ID_VIS_FS:
				break;
			case ID_VIS_MENU:
				{
					POINT pt = {0};
					if(GetCursorPos(&pt))
#ifdef WACUP_BUILD
						// we force the un-skinned version of the menu
						// as us being on a different thread doesn't
						// work with how the gen_ml skinning works
						TrackPopup(popupmenu, TPM_CENTERALIGN, pt.x, pt.y, hwnd);
#else
						TrackPopupMenu(popupmenu, TPM_CENTERALIGN, pt.x, pt.y, 0, hwnd, NULL);
#endif
				}
				return 0;
			}
			break;
		case WM_OPEN_PROPERTYWIN:
			OpenConfigStereoWindow();
			break;
		case WM_CLOSE_PROPERTYWIN:
			if(IsWindow(config_win)) {
				DestroyWindow(config_win);
				config_win = NULL;
				hwndCurrentConfigProperty = NULL;
				if(wParam == PROP_WIN_OK)
					SaveTempCurrentSettings();
				else if(wParam == PROP_WIN_CANCEL)
					LoadCurrentProfile();
			}
			return 0;
	}
	return DefWindowProc(hwnd,message,wParam,lParam);
}

// post the config window close request message to the parent if not yet sent
void ConfigDialog_PostCloseMessage(WPARAM wParam)
{
	if(bPostCloseConfig) {
		PostMessage(hatan, WM_CLOSE_PROPERTYWIN, wParam, 0);
		bPostCloseConfig = false;
	}
}

BOOL ConfigDialog_Notify(HWND hwndDlg, LPARAM lParam)
{
	switch (((LPNMHDR)lParam)->code) {
	case PSN_RESET: // Cancel hit
		if(IsWindow(hatan))
			ConfigDialog_PostCloseMessage(PROP_WIN_CANCEL);
		SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
		return TRUE;
	case PSN_APPLY: // Apply/OK hit
		// if vis window and OK clicked, request to close properties
		if(IsWindow(hatan) && ((LPPSHNOTIFY)lParam)->lParam == TRUE)
			ConfigDialog_PostCloseMessage(PROP_WIN_OK);
		else // Apply clicked
			SaveTempCurrentSettings();
		SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL ConfigDialog_Common(HWND hwndDlg, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam)
{
  switch(uMsg) {
    case WM_NOTIFY:
		return ConfigDialog_Notify(hwndDlg, lParam);
	case WM_CONFIG_OPEN_ERROR:
		MessageBoxA(hwndDlg, "Please close the properties window before starting the plug-in.", "Configure is open!", MB_OK);
		break;
    default: return false;
  }
  return true;
}

void SetDlgItemFloatText(HWND hwndDlg, int nDlgItem, float fValue)
{
	// TODO deal with localisation quirks?
	wchar_t sz[64] = {0};
	_snwprintf(sz, ARRAYSIZE(sz), L"%.2f", (double)fValue);
	SetDlgItemText(hwndDlg, nDlgItem, sz);
}

// for the Analyzer section
BOOL CALLBACK ConfigDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  wchar_t t[16] = {0};
  int i;
  //RECT r;
  switch(uMsg) {
    case WM_SHOWWINDOW:
		if(!wParam)
			return TRUE;
		hwndCurrentConfigProperty = hwndDlg;
	case WM_UPDATE_PROFILE_LIST:
    case WM_INITDIALOG:
      DarkModeSetup(hwndDlg);
      //if(!IsWindow(config_win))
        //config_win = hwndDlg;
      SendDlgItemMessage(hwndDlg, IDC_FALLOFF, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(1, 75));
      SendDlgItemMessage(hwndDlg, IDC_FALLOFF, TBM_SETTICFREQ, (WPARAM)5, 0);
      SendDlgItemMessage(hwndDlg, IDC_FALLOFF, TBM_SETPOS, (WPARAM)true, (LPARAM)falloffrate);
      SetDlgItemText(hwndDlg, IDC_FALLOFFVAL, I2WStr(falloffrate, t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_PEAKCHANGE, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 255));
      SendDlgItemMessage(hwndDlg, IDC_PEAKCHANGE, TBM_SETTICFREQ, (WPARAM)10, 0);
      SendDlgItemMessage(hwndDlg, IDC_PEAKCHANGE, TBM_SETPOS, (WPARAM)true, (LPARAM)peakchangerate);
      SetDlgItemText(hwndDlg, IDC_PEAKCHANGEVAL, I2WStr(peakchangerate, t, ARRAYSIZE(t)));

      //SendDlgItemMessage(hwndDlg, IDC_FREQHIGH, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 575));
      //SendDlgItemMessage(hwndDlg, IDC_FREQHIGH, TBM_SETTICFREQ, (WPARAM)10, 0);
      //SendDlgItemMessage(hwndDlg, IDC_FREQHIGH, TBM_SETPOS, (WPARAM)true, (LPARAM)high_freq);
      //SetDlgItemText(hwndDlg, IDC_FREQHIGHVAL, I2WStr((int)(high_freq * FREQ_SCALE), t, ARRAYSIZE(t)));

      //SendDlgItemMessage(hwndDlg, IDC_FREQLOW, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 575));
      //SendDlgItemMessage(hwndDlg, IDC_FREQLOW, TBM_SETTICFREQ, (WPARAM)10, 0);
      //SendDlgItemMessage(hwndDlg, IDC_FREQLOW, TBM_SETPOS, (WPARAM)true, (LPARAM)low_freq);
      //SetDlgItemText(hwndDlg, IDC_FREQLOWVAL, I2WStr((int)(low_freq * FREQ_SCALE), t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_BANDWIDTH, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(1, 15));
      SendDlgItemMessage(hwndDlg, IDC_BANDWIDTH, TBM_SETTICFREQ, (WPARAM)1, 0);
      SendDlgItemMessage(hwndDlg, IDC_BANDWIDTH, TBM_SETPOS, (WPARAM)true, (LPARAM)requested_band_width);
      //SetDlgItemText(hwndDlg, IDC_BANDWIDTHVAL, I2WStr((int)(low_freq * FREQ_SCALE), t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_XSPACE, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 10));
      SendDlgItemMessage(hwndDlg, IDC_XSPACE, TBM_SETTICFREQ, (WPARAM)1, 0);
      SendDlgItemMessage(hwndDlg, IDC_XSPACE, TBM_SETPOS, (WPARAM)true, (LPARAM)x_spacing);
      //SetDlgItemText(hwndDlg, IDC_XSPACEVAL, I2WStr((int)(low_freq * FREQ_SCALE), t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_YSPACE, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(1, 7));
      SendDlgItemMessage(hwndDlg, IDC_YSPACE, TBM_SETTICFREQ, (WPARAM)1, 0);
      SendDlgItemMessage(hwndDlg, IDC_YSPACE, TBM_SETPOS, (WPARAM)true, (LPARAM)y_spacing);
      //SetDlgItemText(hwndDlg, IDC_YSPACEVAL, I2WStr((int)(low_freq * FREQ_SCALE), t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_LATENCY, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 120));
      SendDlgItemMessage(hwndDlg, IDC_LATENCY, TBM_SETTICFREQ, (WPARAM)10, 0);
      SendDlgItemMessage(hwndDlg, IDC_LATENCY, TBM_SETPOS, (WPARAM)true, (LPARAM)AtAnSt_Vis_mod.latencyMs);
      SetDlgItemText(hwndDlg, IDC_LATENCYVAL, I2WStr(AtAnSt_Vis_mod.latencyMs, t, ARRAYSIZE(t)));

      //SendDlgItemMessage(hwndDlg, IDC_WINHEIGHT, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(8, MAX_WIN_HEIGHT));
      //SendDlgItemMessage(hwndDlg, IDC_WINHEIGHT, TBM_SETTICFREQ, (WPARAM)10, 0);
      //SendDlgItemMessage(hwndDlg, IDC_WINHEIGHT, TBM_SETPOS, (WPARAM)true, (LPARAM)win_height);
      //SetDlgItemText(hwndDlg, IDC_WINHEIGHTVAL, I2WStr(win_height, t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_FFTENVELOPE, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 500));
      SendDlgItemMessage(hwndDlg, IDC_FFTENVELOPE, TBM_SETTICFREQ, (WPARAM)10, 0);
      SendDlgItemMessage(hwndDlg, IDC_FFTENVELOPE, TBM_SETPOS, (WPARAM)true, (LPARAM)(int)(fFftEnvelope * 100.0001));
      //SetDlgItemText(hwndDlg, IDC_FFTENVELOPEVAL, I2WStr((int)(fFftEnvelope * 100.0001), t, ARRAYSIZE(t)));
	  SetDlgItemFloatText(hwndDlg, IDC_FFTENVELOPEVAL, fFftEnvelope);

      SendDlgItemMessage(hwndDlg, IDC_FFTSCALE, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(1, 250));
      SendDlgItemMessage(hwndDlg, IDC_FFTSCALE, TBM_SETTICFREQ, (WPARAM)10, 0);
      SendDlgItemMessage(hwndDlg, IDC_FFTSCALE, TBM_SETPOS, (WPARAM)true, (LPARAM)(int)(fFftScale * 10.0001));
      //SetDlgItemText(hwndDlg, IDC_FFTSCALEVAL, I2WStr((int)(fFftScale * 10.001), t, ARRAYSIZE(t)));
  	  SetDlgItemFloatText(hwndDlg, IDC_FFTSCALEVAL, fFftScale);


	  SendDlgItemMessage(hwndDlg, IDC_REVERSELEFT, BM_SETCHECK, reverseleft ? BST_CHECKED : BST_UNCHECKED, 0);
	  SendDlgItemMessage(hwndDlg, IDC_REVERSERIGHT, BM_SETCHECK, reverseright ? BST_CHECKED : BST_UNCHECKED, 0);
	  SendDlgItemMessage(hwndDlg, IDC_MONO, BM_SETCHECK, mono ? BST_CHECKED : BST_UNCHECKED, 0);
	  SendDlgItemMessage(hwndDlg, IDC_FFTEQUALIZE, BM_SETCHECK, bFftEqualize ? BST_CHECKED : BST_UNCHECKED, 0);

	  // clear the checks
      SendDlgItemMessage(hwndDlg, IDC_BASE_AVERAGE, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      SendDlgItemMessage(hwndDlg, IDC_BASE_UNION, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      // now set the check
      if(levelbase == LEVEL_AVERAGE)
        SendDlgItemMessage(hwndDlg, IDC_BASE_AVERAGE, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      else
        SendDlgItemMessage(hwndDlg, IDC_BASE_UNION, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      return true;
    case WM_HSCROLL:
      falloffrate = (int)SendDlgItemMessage(hwndDlg, IDC_FALLOFF, TBM_GETPOS, 0, 0);
      SetDlgItemText(hwndDlg, IDC_FALLOFFVAL, I2WStr(falloffrate, t, ARRAYSIZE(t)));

      AtAnSt_Vis_mod.latencyMs = (int)SendDlgItemMessage(hwndDlg, IDC_LATENCY, TBM_GETPOS, 0, 0);
      SetDlgItemText(hwndDlg, IDC_LATENCYVAL, I2WStr(AtAnSt_Vis_mod.latencyMs, t, ARRAYSIZE(t)));

      fFftEnvelope = (float)SendDlgItemMessage(hwndDlg, IDC_FFTENVELOPE, TBM_GETPOS, 0, 0) / 99.9990f;
      //SetDlgItemText(hwndDlg, IDC_FFTENVELOPEVAL, I2WStr((int)(fFftEnvelope * 100.0001), t, ARRAYSIZE(t)));
	  SetDlgItemFloatText(hwndDlg, IDC_FFTENVELOPEVAL, fFftEnvelope);

      fFftScale = (float)SendDlgItemMessage(hwndDlg, IDC_FFTSCALE, TBM_GETPOS, 0, 0) / 9.9990f;
      //SetDlgItemText(hwndDlg, IDC_FFTSCALEVAL, I2WStr((int)(fFftScale * 10.001), t, ARRAYSIZE(t)));
  	  SetDlgItemFloatText(hwndDlg, IDC_FFTSCALEVAL, fFftScale);

      requested_band_width = (int)SendDlgItemMessage(hwndDlg, IDC_BANDWIDTH, TBM_GETPOS, 0, 0);
      x_spacing = (int)SendDlgItemMessage(hwndDlg, IDC_XSPACE, TBM_GETPOS, 0, 0);
      y_spacing = (int)SendDlgItemMessage(hwndDlg, IDC_YSPACE, TBM_GETPOS, 0, 0);

      switch(GetDlgCtrlID((HWND)lParam)) {
        /*case IDC_FREQHIGH:
          /*high_freq = (int)SendDlgItemMessage(hwndDlg, IDC_FREQHIGH, TBM_GETPOS, 0, 0);
          if(high_freq <= low_freq) {
            high_freq = low_freq + 1;
            SendDlgItemMessage(hwndDlg, IDC_FREQHIGH, TBM_SETPOS, (WPARAM)true, (LPARAM)high_freq);
          }
          SetDlgItemText(hwndDlg, IDC_FREQHIGHVAL, I2WStr((int)(high_freq * FREQ_SCALE), t, ARRAYSIZE(t)));
          break;
        case IDC_FREQLOW:
          /*low_freq = (int)SendDlgItemMessage(hwndDlg, IDC_FREQLOW, TBM_GETPOS, 0, 0);
          if(low_freq >= high_freq) {
            low_freq = high_freq - 1;
            SendDlgItemMessage(hwndDlg, IDC_FREQLOW, TBM_SETPOS, (WPARAM)true, (LPARAM)low_freq);
          }
          SetDlgItemText(hwndDlg, IDC_FREQLOWVAL, I2WStr((int)(low_freq * FREQ_SCALE), t, ARRAYSIZE(t)));
          break;*/
        case IDC_PEAKCHANGE:
          peakchangerate = (int)SendDlgItemMessage(hwndDlg, IDC_PEAKCHANGE, TBM_GETPOS, 0, 0);
          SetDlgItemText(hwndDlg, IDC_PEAKCHANGEVAL, I2WStr(peakchangerate, t, ARRAYSIZE(t)));
          for(i = 0; i < bands; i++)
            if(peakchange[i] > peakchangerate)
              peakchange[i] = peakchangerate;
          break;
        /*case IDC_WINHEIGHT:
          //win_height = (int)SendDlgItemMessage(hwndDlg, IDC_WINHEIGHT, TBM_GETPOS, (WPARAM)true, (LPARAM)win_height);
          //SetDlgItemText(hwndDlg, IDC_WINHEIGHTVAL, I2WStr(win_height, t, ARRAYSIZE(t)));
          //GetWindowRect(hatan,&r);
          //SetWindowPos(hatan, GetParent(hatan), 0, 0, win_width, win_height, SWP_NOACTIVATE | SWP_NOMOVE);
          //RedrawWindow(hatan, NULL, NULL, RDW_INVALIDATE);  // force complete redraw
          break;*/
      }
	  FFTInit(0);
      CalculateVariables();
	  return true;
    case WM_COMMAND:
      switch(HIWORD(wParam)) {
        case BN_CLICKED:
          // make sure button is "checked" (tabing through controls causes BN_CLICKED to be sent)
          switch(LOWORD(wParam)) {
            case IDC_BASE_AVERAGE:
              levelbase = LEVEL_AVERAGE;
              CalculateVariables();
              return true;
            case IDC_BASE_UNION:
              levelbase = LEVEL_UNION;
              CalculateVariables();
              return true;
			case IDC_REVERSELEFT:
				reverseleft = SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				CalculateVariables();
				return TRUE;
			case IDC_REVERSERIGHT:
				reverseright = SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				CalculateVariables();
				return TRUE;
			case IDC_MONO:
				mono = SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				CalculateVariables();
				return TRUE;
			case IDC_FFTEQUALIZE:
				bFftEqualize = SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				FFTInit(0);
				return TRUE;
		  }
	  }
	  return FALSE;
	default:
		return ConfigDialog_Common(hwndDlg, uMsg, wParam, lParam);
  }
}

BOOL CALLBACK LevelDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  // assign a default function
  static unsigned short int selected_func = IDC_LOG10;
  static int *function_table = volume_func; // default to volume

  switch(uMsg) {
    case WM_SHOWWINDOW:
		if(!wParam)
			return TRUE;
		hwndCurrentConfigProperty = hwndDlg;
		goto UpdateControls;
    case WM_INITDIALOG: {
      //if(!IsWindow(config_win))
        //config_win = hwndDlg;
      DarkModeSetup(hwndDlg);
      // create a bitmap to be displayed in the window
      HDC hdcScreen = GetDC(NULL);
      HBITMAP bitmap = CreateCompatibleBitmap(hdcScreen, 256, 256);
      ReleaseDC(NULL, hdcScreen);
      SendDlgItemMessage(hwndDlg, IDC_LEVELGRAPH, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);

      // check off the defaults
      SendDlgItemMessage(hwndDlg, IDC_FREQBARS, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_func, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    }
	// fall through
	case WM_UPDATE_PROFILE_LIST:
UpdateControls:
        // draw the graph
        DrawLevelGraph(hwndDlg, function_table);  // draw the graph
      return TRUE;
    case WM_DESTROY: {
      // delete the bitmap that was created in INITDIAG
      HBITMAP bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_LEVELGRAPH, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
      DeleteObject(bitmap);
      return TRUE;
    }
    case WM_COMMAND:
      switch(HIWORD(wParam)) {
        case BN_CLICKED:
          // make sure button is "checked" (tabing through controls causes BN_CLICKED to be sent)
          switch(LOWORD(wParam)) {
            /*case IDC_FREQBARS:
              if(SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                function_table = level_func;
                DrawLevelGraph(hwndDlg, function_table);  // update the bitmap
              }
              return true;
            case IDC_VOLUME:
              if(SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                function_table = volume_func;
                DrawLevelGraph(hwndDlg, function_table);  // update the bitmap
              }
              return true;*/
            case IDC_LINEAR:  // all these functions fall through
            case IDC_LOG10:   // to set the selected function
            case IDC_LOG20:
            case IDC_LOG30:
            case IDC_LOG100:
            case IDC_NATLOG:
            case IDC_SINE:
            case IDC_SQRT:
            case IDC_1OVERX:
            case IDC_XSQUARED:
              // make sure button is "checked" (tabing through controls causes BN_CLICKED to be sent)
              if(SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED)
                selected_func = LOWORD(wParam);
              return true;
            case IDC_SET: {
              for(int i = 0; i < 256; i++)
                function_table[i] = i;  // reset level function values
            }
            // fall through
            case IDC_ADD: {   // apply the selected function
              switch(selected_func) {
                case IDC_LINEAR:
                  //LinearTable(function_table);
                  break;
                case IDC_LOG10:
                  LogBase10Table(function_table);
                  break;
                case IDC_LOG20:
                  LogBase20Table(function_table);
                  break;
                case IDC_LOG30:
                  LogBase30Table(function_table);
                  break;
                case IDC_LOG100:
                  LogBase100Table(function_table);
                  break;
                case IDC_NATLOG:
                  NaturalLogTable(function_table);
                  break;
                case IDC_SINE:
                  SineTable(function_table);
                  break;
                case IDC_SQRT:
                  SqrtTable(function_table);
                  break;
                case IDC_1OVERX:
                  FractionalTable(function_table);
                  break;
                case IDC_XSQUARED:
                  SqrTable(function_table);
                  break;
              }
              DrawLevelGraph(hwndDlg, function_table);  // update the bitmap
              return true;
            }
            case IDC_AVERAGE: {
              int temptbl[256]; // temp table to hold function values
              for(int i = 0; i < 256; i++)
                temptbl[i] = i;
              switch(selected_func) {
                case IDC_LINEAR:
                  //LinearTable(temptbl);
                  break;
                case IDC_LOG10:
                  LogBase10Table(temptbl);
                  break;
                case IDC_LOG20:
                  LogBase20Table(temptbl);
                  break;
                case IDC_LOG30:
                  LogBase30Table(temptbl);
                  break;
                case IDC_LOG100:
                  LogBase100Table(temptbl);
                  break;
                case IDC_NATLOG:
                  NaturalLogTable(temptbl);
                  break;
                case IDC_SINE:
                  SineTable(temptbl);
                  break;
                case IDC_SQRT:
                  SqrtTable(temptbl);
                  break;
                case IDC_1OVERX:
                  FractionalTable(temptbl);
                  break;
                case IDC_XSQUARED:
                  SqrTable(temptbl);
                  break;
              }
              for(int i = 0; i < 256; i++)  // calculate average
                function_table[i] = (function_table[i] + temptbl[i]) / 2;
              DrawLevelGraph(hwndDlg, function_table);
              return true;
            }
          }
      }
      return false;
	default:
		return ConfigDialog_Common(hwndDlg, uMsg, wParam, lParam);
  }
}

BOOL CALLBACK StyleDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  // assign defaults
  static unsigned short int selected_barstyle = IDC_FIRE, selected_effect = IDC_EFFECT_NONE,
    selected_bg = IDC_BG_BLACK, selected_peak = IDC_PEAKFADE,
    selected_peakeffect = IDC_PEAKEFFECT_NORMAL;

  switch(uMsg) {
    case WM_SHOWWINDOW:
		if(!wParam)
			return TRUE;
		hwndCurrentConfigProperty = hwndDlg;
	case WM_UPDATE_PROFILE_LIST:
      // uncheck the previous values
      SendDlgItemMessage(hwndDlg, selected_barstyle, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_bg, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_peak, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_effect, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_peakeffect, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      //SendDlgItemMessage(hwndDlg, IDC_LOCATION_BOTTOM, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      //SendDlgItemMessage(hwndDlg, IDC_LOCATION_TOP, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      //SendDlgItemMessage(hwndDlg, IDC_ATTACH_SCREEN, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      //SendDlgItemMessage(hwndDlg, IDC_ATTACH_WINAMP, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      //SendDlgItemMessage(hwndDlg, IDC_ATTACH_NOTHING, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
      // fall through to check off new values
    case WM_INITDIALOG: {
      //if(!IsWindow(config_win))
        //config_win = hwndDlg;
      DarkModeSetup(hwndDlg);
      // set Background style
      switch(backgrounddraw) {
        case BACKGROUND_FLASH:
          selected_bg = IDC_BG_FLASH;
          break;
        case BACKGROUND_FLASHGRID:
          selected_bg = IDC_BG_FLASHGRID;
          break;
        case BACKGROUND_GRID:
          selected_bg = IDC_BG_GRID;
          break;
        case BACKGROUND_SOLID:
          selected_bg = IDC_BG_SOLID;
          break;
        default:
          selected_bg = IDC_BG_BLACK;
      }

      // set Bar style
      if(BarColourScheme == BarColourFire)
        selected_barstyle = IDC_FIRE;
      else if(BarColourScheme == BarColourLines)
        selected_barstyle = IDC_LINES;
      else if(BarColourScheme == BarColourWinampFire)
        selected_barstyle = IDC_WINAMPFIRE;
      else if(BarColourScheme == BarColourElevator)
        selected_barstyle = IDC_ELEVATOR;
      else
        selected_barstyle = IDC_CLASSIC;

      // set Peak style
      if(PeakColourScheme == PeakColourLevel)
        selected_peak = IDC_PEAKLEVEL;
      else if(PeakColourScheme == PeakColourLevelFade)
        selected_peak = IDC_PEAKLEVELFADE;
      else
        selected_peak = IDC_PEAKFADE;

      // set effect
      switch(effect) {
        case EFFECT_MIRROR:
          selected_effect = IDC_EFFECT_MIRROR;
          break;
        case EFFECT_REFLECTION:
          selected_effect = IDC_EFFECT_REFLECTION;
          break;
        case EFFECT_REFLECTIONWAVES:
          selected_effect = IDC_EFFECT_WAVES;
          break;
        case EFFECT_SHADOW:
          selected_effect = IDC_EFFECT_SHADOW;
          break;
        case EFFECT_DOUBLESHADOW:
          selected_effect = IDC_EFFECT_DSHADOW;
          break;
        case EFFECT_FADESHADOW:
          selected_effect = IDC_EFFECT_FADESHADOW;
          break;
        case EFFECT_SMOKE:
          selected_effect = IDC_EFFECT_SMOKE;
          break;
        case EFFECT_NONE:
        default:
          selected_effect = IDC_EFFECT_NONE;
          break;
      }

      switch(peakleveleffect) {
        case PEAK_EFFECT_FALL:
          selected_peakeffect = IDC_PEAKEFFECT_FALL;
          break;
        case PEAK_EFFECT_RISE:
          selected_peakeffect = IDC_PEAKEFFECT_RISE;
          break;
        case PEAK_EFFECT_FALLANDRISE:
          selected_peakeffect = IDC_PEAKEFFECT_FALLANDRISE;
          break;
        case PEAK_EFFECT_RISEFALL:
          selected_peakeffect = IDC_PEAKEFFECT_RISEFALL;
          break;
        case PEAK_EFFECT_SPARKS:
          selected_peakeffect = IDC_PEAKEFFECT_SPARKS;
          break;
        case PEAK_EFFECT_NORMAL:
        default:
          selected_peakeffect = IDC_PEAKEFFECT_NORMAL;
      }

      // check off the default styles
      SendDlgItemMessage(hwndDlg, selected_barstyle, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_bg, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_peak, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_effect, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      SendDlgItemMessage(hwndDlg, selected_peakeffect, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

      CheckDlgButton(hwndDlg, IDC_PEAK_FALL_OFF_MODE, (peakfalloffmode ? BST_CHECKED : BST_UNCHECKED));

      EnableControl(hwndDlg, IDC_PEAK_FALL_OFF_MODE,
					(peakleveleffect == PEAK_EFFECT_FALL)/* ||
					(peakleveleffect == PEAK_EFFECT_RISE)*/);
      return true;
    }
    case WM_COMMAND:
      switch(HIWORD(wParam)) {
        case BN_CLICKED:
          switch(LOWORD(wParam)) {
            case IDC_PEAK_FALL_OFF_MODE:
              peakfalloffmode = !!IsDlgButtonChecked(hwndDlg, LOWORD(wParam));
              UpdatePeakColourLookup();
              return true;
            case IDC_PEAKEFFECT_NORMAL:
            case IDC_PEAKEFFECT_FALL:
            case IDC_PEAKEFFECT_RISE:
            case IDC_PEAKEFFECT_FALLANDRISE:
            case IDC_PEAKEFFECT_RISEFALL:
            case IDC_PEAKEFFECT_SPARKS:
              // make sure button is "checked" (tabing through controls causes BN_CLICKED to be sent)
              if(SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                selected_effect = LOWORD(wParam);
                switch(LOWORD(wParam)) {
                  case IDC_PEAKEFFECT_NORMAL:
                    peakleveleffect = PEAK_EFFECT_NORMAL;
                    break;
                  case IDC_PEAKEFFECT_FALL:
                    peakleveleffect = PEAK_EFFECT_FALL;
                    break;
                  case IDC_PEAKEFFECT_RISE:
                    peakleveleffect = PEAK_EFFECT_RISE;
                    break;
                  case IDC_PEAKEFFECT_FALLANDRISE:
                    peakleveleffect = PEAK_EFFECT_FALLANDRISE;
                    break;
                  case IDC_PEAKEFFECT_RISEFALL:
                    peakleveleffect = PEAK_EFFECT_RISEFALL;
                    break;
                  case IDC_PEAKEFFECT_SPARKS:
                    peakleveleffect = PEAK_EFFECT_SPARKS;
                    break;
                }
                EnableControl(hwndDlg, IDC_PEAK_FALL_OFF_MODE,
							  (peakleveleffect == PEAK_EFFECT_FALL)/* ||
							  (peakleveleffect == PEAK_EFFECT_RISE)*/);
                CalculateVariables();
                UpdatePeakColourLookup();
              }
              return true;
            case IDC_EFFECT_NONE:
            case IDC_EFFECT_MIRROR:
            case IDC_EFFECT_REFLECTION:
            case IDC_EFFECT_WAVES:
            case IDC_EFFECT_SHADOW:
            case IDC_EFFECT_DSHADOW:
            case IDC_EFFECT_FADESHADOW:
            case IDC_EFFECT_SMOKE:
              // make sure button is "checked" (tabing through controls causes BN_CLICKED to be sent)
              if(SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                selected_effect = LOWORD(wParam);
                switch(LOWORD(wParam)) {
                  case IDC_EFFECT_NONE:
                    effect = EFFECT_NONE;
                    break;
                  case IDC_EFFECT_MIRROR:
                    effect = EFFECT_MIRROR;
                    break;
                  case IDC_EFFECT_REFLECTION:
                    effect = EFFECT_REFLECTION;
                    break;
                  case IDC_EFFECT_WAVES:
                    effect = EFFECT_REFLECTIONWAVES;
                    break;
                  case IDC_EFFECT_SHADOW:
                    effect = EFFECT_SHADOW;
                    break;
                  case IDC_EFFECT_DSHADOW:
                    effect = EFFECT_DOUBLESHADOW;
                    break;
                  case IDC_EFFECT_FADESHADOW:
                    effect = EFFECT_FADESHADOW;
                    break;
                  case IDC_EFFECT_SMOKE:
                    effect = EFFECT_SMOKE;
                    break;
                }
                ClearBackground();  // clear background (waves draws between grid lines)
                CalculateVariables();
              }
              return true;
            case IDC_PEAKFADE:
            case IDC_PEAKLEVEL:
            case IDC_PEAKLEVELFADE:
              // make sure button is "checked" (tabing through controls causes BN_CLICKED to be sent)
              if(SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                selected_peak = LOWORD(wParam);
                switch(LOWORD(wParam)) {
                  case IDC_PEAKFADE:
                    PeakColourScheme = PeakColourFade;
                    peakcolourstyle = PEAK_FADE;
                    break;
                  case IDC_PEAKLEVEL:
                    PeakColourScheme = PeakColourLevel;
                    peakcolourstyle = PEAK_LEVEL;
                    break;
                  case IDC_PEAKLEVELFADE:
                    PeakColourScheme = PeakColourLevelFade;
                    peakcolourstyle = PEAK_LEVELFADE;
                    break;
                }
                UpdatePeakColourLookup();
              }
              return true;
            case IDC_RANDOM:
              SendDlgItemMessage(hwndDlg, selected_barstyle, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
              RandomColourLookup();
              return true;
            case IDC_CLASSIC:  // all these functions fall through
            case IDC_FIRE:    // to set the selected style
            case IDC_ELEVATOR:
            case IDC_LINES:
            case IDC_WINAMPFIRE:
              // make sure button is "checked" (tabing through controls causes BN_CLICKED to be sent)
              if(SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                selected_barstyle = LOWORD(wParam);
                switch(LOWORD(wParam)) {
                  case IDC_CLASSIC:
                    BarColourScheme = BarColourClassic;
                    barcolourstyle = BAR_CLASSIC;
                    break;
                  case IDC_ELEVATOR:
                    BarColourScheme = BarColourElevator;
                    barcolourstyle = BAR_ELEVATOR;
                    break;
                  case IDC_FIRE:
                    BarColourScheme = BarColourFire;
                    barcolourstyle = BAR_FIRE;
                    break;
                  case IDC_LINES:
                    BarColourScheme = BarColourLines;
                    barcolourstyle = BAR_LINES;
                    break;
                  case IDC_WINAMPFIRE:
                    BarColourScheme = BarColourWinampFire;
                    barcolourstyle = BAR_WINAMPFIRE;
                    break;
                }
                UpdateColourLookup();
              }
              return true;
            case IDC_BG_BLACK:
            case IDC_BG_FLASHGRID:
            case IDC_BG_FLASH:
            case IDC_BG_GRID:
            case IDC_BG_SOLID:
              if(SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                selected_bg = LOWORD(wParam);
                ClearBackground();
                switch(LOWORD(wParam)) {
                  case IDC_BG_BLACK:
                    backgrounddraw = BACKGROUND_BLACK;
                    break;
                  case IDC_BG_FLASHGRID:
                    backgrounddraw = BACKGROUND_FLASHGRID;
                    break;
                  case IDC_BG_FLASH:
                    backgrounddraw = BACKGROUND_FLASH;
                    break;
                  case IDC_BG_GRID:
                    backgrounddraw = BACKGROUND_GRID;
                    break;
                  case IDC_BG_SOLID:
                    backgrounddraw = BACKGROUND_SOLID;
                    break;
                }
                CalculateVariables();
				EraseWindow();
              }
              return true;
            /*case IDC_ATTACH_WINAMP:
              //positioning = POSITIONING_WINAMP;
              CalculateVariables(); // Window needs redrawn
              return true;
            case IDC_ATTACH_SCREEN:
              //positioning = POSITIONING_SCREEN;
              CalculateVariables(); // Window needs redrawn
              return true;
            case IDC_ATTACH_NOTHING:
              //positioning = POSITIONING_COORDINATES;
              CalculateVariables(); // Window needs redrawn
              return true;
            case IDC_LOCATION_BOTTOM:
              //location = LOCATION_BOTTOM;
              CalculateVariables(); // Window needs redrawn
              return true;
            case IDC_LOCATION_TOP:
              //location = LOCATION_TOP;
              CalculateVariables(); // Window needs redrawn
              return true;*/
          }
      }
      return false;
	default:
		return ConfigDialog_Common(hwndDlg, uMsg, wParam, lParam);
  }
}

BOOL CALLBACK ColourDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  // assign a default function
  static COLORREF *colour_table = FreqBarColour;  // selected table
  static int left = 255, right = 0; // selected positions
  //static int selected_border = 0; // selected border colour
  static COLORREF current_colour = 0; // current selected colour
  wchar_t t[16] = {0};

  switch(uMsg) {
    case WM_SHOWWINDOW:
		if(!wParam)
			return TRUE;
		hwndCurrentConfigProperty = hwndDlg;
		goto UpdateControls;
    case WM_INITDIALOG: {
      //if(!IsWindow(config_win))
        //config_win = hwndDlg;
      DarkModeSetup(hwndDlg);
      LoadUserColours();
      // create bitmaps to be displayed in the window
      HDC hdcScreen = GetDC(NULL);
      HBITMAP bitmap = CreateCompatibleBitmap(hdcScreen, 20, 256);
      SendDlgItemMessage(hwndDlg, IDC_BITMAP_RAMP, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);
      bitmap = CreateCompatibleBitmap(hdcScreen, 20, 20);
      SendDlgItemMessage(hwndDlg, IDC_BITMAP_LEFT, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);
      bitmap = CreateCompatibleBitmap(hdcScreen, 20, 20);
      SendDlgItemMessage(hwndDlg, IDC_BITMAP_RIGHT, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);
      bitmap = CreateCompatibleBitmap(hdcScreen, 20, 20);
      SendDlgItemMessage(hwndDlg, IDC_BITMAP_CURCOLOUR, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);
      //bitmap = CreateCompatibleBitmap(hdcScreen, 20, 20);
      //SendDlgItemMessage(hwndDlg, IDC_BITMAP_BORDER, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);
      ReleaseDC(NULL, hdcScreen);

      SendDlgItemMessage(hwndDlg, IDC_LEFTSELECT, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 255));
      SendDlgItemMessage(hwndDlg, IDC_LEFTSELECT, TBM_SETPOS, (WPARAM)true, 255 - left);
      SetDlgItemText(hwndDlg, IDC_LEFTVAL, I2WStr(left, t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_RIGHTSELECT, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 255));
      SendDlgItemMessage(hwndDlg, IDC_RIGHTSELECT, TBM_SETPOS, (WPARAM)true, 255 - right);
      SetDlgItemText(hwndDlg, IDC_RIGHTVAL, I2WStr(right, t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 255));
      SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, (WPARAM)true, GetBValue(current_colour));
      SetDlgItemText(hwndDlg, IDC_REDVAL, I2WStr(GetBValue(current_colour), t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 255));
      SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, (WPARAM)true, GetGValue(current_colour));
      SetDlgItemText(hwndDlg, IDC_GREENVAL, I2WStr(GetGValue(current_colour), t, ARRAYSIZE(t)));

      SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, 255));
      SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, (WPARAM)true, GetRValue(current_colour));
      SetDlgItemText(hwndDlg, IDC_BLUEVAL, I2WStr(GetRValue(current_colour), t, ARRAYSIZE(t)));
      DrawSolidColour(hwndDlg, current_colour, IDC_BITMAP_CURCOLOUR);  // draw the colour box

      // resize the Left & Right select track bars so the bar = exactly 255
      // to perfectly match the bitmap size
      RECT r = { 0 };
      POINT p;
      GetDlgItemRect(hwndDlg, IDC_LEFTSELECT, &r);
      p.x = r.left;
      p.y = r.top;
      ScreenToClient(hwndDlg, &p);
      r.left = p.x;
      r.top = p.y;
      p.x = r.right;
      p.y = r.bottom;
      ScreenToClient(hwndDlg, &p);
      r.right = p.x;
      r.bottom = p.y;
      MoveWindow(GetDlgItem(hwndDlg, IDC_LEFTSELECT), r.left, r.top, r.right - r.left, 274, true);
      GetDlgItemRect(hwndDlg, IDC_RIGHTSELECT,&r);
      p.x = r.left;
      p.y = r.top;
      ScreenToClient(hwndDlg, &p);
      r.left = p.x;
      r.top = p.y;
      p.x = r.right;
      p.y = r.bottom;
      ScreenToClient(hwndDlg, &p);
      r.right = p.x;
      r.bottom = p.y;
      MoveWindow(GetDlgItem(hwndDlg, IDC_RIGHTSELECT), r.left, r.top, r.right - r.left, 274, true);
      //MessageBox(hwndDlg,I2WStr(r.bottom-r.top,t,ARRAYSIZE(t)),"Uh Oh",MB_OK);

      // check off the defaults
      if(colour_table == PeakColour)
        SendDlgItemMessage(hwndDlg, IDC_PEAK, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      else if (colour_table == VolumeColour)
        SendDlgItemMessage(hwndDlg, IDC_VOLUME, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
      else
        SendDlgItemMessage(hwndDlg, IDC_FREQBARS, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

      /*switch(selected_border) {
        case 0:
          SendDlgItemMessage(hwndDlg, IDC_BORDER1, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
          break;
        case 1:
          SendDlgItemMessage(hwndDlg, IDC_BORDER2, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
          break;
        case 2:
          SendDlgItemMessage(hwndDlg, IDC_BORDER3, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
          break;
      }*/
      // fall through to update all other controls
    }
	case WM_UPDATE_PROFILE_LIST:
UpdateControls:
      DrawColourRamp(hwndDlg, colour_table);  // draw the ramp

      SetDlgItemText(hwndDlg, IDC_LEFTRVAL, I2WStr(GetBValue(colour_table[left]), t, ARRAYSIZE(t)));
      SetDlgItemText(hwndDlg, IDC_LEFTGVAL, I2WStr(GetGValue(colour_table[left]), t, ARRAYSIZE(t)));
      SetDlgItemText(hwndDlg, IDC_LEFTBVAL, I2WStr(GetRValue(colour_table[left]), t, ARRAYSIZE(t)));
      DrawSolidColour(hwndDlg, colour_table[left], IDC_BITMAP_LEFT);  // draw the colour box

      SetDlgItemText(hwndDlg, IDC_RIGHTRVAL, I2WStr(GetBValue(colour_table[right]), t, ARRAYSIZE(t)));
      SetDlgItemText(hwndDlg, IDC_RIGHTGVAL, I2WStr(GetGValue(colour_table[right]), t, ARRAYSIZE(t)));
      SetDlgItemText(hwndDlg, IDC_RIGHTBVAL, I2WStr(GetRValue(colour_table[right]), t, ARRAYSIZE(t)));
      DrawSolidColour(hwndDlg, colour_table[right], IDC_BITMAP_RIGHT);  // draw the colour box

      // draw the colour box for the border colour
      //DrawSolidColour(hwndDlg, BorderColour[selected_border], IDC_BITMAP_BORDER);
      return true;
    case WM_DESTROY: {
      // delete the bitmaps that was created in INITDIAG
      HBITMAP bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_BITMAP_RAMP, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
      DeleteObject(bitmap);
      bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_BITMAP_LEFT, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
      DeleteObject(bitmap);
      bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_BITMAP_RIGHT, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
      DeleteObject(bitmap);
      bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_BITMAP_CURCOLOUR, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
      DeleteObject(bitmap);
      //bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_BITMAP_BORDER, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
      //DeleteObject(bitmap);
      WritePrivateProfileColourArray(L"UserColours", UserColours, 16, szMainIniFilename);
      return true;
    }
    case WM_HSCROLL:
      switch(GetDlgCtrlID((HWND)lParam)) {
        case IDC_RED:
        case IDC_GREEN:
        case IDC_BLUE:
          current_colour =
            bmpRGB(SendDlgItemMessage(hwndDlg, IDC_RED, TBM_GETPOS, 0, 0),
            SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_GETPOS, 0, 0),
            SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_GETPOS, 0, 0));

          SetDlgItemText(hwndDlg, IDC_REDVAL, I2WStr(GetBValue(current_colour), t, ARRAYSIZE(t)));
          SetDlgItemText(hwndDlg, IDC_GREENVAL, I2WStr(GetGValue(current_colour), t, ARRAYSIZE(t)));
          SetDlgItemText(hwndDlg, IDC_BLUEVAL, I2WStr(GetRValue(current_colour), t, ARRAYSIZE(t)));
          DrawSolidColour(hwndDlg, current_colour, IDC_BITMAP_CURCOLOUR);  // draw the colour box
          return true;
      }
      return false;
    case WM_VSCROLL:
      switch(GetDlgCtrlID((HWND)lParam)) {
        case IDC_LEFTSELECT:
          left = 255 - (int)SendDlgItemMessage(hwndDlg, IDC_LEFTSELECT, TBM_GETPOS, 0, 0);
          SetDlgItemText(hwndDlg, IDC_LEFTVAL, I2WStr(left, t, ARRAYSIZE(t)));
          SetDlgItemText(hwndDlg, IDC_LEFTRVAL, I2WStr(GetBValue(colour_table[left]), t, ARRAYSIZE(t)));
          SetDlgItemText(hwndDlg, IDC_LEFTGVAL, I2WStr(GetGValue(colour_table[left]), t, ARRAYSIZE(t)));
          SetDlgItemText(hwndDlg, IDC_LEFTBVAL, I2WStr(GetRValue(colour_table[left]), t, ARRAYSIZE(t)));
          DrawSolidColour(hwndDlg, colour_table[left], IDC_BITMAP_LEFT);  // draw the colour box
          return true;
        case IDC_RIGHTSELECT:
          right = 255 - (int)SendDlgItemMessage(hwndDlg, IDC_RIGHTSELECT, TBM_GETPOS, 0, 0);
          SetDlgItemText(hwndDlg, IDC_RIGHTVAL, I2WStr(right, t, ARRAYSIZE(t)));
          SetDlgItemText(hwndDlg, IDC_RIGHTRVAL, I2WStr(GetBValue(colour_table[right]), t, ARRAYSIZE(t)));
          SetDlgItemText(hwndDlg, IDC_RIGHTGVAL, I2WStr(GetGValue(colour_table[right]), t, ARRAYSIZE(t)));
          SetDlgItemText(hwndDlg, IDC_RIGHTBVAL, I2WStr(GetRValue(colour_table[right]), t, ARRAYSIZE(t)));
          DrawSolidColour(hwndDlg, colour_table[right], IDC_BITMAP_RIGHT);  // draw the colour box
          return true;
      }
      return false;
    case WM_COMMAND:
      switch(HIWORD(wParam)) {
        case BN_CLICKED:
          switch(LOWORD(wParam)) {
            /*case IDC_BORDER1:
              selected_border = 0;
              //DrawSolidColour(hwndDlg, BorderColour[selected_border], IDC_BITMAP_BORDER);
              return true;
            case IDC_BORDER2:
              selected_border = 1;
              //DrawSolidColour(hwndDlg, BorderColour[selected_border], IDC_BITMAP_BORDER);
              return true;
            case IDC_BORDER3:
              selected_border = 2;
              //DrawSolidColour(hwndDlg, BorderColour[selected_border], IDC_BITMAP_BORDER);
              return true;
            case IDC_GUESSBORDER: {
              /*RECT r;
              GetClientRect(GetParent(hatan), &r);
              int y = r.bottom / 4 + (m_rand() % r.bottom/2);
              HDC dc = GetDC(GetParent(hatan));
              /*if(m_rand() % 2) {
                BorderColour[0] = FixCOLORREF(GetPixel(dc, 0, y));
                BorderColour[1] = FixCOLORREF(GetPixel(dc, 1, y));
                BorderColour[2] = FixCOLORREF(GetPixel(dc, 2, y));
              } else {
                BorderColour[0] = FixCOLORREF(GetPixel(dc, r.right - 1, y));
                BorderColour[1] = FixCOLORREF(GetPixel(dc, r.right - 2, y));
                BorderColour[2] = FixCOLORREF(GetPixel(dc, r.right - 3, y));
              }
              ReleaseDC(GetParent(hatan), dc);
              DrawSolidColour(hwndDlg, BorderColour[selected_border], IDC_BITMAP_BORDER);
              RedrawWindow(hatan, NULL, NULL, RDW_INVALIDATE);  // force complete redraw
              return true;
            }
            case IDC_BORDERGET:
              //BorderColour[selected_border] = current_colour;
              //DrawSolidColour(hwndDlg, BorderColour[selected_border], IDC_BITMAP_BORDER);
              //RedrawWindow(hatan, NULL, NULL, RDW_INVALIDATE);  // force complete redraw
              return true;*/
            case IDC_PICKCOLOUR: {
              CHOOSECOLOR colour = {0};
              colour.lStructSize = sizeof(CHOOSECOLOR);
              colour.hwndOwner = hwndDlg;
              colour.rgbResult = FixCOLORREF(current_colour);  // result
              colour.lpCustColors = UserColours;   // user colours
              colour.Flags = CC_FULLOPEN | CC_RGBINIT;
              if(PickColour(&colour)) {  // now choose the colour
                // if a colour was chosen, fix it and update the display
                current_colour = FixCOLORREF(colour.rgbResult);
                SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, (WPARAM)true, GetBValue(current_colour));
                SetDlgItemText(hwndDlg, IDC_REDVAL, I2WStr(GetBValue(current_colour), t, ARRAYSIZE(t)));

                SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, (WPARAM)true, GetGValue(current_colour));
                SetDlgItemText(hwndDlg, IDC_GREENVAL, I2WStr(GetGValue(current_colour), t, ARRAYSIZE(t)));

                SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, (WPARAM)true, GetRValue(current_colour));
                SetDlgItemText(hwndDlg, IDC_BLUEVAL, I2WStr(GetRValue(current_colour), t, ARRAYSIZE(t)));
                DrawSolidColour(hwndDlg, current_colour, IDC_BITMAP_CURCOLOUR);  // draw the colour box
              }
              return true;
            }
            case IDC_FADE:
              FadeColours(colour_table, left, right);
              DrawColourRamp(hwndDlg, colour_table);  // draw the ramp
			  CalculateAuxColours();
              return true;
            case IDC_ALLRANDOM:
              CreateRandomRamp(colour_table);
              // fall through
            case IDC_LEFTGET: // fall through
            case IDC_RIGHTGET:
              switch(LOWORD(wParam)) {
                case IDC_LEFTGET:
                  colour_table[left] = current_colour;
					if(left == 0)
						EraseWindow();
                  break;
                case IDC_RIGHTGET:
                  colour_table[right] = current_colour;
					if(right == 0)
						EraseWindow();
                  break;
              }
              SetDlgItemText(hwndDlg, IDC_LEFTRVAL, I2WStr(GetBValue(colour_table[left]), t, ARRAYSIZE(t)));
              SetDlgItemText(hwndDlg, IDC_LEFTGVAL, I2WStr(GetGValue(colour_table[left]), t, ARRAYSIZE(t)));
              SetDlgItemText(hwndDlg, IDC_LEFTBVAL, I2WStr(GetRValue(colour_table[left]), t, ARRAYSIZE(t)));
              DrawSolidColour(hwndDlg, colour_table[left], IDC_BITMAP_LEFT);  // draw the colour box

              SetDlgItemText(hwndDlg, IDC_RIGHTRVAL, I2WStr(GetBValue(colour_table[right]), t, ARRAYSIZE(t)));
              SetDlgItemText(hwndDlg, IDC_RIGHTGVAL, I2WStr(GetGValue(colour_table[right]), t, ARRAYSIZE(t)));
              SetDlgItemText(hwndDlg, IDC_RIGHTBVAL, I2WStr(GetRValue(colour_table[right]), t, ARRAYSIZE(t)));
              DrawSolidColour(hwndDlg, colour_table[right], IDC_BITMAP_RIGHT);  // draw the colour box

              DrawColourRamp(hwndDlg, colour_table);  // draw the ramp
			  CalculateAuxColours();
              return true;
            //case IDC_BORDERPUT:
            case IDC_LEFTPUT:
            case IDC_RIGHTPUT:
              switch(LOWORD(wParam)) {
                //case IDC_BORDERPUT:
                  //current_colour = BorderColour[selected_border];
                  //break;
                case IDC_LEFTPUT:
                   current_colour = colour_table[left];
                  break;
                case IDC_RIGHTPUT:
                   current_colour = colour_table[right];
                  break;
              }
              SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, (WPARAM)true, GetBValue(current_colour));
              SetDlgItemText(hwndDlg, IDC_REDVAL, I2WStr(GetBValue(current_colour), t, ARRAYSIZE(t)));

              SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, (WPARAM)true, GetGValue(current_colour));
              SetDlgItemText(hwndDlg, IDC_GREENVAL, I2WStr(GetGValue(current_colour), t, ARRAYSIZE(t)));

              SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, (WPARAM)true, GetRValue(current_colour));
              SetDlgItemText(hwndDlg, IDC_BLUEVAL, I2WStr(GetRValue(current_colour), t, ARRAYSIZE(t)));
              DrawSolidColour(hwndDlg, current_colour, IDC_BITMAP_CURCOLOUR);  // draw the colour box
              return true;
            case IDC_FREQBARS:  // fall through
            case IDC_PEAK:    // fall through
            case IDC_VOLUME: {
              switch(LOWORD(wParam)) {
                case IDC_FREQBARS:
                  colour_table = FreqBarColour;
                  break;
                case IDC_PEAK:
                  colour_table = PeakColour;
                  break;
                case IDC_VOLUME:
                  colour_table = VolumeColour;
                  break;
              }
              DrawColourRamp(hwndDlg, colour_table);  // draw the ramp

              SetDlgItemText(hwndDlg, IDC_LEFTRVAL, I2WStr(GetBValue(colour_table[left]), t, ARRAYSIZE(t)));
              SetDlgItemText(hwndDlg, IDC_LEFTGVAL, I2WStr(GetGValue(colour_table[left]), t, ARRAYSIZE(t)));
              SetDlgItemText(hwndDlg, IDC_LEFTBVAL, I2WStr(GetRValue(colour_table[left]), t, ARRAYSIZE(t)));
              DrawSolidColour(hwndDlg, colour_table[left], IDC_BITMAP_LEFT);  // draw the colour box

              SetDlgItemText(hwndDlg, IDC_RIGHTRVAL, I2WStr(GetBValue(colour_table[right]), t, ARRAYSIZE(t)));
              SetDlgItemText(hwndDlg, IDC_RIGHTGVAL, I2WStr(GetGValue(colour_table[right]), t, ARRAYSIZE(t)));
              SetDlgItemText(hwndDlg, IDC_RIGHTBVAL, I2WStr(GetRValue(colour_table[right]), t, ARRAYSIZE(t)));
              DrawSolidColour(hwndDlg, colour_table[right], IDC_BITMAP_RIGHT);  // draw the colour box
              return true;
            }
          }
      }
      return false;
	default:
		return ConfigDialog_Common(hwndDlg, uMsg, wParam, lParam);
  }
}

// if the config win is open, update Profiles listbox
void SaveDialog_UpdateProfilesProperty(void)
{
	if(IsWindow(hwndCurrentConfigProperty))
		PostMessage(hwndCurrentConfigProperty, WM_UPDATE_PROFILE_LIST, 0, 0);
}

void SaveDialog_LoadProfile(HWND hwndDlg)
{
	wchar_t szProfile[cnProfileNameBufLen];
	// check for text entered, and get it into selected
	if(SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, WM_GETTEXT, cnProfileNameBufLen, (LPARAM)szProfile)) {
		// look for exact match first
		int submatch = (int)SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_FINDSTRINGEXACT, 0, (LPARAM)szProfile);
		if(submatch == CB_ERR)  // exact match not found, try sub string match
			submatch = (int)SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_FINDSTRING, 0, (LPARAM)szProfile);
		if(submatch != CB_ERR) {  // if match found, load it
			SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_SETCURSEL, submatch, 0);
			SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, WM_GETTEXT, cnProfileNameBufLen, (LPARAM)szProfile);
			//int e = LoadProfile(szProfile);
			LoadProfile(szProfile);
			SaveDialog_UpdateProfilesProperty();
		}
	}
	EndDialog(hwndDlg, IDC_LOAD);
}

// this is not meant to be displayed on a propertysheet
BOOL CALLBACK SaveDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
  wchar_t szProfile[cnProfileNameBufLen] = {0};

  switch(uMsg) {
    case WM_INITDIALOG: {
      DarkModeSetup(hwndDlg);
      // restrict maximum name length
      SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_LIMITTEXT, cnProfileNameBufLen - 1, 0);
	  // set the text
      SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, WM_SETTEXT, 0, (LPARAM)szCurrentProfile);
      // load the profile names into the combo box
  	  EnumProfilesToControl(hwndDlg, IDC_COMBOPROFILE, CB_ADDSTRING, CB_SETCURSEL);
      return true;
    }
    case WM_DESTROY:
      return true;
    case WM_COMMAND:
      switch(HIWORD(wParam)) {
        case BN_CLICKED:
          switch(LOWORD(wParam)) {
            case IDC_CLOSE:
            case IDCANCEL:
              EndDialog(hwndDlg, IDCANCEL);
              return true;
            case IDC_SAVE:
				// check for text entered and read it into buffer
				if(SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, WM_GETTEXT, cnProfileNameBufLen, (LPARAM)szProfile)) {
					bool bSave = true;
					// see if the profile already exists, if not then add it
					if(SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_FINDSTRINGEXACT, 0, (LPARAM)szProfile) == CB_ERR)
						SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_ADDSTRING, 0, (LPARAM)szProfile);
					else {
						wchar_t message[64 + cnProfileNameBufLen] = {0};
						_snwprintf(message, ARRAYSIZE(message), L"Are you sure you want to overwrite\n%s?", szProfile);
						if(MessageBox(hwndDlg, message, L"Confirm Save", MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
							bSave = false;
					}

					if(bSave) {
						SaveProfile(szProfile);
						SaveDialog_UpdateProfilesProperty();
					}
              }
              return true;
            case IDC_LOAD:
				SaveDialog_LoadProfile(hwndDlg);
				return true;
            case IDC_DELETE:
              // check for text and get the text entered (if any)
              if(SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, WM_GETTEXT, cnProfileNameBufLen, (LPARAM)szProfile)) {
                // see if the entered text even matches
                if(SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_FINDSTRINGEXACT, 0, (LPARAM)szProfile) != CB_ERR) {
					wchar_t message[64 + cnProfileNameBufLen] = {0};
                  _snwprintf(message, ARRAYSIZE(message), L"Are you sure you want to delete\n%s?", szProfile);
                  if(MessageBox(hwndDlg, message, L"Confirm Delete", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
                    DeleteProfile(szProfile);
					if(!wcscmp(szProfile, cszCurrentSettings)) {
					  const char *cszMessage = "The Truth Is Out There";
					  const char *cszCaption = "Believe";
					  UINT uType = MB_ICONINFORMATION;
					  switch(m_rand() % 3) {
					  case 1:
							cszMessage = "Oh my God, they killed Kenny!\nYou Bastards!";
							cszCaption = "Death";
							uType = MB_ICONEXCLAMATION;
							break;
					  case 2:
							cszMessage = "Are you pondering what I'm pondering?";
							cszCaption = "Brain";
							uType = MB_YESNO | MB_ICONQUESTION;
							break;
                      }
                      MessageBoxA(hwndDlg, cszMessage, cszCaption, uType);
                    }
                    int index = (int)SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_FINDSTRING, 0, (LPARAM)szProfile);
                    SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, CB_DELETESTRING, index, 0);
					SendDlgItemMessage(hwndDlg, IDC_COMBOPROFILE, WM_SETTEXT, 0, (LPARAM)cszCurrentSettings);
					SaveDialog_UpdateProfilesProperty();
                    //if(hwndConfigProfiles)  // TODO: if the config win is open, update it to force updating the Profiles listbox if it is being shown
					//PostMessage(hwndConfigProfiles, WM_UPDATE_PROFILE_LIST, 0, 0);
                    //RedrawWindow(config_win, NULL, NULL, RDW_INVALIDATE);
                  }
                }
              }
              return true;
            default: return false;
          }
        case CBN_DBLCLK:
          switch(LOWORD(wParam)) {
            case IDC_COMBOPROFILE:
				SaveDialog_LoadProfile(hwndDlg);
				return true;
            default: return false;
          }
        default: return false;
      }
    default:
    //MessageBox(hatan, "Unhandled WM Message", "Message", MB_ICONINFORMATION);
    //MessageBeep(MB_OK);
    return false;
  }
}

// propertysheet for selecting a profile
BOOL CALLBACK ProfileSelectDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
    case WM_SHOWWINDOW:
		if(!wParam)
			return TRUE;
		hwndCurrentConfigProperty = hwndDlg;
	case WM_INITDIALOG:
        DarkModeSetup(hwndDlg);
	case WM_UPDATE_PROFILE_LIST:
		SetDlgItemText(hwndDlg, IDC_STATICMESSAGE, szProfileMessage[0] ? szProfileMessage : cszDefaultProfileMessage);
		SendDlgItemMessage(hwndDlg, IDC_PROFILELIST, LB_RESETCONTENT, 0, 0);
		EnumProfilesToControl(hwndDlg, IDC_PROFILELIST, LB_ADDSTRING, LB_SETCURSEL);
		return TRUE;
    case WM_COMMAND:
      switch(HIWORD(wParam)) {
        case LBN_SELCHANGE:
        case LBN_DBLCLK:
          switch(LOWORD(wParam)) {
            case IDC_PROFILELIST:
			{
              // copies the selected (anchor item) text into select_profile
			  wchar_t szProfile[MAX_PATH] = {0};
              if(SendDlgItemMessage(hwndDlg, IDC_PROFILELIST, LB_GETTEXT, SendDlgItemMessage(hwndDlg, IDC_PROFILELIST, LB_GETANCHORINDEX, 0, 0), (LPARAM)szProfile) != LB_ERR) {
                LoadTempProfile(szProfile);
				SetDlgItemText(hwndDlg, IDC_STATICMESSAGE, szProfileMessage[0] ? szProfileMessage : cszDefaultProfileMessage);
              }
              return TRUE;
			}
            default: return FALSE;
          }
        default: return FALSE;
      }
	default:
		return ConfigDialog_Common(hwndDlg, uMsg, wParam, lParam);
  }
}

// stereo property sheet
/*BOOL CALLBACK StereoDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
	case WM_SHOWWINDOW:
		if(!wParam)
			return TRUE;
		hwndCurrentConfigProperty = hwndDlg;
    case WM_INITDIALOG:
	case WM_UPDATE_PROFILE_LIST:
		SendDlgItemMessage(hwndDlg, IDC_REVERSELEFT, reverseleft ? BM_SETCHECK : BST_UNCHECKED, (WPARAM)BST_CHECKED, 0);
		SendDlgItemMessage(hwndDlg, IDC_REVERSERIGHT, reverseright ? BM_SETCHECK : BST_UNCHECKED, (WPARAM)BST_CHECKED, 0);
		return TRUE;
	case WM_COMMAND:
		switch(HIWORD(wParam)) {
		case BN_CLICKED:
			switch(LOWORD(wParam)) {
			case IDC_REVERSELEFT:
				reverseleft = SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				CalculateVariables();
				return TRUE;
			case IDC_REVERSERIGHT:
				reverseright = SendDlgItemMessage(hwndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				CalculateVariables();
				return TRUE;
			}
		}
		return FALSE;
	default:
		return ConfigDialog_Common(hwndDlg, uMsg, wParam, lParam);
	}
}*/

/*
* file functions
*/
// makes an absolute path to the profile directory (not ending in backslash)
void GetProfilePath(wchar_t *szBuf)
{
#ifdef WACUP_BUILD
	CombinePath(szBuf, GetPaths()->settings_sub_dir, L"vis_classic");
#else
	PathCombine(szBuf, GetPaths()->settings_sub_dir, L"vis_classic");
#endif
}

// makes an absolute path to a profile ini file
void GetProfileINIFilename(wchar_t *szBuf, const wchar_t *cszProfile)
{
	if(!wcscmp(cszProfile, cszCurrentSettings)) {
		wcsncpy(szBuf, szMainIniFilename, MAX_PATH);
	}
	else {
		GetProfilePath(szBuf);
        AppendOnPath(szBuf, cszProfile);
        AddExtension(szBuf, L".ini");
	}
}

int ProfileFileExists(const wchar_t *cszFilename)
{
	WIN32_FIND_DATA fd = {0};
	HANDLE h = FindFirstFile(cszFilename, &fd);
	if(h != INVALID_HANDLE_VALUE) {
		FindClose(h);
		return 0;
	}
	return GetLastError();
}

void DeleteProfile(const wchar_t *cszProfile)
{
	wchar_t szBuf[MAX_PATH] = {0};
	GetProfileINIFilename(szBuf, cszProfile);
	if(DeleteFile(szBuf)) {
		// profile deleted, set the profile name to current settings
		SaveProfileNameEntry(cszCurrentSettings);
	}
}

HANDLE FindProfileFiles(LPWIN32_FIND_DATA pfd)
{
	wchar_t szPath[MAX_PATH] = {0};
	GetProfilePath(szPath);
    AppendOnPath(szPath, L"*.ini");
	return FindFirstFile(szPath, pfd);
}

void EnumProfilesToControl(HWND hDlg, int nIDDlgItem, UINT nMsg, UINT nSelMsg)
{
	WIN32_FIND_DATA fd = {0};
	HANDLE hFind = FindProfileFiles(&fd);
	if(hFind != INVALID_HANDLE_VALUE) {
		do {
			fd.cFileName[wcslen(fd.cFileName) - 4] = 0;
			int n = (int)SendDlgItemMessage(hDlg, nIDDlgItem, nMsg, 0, (LPARAM)fd.cFileName);
			if(nSelMsg && !_wcsicmp(fd.cFileName, szTempProfile))
				SendDlgItemMessage(hDlg, nIDDlgItem, nSelMsg, n, 0);
		} while(FindNextFile(hFind, &fd));
		FindClose(hFind);
	}
}

// get the next and previous profile names relative to the current profile
// the previous and next will wrap to first/last
// returns 0 on success, or GetLastError() value
int GetRelativeProfiles(const wchar_t *szCurrent, wchar_t *szPrevious, wchar_t *szNext)
{
	WIN32_FIND_DATA fd = {0};
	HANDLE hFind = FindProfileFiles(&fd);
	if(hFind != INVALID_HANDLE_VALUE) {
		// init next and previous
		if(szPrevious) *szPrevious = 0;
		if(szNext) *szNext = 0;
		// remove extension
		fd.cFileName[wcslen(fd.cFileName) - 4] = 0;
		// setup first and last
		wchar_t szFirst[MAX_PATH] = {0};
		wchar_t szLast[MAX_PATH] = {0};
		wcsncpy(szFirst, fd.cFileName, ARRAYSIZE(szFirst));
		wcsncpy(szLast, fd.cFileName, ARRAYSIZE(szLast));
		// need to check if the first file is valid for prev/next, so jump into the loop
		goto skipFind;
		while(FindNextFile(hFind, &fd)) {
			// remove extension
			fd.cFileName[wcslen(fd.cFileName) - 4] = 0;
			// check first/last
			if(_wcsicmp(fd.cFileName, szFirst) < 0) wcsncpy(szFirst, fd.cFileName, ARRAYSIZE(szFirst));
			if(_wcsicmp(fd.cFileName, szLast) > 0) wcsncpy(szLast, fd.cFileName, ARRAYSIZE(szLast));
skipFind:	// check prev/next
			if(szPrevious && (_wcsicmp(fd.cFileName, szPrevious) > 0 || !(*szPrevious)) && _wcsicmp(fd.cFileName, szCurrent) < 0) wcsncpy(szPrevious, fd.cFileName, MAX_PATH);
			if(szNext && (_wcsicmp(fd.cFileName, szNext) < 0 || !(*szNext)) && _wcsicmp(fd.cFileName, szCurrent) > 0) wcsncpy(szNext, fd.cFileName, MAX_PATH);
		}
		FindClose(hFind);
		// if current profile is first or last, then make prev/next wrap to first/last
		if(szPrevious && !_wcsicmp(szCurrent, szFirst)) wcsncpy(szPrevious, szLast, MAX_PATH);
		if(szNext && !_wcsicmp(szCurrent, szLast)) wcsncpy(szNext, szFirst, MAX_PATH);
		return 0;
	}
	return GetLastError();
}

unsigned int CountProfileFiles(void)
{
	unsigned int nCount = 0;
	WIN32_FIND_DATA fd = {0};
	HANDLE hFind = FindProfileFiles(&fd);
	if(hFind != INVALID_HANDLE_VALUE) {
		nCount++;
		while(FindNextFile(hFind, &fd))
			nCount++;
		FindClose(hFind);
	}
	return nCount;
}

/*
* file loading functions
*/

// loads the given profile
// returns 0 on success
int LoadProfileIni(const wchar_t *cszProfile)
{
	AtAnSt_Vis_mod.latencyMs = ReadPrivateProfileInt(cszIniMainSection, L"Latency", 10, 0, 1000, szMainIniFilename);
    bFlip = ReadPrivateProfileBool(cszIniMainSection, L"Flip", bFlip, szMainIniFilename);

	wchar_t szFilename[MAX_PATH] = {0};
	GetProfileINIFilename(szFilename, cszProfile);
	int error = ProfileFileExists(szFilename);
	if(error)
		return error;

	falloffrate = ReadPrivateProfileInt(cszIniMainSection, L"Falloff", falloffrate, 0, 75, szFilename);
	peakchangerate = ReadPrivateProfileInt(cszIniMainSection, L"PeakChange", peakchangerate, 0, 255, szFilename);
	requested_band_width = ReadPrivateProfileInt(cszIniMainSection, L"Bar Width", requested_band_width, 1, 50, szFilename);
	x_spacing = ReadPrivateProfileInt(cszIniMainSection, L"X-Spacing", x_spacing, 0, 10, szFilename);
	y_spacing = ReadPrivateProfileInt(cszIniMainSection, L"Y-Spacing",y_spacing, 1, 7, szFilename);
	backgrounddraw = ReadPrivateProfileInt(cszIniMainSection, L"BackgroundDraw", backgrounddraw, BACKGROUND_MIN, BACKGROUND_MAX, szFilename);
	barcolourstyle = ReadPrivateProfileInt(cszIniMainSection, L"BarColourStyle", barcolourstyle, BAR_MIN, BAR_MAX, szFilename);
	peakcolourstyle = ReadPrivateProfileInt(cszIniMainSection, L"PeakColourStyle", peakcolourstyle, PEAK_MIN, PEAK_MAX, szFilename);
	effect = ReadPrivateProfileInt(cszIniMainSection, L"Effect", effect, EFFECT_MIN, EFFECT_MAX, szFilename);
	peakleveleffect = ReadPrivateProfileInt(cszIniMainSection, L"Peak Effect", peakleveleffect, PEAK_EFFECT_MIN, PEAK_EFFECT_MAX, szFilename);
	peakfalloffmode = ReadPrivateProfileBool(cszIniMainSection, L"PeakFallOffMode", peakfalloffmode, szFilename);
	levelbase = ReadPrivateProfileInt(cszIniMainSection, L"Bar Level", levelbase, LEVEL_MIN, LEVEL_MAX, szFilename);
	reverseleft = ReadPrivateProfileBool(cszIniMainSection, L"ReverseLeft", reverseleft, szFilename);
	reverseright = ReadPrivateProfileBool(cszIniMainSection, L"ReverseRight", reverseright, szFilename);
	mono = ReadPrivateProfileBool(cszIniMainSection, L"Mono", mono, szFilename);
	bFftEqualize = ReadPrivateProfileBool(cszIniMainSection, L"FFTEqualize", bFftEqualize, szFilename);
	fFftEnvelope = ReadPrivateProfileFloat(cszIniMainSection, L"FFTEnvelope", fFftEnvelope, szFilename);
	fFftScale = ReadPrivateProfileFloat(cszIniMainSection, L"FFTScale", fFftScale, szFilename);
	szProfileMessage[0] = 0;
	GetPrivateProfileString(cszIniMainSection, L"Message", szProfileMessage, szProfileMessage, PROFILE_MESSAGE_STRING_LENGTH, szFilename);
	ReadPrivateProfileColourArray(L"BarColours", FreqBarColour, 256, szFilename);
	ReadPrivateProfileColourArray(L"PeakColours", PeakColour, 256, szFilename);
	ReadPrivateProfileColourArray(L"VolumeColours", VolumeColour, 256, szFilename);
	ReadPrivateProfileIntArray(L"VolumeFunction", volume_func, 256, szFilename);
	return 0;
}

// load a profile, if successful then update current settings
// returns 0 on success
int LoadProfile(const wchar_t *cszProfile)
{
	int e = LoadProfileIni(cszProfile);
	if(!e) {
		// profile loaded, update the entry and current settings
		SaveProfileNameEntry(cszProfile);
		CalculateAndUpdate();
	}
	return e;
}

// load temporary profile, current settings not updated
// returns 0 on success
int LoadTempProfile(const wchar_t *cszProfile)
{
	int e = LoadProfileIni(cszProfile);
	if(!e) {
		wcsncpy(szTempProfile, cszProfile, ARRAYSIZE(szTempProfile));
		CalculateAndUpdate();
	}
	return e;
}

// returns 0 on success
int LoadCurrentProfile(void)
{
	int error = 1;
	// get profile name
	int nProfileLen = GetPrivateProfileString(cszIniMainSection, L"Profile", cszDefaultSettingsName, szCurrentProfile, MAX_PATH, szMainIniFilename);
	// try to load the profile
	if(nProfileLen > 0) {
		error = LoadProfileIni(szCurrentProfile);
		if(!error) {
			wcsncpy(szTempProfile, szCurrentProfile, ARRAYSIZE(szTempProfile));
			CalculateAndUpdate();
		}
	}

	// if error, try current settings
	if(error)
		error = LoadProfile(cszCurrentSettings);

	// if error, try current settings
	if(error)
		error = LoadProfile(cszDefaultSettingsName);
	
	// if error, load the first profile
	if(error)
		error = LoadProfileNumber(0);
	return error;
}

// returns 0 on success
int LoadCurrentProfileOrCreateDefault(void)
{
	int e = LoadCurrentProfile();
	if(e) {
		DefaultSettings();
		e = SaveProfile(cszDefaultSettingsName);
	}
	return e;
}

void LoadNextProfile(void)
{
	wchar_t szProfile[MAX_PATH] = {0};
	if(!GetRelativeProfiles(szCurrentProfile, NULL, szProfile)) {
		//if(!strcmpi(szProfile, cszCurrentSettings))
			//GetRelativeProfiles(cszCurrentSettings, NULL, szProfile);
		LoadProfile(szProfile);
	}
}

void LoadPreviousProfile(void)
{
	wchar_t szProfile[MAX_PATH] = {0};
	if(!GetRelativeProfiles(szCurrentProfile, szProfile, NULL)) {
		//if(!strcmpi(szProfile, cszCurrentSettings))
			//GetRelativeProfiles(cszCurrentSettings, szProfile, NULL);
		LoadProfile(szProfile);
	}
}

// load a profile number, 0 based index
int LoadProfileNumber(unsigned int nProfileNumber)
{
	int e = 1;
	WIN32_FIND_DATA fd = {0};
	HANDLE hFind = FindProfileFiles(&fd);
	if(hFind != INVALID_HANDLE_VALUE) {
		unsigned int nCount = 0;
		while(nCount != nProfileNumber) {
			if(FindNextFile(hFind, &fd))
				nCount++;
			else
				break;
		}
		FindClose(hFind);
		_ASSERT(nCount == nProfileNumber);
		if(nCount == nProfileNumber) {
			fd.cFileName[wcslen(fd.cFileName) - 4] = 0;
			e = LoadProfile(fd.cFileName);
		}
	} else
		e = GetLastError();
	return e;
}

void LoadUserColours(void)
{
	ReadPrivateProfileColourArray(L"UserColours", UserColours, 16, szMainIniFilename);
}

void LoadWindowPostion(RECT *pr)
{
	pr->left = GetPrivateProfileInt(cszIniMainSection, L"WindowPosLeft", pr->left, szMainIniFilename);
	pr->right = GetPrivateProfileInt(cszIniMainSection, L"WindowPosRight", pr->right, szMainIniFilename);
	pr->top = GetPrivateProfileInt(cszIniMainSection, L"WindowPosTop", pr->top, szMainIniFilename);
	pr->bottom = GetPrivateProfileInt(cszIniMainSection, L"WindowPosBottom", pr->bottom, szMainIniFilename);
}

/*
* file saving functions
*/

int SaveProfileIni(const wchar_t *cszProfile)
{
    WritePrivateProfileInt(cszIniMainSection, L"Latency", AtAnSt_Vis_mod.latencyMs, szMainIniFilename);

	wchar_t szFilename[MAX_PATH] = {0};
	GetProfileINIFilename(szFilename, cszProfile);

	WritePrivateProfileInt(cszIniMainSection, L"Falloff", falloffrate, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"PeakChange", peakchangerate, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"Bar Width", requested_band_width, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"X-Spacing", x_spacing, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"Y-Spacing", y_spacing, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"BackgroundDraw", backgrounddraw, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"BarColourStyle", barcolourstyle, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"PeakColourStyle", peakcolourstyle, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"Effect", effect, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"Peak Effect", peakleveleffect, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"PeakFallOffMode", peakfalloffmode, szFilename);
	WritePrivateProfileInt(cszIniMainSection, L"Bar Level", levelbase, szFilename);

	WritePrivateProfileBool(cszIniMainSection, L"ReverseLeft", reverseleft, szFilename);
	WritePrivateProfileBool(cszIniMainSection, L"ReverseRight", reverseright, szFilename);
	WritePrivateProfileBool(cszIniMainSection, L"Mono", mono, szFilename);
	WritePrivateProfileBool(cszIniMainSection, L"FFTEqualize", bFftEqualize, szFilename);

	WritePrivateProfileFloat(cszIniMainSection, L"FFTEnvelope", fFftEnvelope, szFilename);
	WritePrivateProfileFloat(cszIniMainSection, L"FFTScale", fFftScale, szFilename);

	WritePrivateProfileColourArray(L"BarColours", FreqBarColour, 256, szFilename);
	WritePrivateProfileColourArray(L"PeakColours", PeakColour, 256, szFilename);
	WritePrivateProfileColourArray(L"VolumeColours", VolumeColour, 256, szFilename);

	WritePrivateProfileIntArray(L"VolumeFunction", volume_func, 256, szFilename);
	return 0;
}

// update the last profile
void SaveProfileNameEntry(const wchar_t *cszProfile)
{
	WritePrivateProfileString(cszIniMainSection, L"Profile", cszProfile, szMainIniFilename);
	wcsncpy(szCurrentProfile, cszProfile, ARRAYSIZE(szCurrentProfile));
	wcsncpy(szTempProfile, cszProfile, ARRAYSIZE(szTempProfile));
}

// save "Current Settings" file
int SaveTempCurrentSettings(void)
{
	//SaveProfileNameEntry(szTempProfile);
	SaveProfileNameEntry(cszCurrentSettings);
	return SaveProfileIni(cszCurrentSettings);
}

// save a profile and update the profile entry
int SaveProfile(const wchar_t *cszProfile)
{
	int e = SaveProfileIni(cszProfile);
	if(!e)
		SaveProfileNameEntry(cszProfile);
	return e;
}

/*
* more functions
*/

void ClearBackground()
{
  if(rgbbuffer)
    ZeroMemory(rgbbuffer, image_width * MAX_WIN_HEIGHT * sizeof(COLORREF));
}

void DrawLevelGraph(HWND hwndDlg, const int *table)
{
  // get the bitmap handle and select into a DC for processing
  HDC hdcScreen = GetDC(NULL);
  HDC mDC = CreateCompatibleDC(hdcScreen);
  ReleaseDC(NULL, hdcScreen);
  HBITMAP bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_LEVELGRAPH, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
  SelectObject(mDC, bitmap);
  //ReleaseDC(NULL, hdcScreen);

  // fill it black
  RECT rect = {0, 0, 256, 256};
  FillRectWithColour(mDC, &rect, RGB(0, 0, 0), TRUE);

  // draw the graph
  for(int i = 0; i < 256; i++)
    SetPixel(mDC, i, 255 - table[i], RGB(255, 255, 255));

  // copy it back to the window
  SendDlgItemMessage(hwndDlg, IDC_LEVELGRAPH, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);

  DeleteDC(mDC);  // temporary DC no longer needed

  //if(force_redraw)
	InvalidateRect(GetDlgItem(hwndDlg, IDC_LEVELGRAPH), NULL, FALSE);
    //InvalidateRect(hwndDlg, NULL, false); // force window to redraw, needed for NT
}

void DrawColourRamp(HWND hwndDlg, COLORREF *table)
{
  // get the bitmap handle and select into a DC for processing
  HDC hdcScreen = GetDC(NULL);
  HDC mDC = CreateCompatibleDC(hdcScreen);
  HBITMAP bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_BITMAP_RAMP, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
  SelectObject(mDC, bitmap);
  ReleaseDC(NULL, hdcScreen);

  // draw the graph
  for(int y = 0; y < 256; y++)
    for(int x = 0; x < 20; x++)
      SetPixel(mDC, x, 255 - y, RGB(GetBValue(table[y]), GetGValue(table[y]), GetRValue(table[y])));

  // copy it back to the window
  SendDlgItemMessage(hwndDlg, IDC_BITMAP_RAMP, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);

  DeleteDC(mDC);  // temporary DC no longer needed

  //if(force_redraw)
	InvalidateRect(GetDlgItem(hwndDlg, IDC_BITMAP_RAMP), NULL, false); // force window to redraw, needed for NT
    //InvalidateRect(hwndDlg, NULL, false); // force window to redraw, needed for NT
}

void DrawSolidColour(HWND hwndDlg, COLORREF colour, int identifier)
{
  // get the bitmap handle and select into a DC for processing
  RECT rect = {0, 0, 20, 20};
  HDC hdcScreen = GetDC(NULL);
  HDC mDC = CreateCompatibleDC(hdcScreen);
  HBITMAP bitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, identifier, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, 0);
  SelectObject(mDC, bitmap);
  ReleaseDC(NULL, hdcScreen);

  // fill it
  FillRectWithColour(mDC, &rect, RGB(GetBValue(colour), GetGValue(colour), GetRValue(colour)), TRUE);

  // copy it back to the window
  SendDlgItemMessage(hwndDlg, identifier, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) bitmap);

  DeleteDC(mDC);  // temporary DC no longer needed

  //if(force_redraw)
	InvalidateRect(GetDlgItem(hwndDlg, identifier), NULL, false); // force window to redraw, needed for NT
    //InvalidateRect(hwndDlg, NULL, false); // force window to redraw, needed for NT
}

// fades the colour table between 2 specified points (inclusive)
void FadeColours(COLORREF *colour_table, int left, int right)
{
  // if right = left, no need to fade (also causes division by 0)
  if(right != left) {
    const int low = left > right ? right : left;
    const int high = left > right ? left : right;
    double red = GetBValue(colour_table[low]);
    double green = GetGValue(colour_table[low]);
    double blue = GetRValue(colour_table[low]);
    const double redstep = (GetBValue(colour_table[high]) - red) / (high - low);
    const double greenstep = (GetGValue(colour_table[high]) - green) / (high - low);
    const double bluestep = (GetRValue(colour_table[high]) - blue) / (high - low);
    for(int i = low; i < high; i++, red += redstep, green += greenstep, blue += bluestep)
      colour_table[i] = bmpRGB((int)red, (int)green, (int)blue);
  }
}

// creates a nicely faded random ramp
void CreateRandomRamp(COLORREF *table)
{
  int low = 0;
  int high = (m_rand() % 45) + 5;

  // fades red
  table[low] = bmpRGB(m_rand() % 256, 0, 0);
  while(low < 255) {
    table[high] = bmpRGB(m_rand() % 256, 0, 0);
    FadeColours(table, low, high);
    low = high;
    high = low + (m_rand() % 45) + 5;
    if(high > 255)
      high = 255;
  };

  //fade green
  low = 0;
  high = (m_rand() % 45) + 5;
  table[low] = bmpRGB(GetBValue(table[low]), m_rand() % 256, 0);
  while(low < 255) {
    table[high] = bmpRGB(GetBValue(table[high]), m_rand() % 256, 0);
    FadeColours(table, low, high);
    low = high;
    high = low + (m_rand() % 45) + 5;
    if(high > 255)
      high = 255;
  };

  //fade blue
  low = 0;
  high = (m_rand() % 45) + 5;
  table[low] = bmpRGB(GetBValue(table[low]), GetGValue(table[low]), m_rand() % 256);
  while(low < 255) {
    table[high] = bmpRGB(GetBValue(table[high]), GetGValue(table[high]), m_rand() % 256);
    FadeColours(table, low, high);
    low = high;
    high = low + (m_rand() % 45) + 5;
    if(high > 255)
      high = 255;
  };
}

void AboutMessage(void)
{
#ifdef WACUP_BUILD
	// TODO localise
    wchar_t message[1024] = { 0 };
	_snwprintf(message, ARRAYSIZE(message), L"Classic Spectrum Analyzer plug-in originally\n"
               L"by Mike Lynch (Copyright © 2007-2018)\n\nUpdated by %s for\nWACUP (Copyright "
               L"© 2018-%s)\n\nhttps://github.com/WACUP/vis_classic\n\nBuild date: %s",
               WACUP_Author(), WACUP_Copyright(), TEXT(__DATE__));
	AboutMessageBox(hatan, message, L"Classic Spectrum Analyzer");
#else
	MessageBox(hatan, L"Classic Spectrum Analyzer\n\nCopyright © 2007 Mike Lynch\n\n"
			   L"mlynch@gmail.com", L"Classic Spectrum Analyzer", MB_ICONINFORMATION);
#endif
}
