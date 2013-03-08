// Quick Color Picker plugin for Notepad++

#include "NppQCP.h"

#include "ColorPicker\ColorPicker.h"
#include "ColorPicker\ColorPicker.res.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

// The data of Notepad++ that you can use in your plugin commands
// extern variables - don't change the name
NppData nppData;
FuncItem funcItem[_command_count];
bool doCloseTag;


#define NPP_SUBCLASS_ID 101
#define MAX_COLOR_CODE_HIGHTLIGHT 50

const wchar_t _ini_section[] = L"nppqcp";
const wchar_t _ini_key[] = L"enabled";
const wchar_t _ini_file[] = L"nppqcp.ini";
TCHAR _ini_file_path[MAX_PATH];

// color picker /////////////////////////////////////////////
ColorPicker* _pColorPicker = NULL;
HINSTANCE _instance;
HWND _message_window;

bool _enable_qcp = false;
bool _is_color_picker_shown = false;

bool _rgb_selected = false;
int _rgb_start = -1;
int _rgb_end = -1;

int _last_select_start = -1;
int _last_select_end = -1;


// color code highlight /////////////////////////////////////
// search list - can't be large than max indicator count
int _indicator_count = INDIC_CONTAINER;
COLORREF _indicator_colors[INDIC_MAX+1];

void AttachDll(HANDLE module) {

	_instance = (HINSTANCE)module;

}

// Initialize plugin - will be called when setInfo(), after nppData is available
void PluginInit() {

	LoadConfig();

	CreateMessageWindow();
	AddNppSubclass();

	InitCommandMenu();

}

// End plugin
void PluginCleanUp() {

	SaveConfig();

	DestroyMessageWindow();
	RemoveNppSubclass();

}



////////////////////////////////////////
// Access config file
////////////////////////////////////////
void LoadConfig(){

	// get path of plugin configuration
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)_ini_file_path);

	if (PathFileExists(_ini_file_path) == FALSE) {
		::CreateDirectory(_ini_file_path, NULL);
	}

	PathAppend(_ini_file_path, _ini_file);

	// read in config
	int enabled = ::GetPrivateProfileInt(_ini_section, _ini_key, 5, _ini_file_path);
	if(enabled == 5)
		enabled = 1;
	_enable_qcp = ( enabled == 1);

}

void SaveConfig(){

	::WritePrivateProfileString(
		_ini_section, _ini_key,
		_enable_qcp ? L"1" : L"0",
		_ini_file_path
	);

}


////////////////////////////////////////
// Menu Command
////////////////////////////////////////
void InitCommandMenu() {

    // setCommand(int index,                      // zero based number to indicate the order of command
    //            wchar_t *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool checkOnInit                // optional. Make this menu item be checked visually
    //            );

	setCommand(0, L"Enable Quick Color Picker", ToggleQCP, NULL, _enable_qcp);

	setCommand(1, L"---", NULL, NULL, false);

	wchar_t text[200] = L"Visit Website ";
	wcscat(text, L" (Version ");
	wcscat(text, NPP_PLUGIN_VER);
	wcscat(text, L")");
	setCommand(2, text, VisitWebsite, NULL, FALSE);

}

bool setCommand(size_t index, wchar_t *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool checkOnInit) {

    if (index >= _command_count)
        return false;

    if (!pFunc)
        return false;

    wcscpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = checkOnInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void commandMenuCleanUp() {

}


///////////////////////////////////////////////////////////
// MENU COMMANDS
///////////////////////////////////////////////////////////

// toggle QCP plugins
void ToggleQCP() {

	_enable_qcp = !_enable_qcp;
	
	if (_enable_qcp) {
		HighlightColorCode();
	} else {
		RemoveColorHighlight();
		HideColorPicker();
	}

	::CheckMenuItem(::GetMenu(nppData._nppHandle), funcItem[0]._cmdID, MF_BYCOMMAND | (_enable_qcp ? MF_CHECKED : MF_UNCHECKED));

}

// visit nppqcp website
void VisitWebsite() {

	wchar_t url[200] = L"http://code.google.com/p/nppqcp/";
	::ShellExecute(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);

}


///////////////////////////////////////////////////////////
//  MESSAGE WINDOW
///////////////////////////////////////////////////////////

// create a background window for message communication
void CreateMessageWindow() {
	
	static wchar_t szWindowClass[] = L"npp_qcp_msgwin";
	
	WNDCLASSEX wc    = {0};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = MessageWindowWinproc;
	wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = _instance;
	wc.lpszClassName = szWindowClass;

	if (!RegisterClassEx(&wc)) {
		throw std::runtime_error("NppQCP: RegisterClassEx() failed");
	}

	_message_window = CreateWindowEx(0, szWindowClass, szWindowClass, WS_POPUP,	0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

	if (!_message_window) {
		throw std::runtime_error("NppQCP: CreateWindowEx() function returns null");
	}

}

void DestroyMessageWindow() {

	::DestroyWindow(_message_window);
	::UnregisterClass(L"npp_qcp_msgwin", _instance);

}

// message proccessor
LRESULT CALLBACK MessageWindowWinproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_QCP_PICK:
		{
			::SetActiveWindow(nppData._nppHandle);
			WriteColor((COLORREF)wparam);
			break;
		}
		case WM_QCP_CANCEL:
		{
			::SetActiveWindow(nppData._nppHandle);
			break;
		}
		case WM_QCP_START_SCREEN_PICKER:
		{
			::SetWindowPos(nppData._nppHandle, HWND_BOTTOM, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
		}
		case WM_QCP_END_SCREEN_PICKER:
		{
			::ShowWindow(nppData._nppHandle, SW_SHOW);
		}
		default:
		{
			return TRUE;
		}
	}

	return TRUE;

}


// Get current scintilla comtrol hwnd
HWND GetScintilla() {

    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return NULL;

    HWND h_scintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
	
	return h_scintilla;

}

////////////////////////////////////////////////////////////////////////////////
// NPP SUBCLASSING
////////////////////////////////////////////////////////////////////////////////

void AddNppSubclass(){

	::SetWindowSubclass(nppData._scintillaMainHandle, NppSubclassProc, NPP_SUBCLASS_ID, NULL);
	::SetWindowSubclass(nppData._scintillaSecondHandle, NppSubclassProc, NPP_SUBCLASS_ID, NULL);

}

void RemoveNppSubclass(){
	
	::RemoveWindowSubclass(nppData._scintillaMainHandle, NppSubclassProc, NPP_SUBCLASS_ID);
	::RemoveWindowSubclass(nppData._scintillaSecondHandle, NppSubclassProc, NPP_SUBCLASS_ID);

}

LRESULT CALLBACK NppSubclassProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	
	switch (message) {
		case WM_KEYDOWN:
			// hide the palette when user is typing or copy/paste
			HideColorPicker();
			break;
	}

	return DefSubclassProc(hwnd, message, wparam, lparam);

}


////////////////////////////////////////////////////////////////////////////////
// COLOR PALETTE POPUP
////////////////////////////////////////////////////////////////////////////////

void CreateColorPicker(){

	_pColorPicker = new ColorPicker();
	_pColorPicker->focus_on_show = false;
	_pColorPicker->Create(_instance, nppData._nppHandle, _message_window);

	LoadRecentColor();

}

bool ShowColorPicker(){

	if (!_enable_qcp)
		return false;

	HWND h_scintilla = GetScintilla();

	// detect hex color code
	int selection_start = ::SendMessage(h_scintilla, SCI_GETSELECTIONSTART, 0, 0);
	int selection_end = ::SendMessage(h_scintilla, SCI_GETSELECTIONEND, 0, 0);

	// if this selection hasn't changed
	if(_last_select_start == selection_start && _last_select_start == selection_end)
		return false;

	_last_select_start = selection_start;
	_last_select_start = selection_end;

	_rgb_selected = false;
	bool result = CheckForHexColor(h_scintilla, selection_start, selection_end);

	if(!result){
		result = CheckForRgbColor(h_scintilla, selection_start, selection_end);
		_rgb_selected = result;
	}

	if(!result)
		return false;

	// prepare coordinates
	POINT p;
	p.x = ::SendMessage(h_scintilla, SCI_POINTXFROMPOSITION , 0, selection_start);
	p.y = ::SendMessage(h_scintilla, SCI_POINTYFROMPOSITION , 0, selection_start);
	::ClientToScreen(h_scintilla, &p);
	 // all line height in scintilla is the same
	int line_height = ::SendMessage(h_scintilla, SCI_TEXTHEIGHT , 0, 1);

	RECT rc;
	rc.top = p.y;
	rc.right = p.x; // not used anyway
	rc.bottom = p.y + line_height;
	rc.left = p.x;
	
	_pColorPicker->SetParentRect(rc);

	// set and show
	_pColorPicker->Show();

	return true;

}


bool CheckForHexColor(const HWND h_scintilla, const int start, const int end){
	
	int len = end - start;

	// break - wrong length: fc6 ffcc66
	if (len != 3 && len != 6)
		return false;

	char prev_char = (char)::SendMessage(h_scintilla, SCI_GETCHARAT, start - 1, 0);
	// break - no # mark
	if (prev_char == 0 || prev_char != '#')
		return false;

	char next_char = (char)::SendMessage(h_scintilla, SCI_GETCHARAT, end, 0);
	// break - next char is still hex char
	if (strchr("01234567890abcdefABCDEF", next_char) != NULL)
		return false;

	// passed -

	// create the color picker if not created
	if (!_pColorPicker)
		CreateColorPicker();

	// get hex text - scintilla only accept char
	char hex_str[10];
    ::SendMessage(h_scintilla, SCI_GETSELTEXT, 0, (LPARAM)&hex_str);

	wchar_t hex_color[20];

	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, hex_str, -1, hex_color, sizeof(hex_str));

	// put current color to picker
	if (!_pColorPicker->SetHexColor(hex_color)) {
		// not a valid hex color string
		return false;
	}

	return true;

}

bool CheckForRgbColor(const HWND h_scintilla, const int start, const int end){
	
	int len = end - start;

	// break - wrong length: rgb rgba
	if (len != 3 && len != 4)
		return false;

	char next_char = (char)::SendMessage(h_scintilla, SCI_GETCHARAT, end, 0);

	// break - next char should be the open bracket
	if (next_char != '(')
		return false;

	int line = ::SendMessage(h_scintilla, SCI_LINEFROMPOSITION, end, 0);
	int line_end = ::SendMessage(h_scintilla, SCI_GETLINEENDPOSITION, line, 0);

	// get first close bracket position
	char close_bracket[] = ")";
	::SendMessage(h_scintilla, SCI_SETTARGETSTART, end, 0);
	::SendMessage(h_scintilla, SCI_SETTARGETEND, line_end, 0);
	::SendMessage(h_scintilla, SCI_SETSEARCHFLAGS, 0, 0);
    ::SendMessage(h_scintilla, SCI_SEARCHANCHOR, 0, 0);
	int close_pos = ::SendMessage(h_scintilla, SCI_SEARCHINTARGET, strlen(close_bracket), (LPARAM)close_bracket);

	// no close bracket
	if(close_pos == -1)
		return false;

	char regexp[] = "(?i:rgb\\(|rgba\\()([0-9]{1,3})(?:[ ]*,[ ]*)([0-9]{1,3})(?:[ ]*,[ ]*)([0-9]{1,3})(?:[ )]|,[0-9. ]+\\))";
	::SendMessage(h_scintilla, SCI_SETTARGETSTART, start, 0);
	::SendMessage(h_scintilla, SCI_SETTARGETEND, close_pos+1, 0);
	::SendMessage(h_scintilla, SCI_SETSEARCHFLAGS, SCFIND_REGEXP, 0);
    ::SendMessage(h_scintilla, SCI_SEARCHANCHOR, 0, 0);
	int target_pos = ::SendMessage(h_scintilla, SCI_SEARCHINTARGET, strlen(regexp), (LPARAM)regexp);

	// no match
	if(target_pos == -1)
		return false;

	char r[6], g[6], b[6];
	::SendMessage(h_scintilla, SCI_GETTAG, 1, (LPARAM)&r);
	::SendMessage(h_scintilla, SCI_GETTAG, 2, (LPARAM)&g);
	::SendMessage(h_scintilla, SCI_GETTAG, 3, (LPARAM)&b);

	int ir = strtol(r, NULL, 10);
	int ig = strtol(g, NULL, 10);
	int ib = strtol(b, NULL, 10);

	// invalid color value
	if(ir>255 || ig>255 || ib>255)
		return false;

	// passed -
	COLORREF color = RGB(ir, ig, ib);

	// prepare seletion range for replacement
	_rgb_start = end + 1;
	_rgb_end = close_pos;

	// create the color picker if not created
	if (!_pColorPicker)
		CreateColorPicker();

	// put current color to picker
	_pColorPicker->Color(color);

	return true;

}

// hide the palette
void HideColorPicker() {
	
	if (!_pColorPicker)
		return;

	if (!_pColorPicker->IsVisible())
		return;

	_pColorPicker->Hide();

}


void ToggleColorPicker(){

	if (!_pColorPicker)
		CreateColorPicker();

	if (!_pColorPicker->IsVisible()) {
		ShowColorPicker();
	} else {
		HideColorPicker();
	}

}


void WriteColor(COLORREF color) {

	// convert to rgb color
	color = RGB(GetBValue(color),GetGValue(color),GetRValue(color));

	HWND h_scintilla = GetScintilla();

	char buff[100];

	if(_rgb_selected){
		sprintf(buff, "%d, %d, %d", GetRValue(color), GetGValue(color), GetBValue(color));
		::SendMessage(h_scintilla, SCI_SETSELECTIONSTART, _rgb_start, 0);
		::SendMessage(h_scintilla, SCI_SETSELECTIONEND, _rgb_end, 0);
	}else{
		sprintf(buff, "%0*X", 6, color);
	}
		
	::SendMessage(h_scintilla, SCI_REPLACESEL, NULL, (LPARAM)(char*)buff);

	::SetActiveWindow(h_scintilla);

	SaveRecentColor();

}

// load & save recent used colors
void LoadRecentColor(){

	if (!_pColorPicker)
		return;

	COLORREF colors[10];
	wchar_t key[20];

	for (int i=0; i<10; i++) {
		wsprintf(key, L"recent%d", i);
		colors[i] = ::GetPrivateProfileInt(_ini_section, key, 0, _ini_file_path);		
	}

	_pColorPicker->SetRecentColor(colors);
}

void SaveRecentColor(){
	
	if(!_pColorPicker)
		return;

	COLORREF colors[10];
	COLORREF* p_colors = colors;
	_pColorPicker->GetRecentColor(p_colors);

	wchar_t color[10];
	wchar_t key[20];

	for (int i=0; i<10; i++) {
		wsprintf(color, L"%d", colors[i]);
		wsprintf(key, L"recent%d", i);
		::WritePrivateProfileString(_ini_section, key, color, _ini_file_path);
	}

}



////////////////////////////////////////////////////////////////////////////////
// highlight hex color
////////////////////////////////////////////////////////////////////////////////
void HighlightColorCode() {

	if(!_enable_qcp)
		return;

	int lang = -100;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&lang);

	if (lang != L_CSS && lang != L_JS && lang != L_HTML) {
        return;
    }
    
    RemoveColorHighlight();

    HWND h_scintilla = GetScintilla();

    //SCI_GETFIRSTVISIBLELINE first line is 0
    int first_visible_line = ::SendMessage(h_scintilla, SCI_GETFIRSTVISIBLELINE, 0, 0);
	int last_line = first_visible_line + (int)::SendMessage(h_scintilla, SCI_LINESONSCREEN, 0, 0);

	first_visible_line = first_visible_line - 1; // i don't know why

    int start_position = ::SendMessage(h_scintilla, SCI_POSITIONFROMLINE, first_visible_line, 0);
	int end_position = ::SendMessage(h_scintilla, SCI_GETLINEENDPOSITION, last_line, 0);

	HighlightHexColor(h_scintilla, start_position, end_position);
	HighlightRgbColor(h_scintilla, start_position, end_position);

}

void HighlightHexColor(const HWND h_scintilla, const int start_position, const int end_position){

	int match_count = 0;
	int search_start = start_position;

    while (match_count < MAX_COLOR_CODE_HIGHTLIGHT && search_start < end_position) {

		char regexp[] = "(?:#)([0-9abcdefABCDEF]{3,6})(?:[^0-9abcdefABCDEF])";

		::SendMessage(h_scintilla, SCI_SETTARGETSTART, search_start, 0);
		::SendMessage(h_scintilla, SCI_SETTARGETEND, end_position, 0);
		::SendMessage(h_scintilla, SCI_SETSEARCHFLAGS, SCFIND_REGEXP, 0);
        ::SendMessage(h_scintilla, SCI_SEARCHANCHOR, 0, 0);
		int target_pos = ::SendMessage(h_scintilla, SCI_SEARCHINTARGET, strlen(regexp), (LPARAM)regexp);

		// not found
		if(target_pos == -1) {
			break;
		}

		char hex_color[8];
		::SendMessage(h_scintilla, SCI_GETTAG, 1, (LPARAM)&hex_color);

        int target_length = strlen(hex_color);
        int target_start = target_pos;
        int target_end = target_pos + target_length + 1; // don't forget the '#'


		// invalid hex color length
        if (target_length !=3 && target_length != 6) {
			search_start = target_end; // move on
            continue;
        }

		// pad 3 char hex string
        if (target_length == 3) {
			hex_color[6] = '\0';
            hex_color[5] = hex_color[2];
            hex_color[4] = hex_color[2];
            hex_color[3] = hex_color[1];
            hex_color[2] = hex_color[1];
            hex_color[1] = hex_color[0];
            hex_color[0] = hex_color[0];
        }

        // parse hex color string to COLORREF
		COLORREF color = strtol(hex_color, NULL, 16);
		color = RGB(GetBValue(color),GetGValue(color),GetRValue(color));

		bool can_proceed = HighlightCode(h_scintilla, color, target_start, target_end);

		// exceeded the indicator count
		if(!can_proceed)
			break;

		search_start = target_end; // move on
        match_count++;

    }

}

void HighlightRgbColor(const HWND h_scintilla, const int start_position, const int end_position){

	int match_count = 0;
	int search_start = start_position;

    while (match_count < MAX_COLOR_CODE_HIGHTLIGHT && search_start < end_position) {

		char regexp[] = "(?i:rgb\\(|rgba\\()([0-9]{1,3})(?:[ ]*,[ ]*)([0-9]{1,3})(?:[ ]*,[ ]*)([0-9]{1,3})(?:[^)]*\\))";

		::SendMessage(h_scintilla, SCI_SETTARGETSTART, search_start, 0);
		::SendMessage(h_scintilla, SCI_SETTARGETEND, end_position, 0);
		::SendMessage(h_scintilla, SCI_SETSEARCHFLAGS, SCFIND_REGEXP, 0);
        ::SendMessage(h_scintilla, SCI_SEARCHANCHOR, 0, 0);
		int target_pos = ::SendMessage(h_scintilla, SCI_SEARCHINTARGET, strlen(regexp), (LPARAM)regexp);

		// not found
		if(target_pos == -1) {
			break;
		}

		char r[6], g[6], b[6];
		::SendMessage(h_scintilla, SCI_GETTAG, 1, (LPARAM)&r);
		::SendMessage(h_scintilla, SCI_GETTAG, 2, (LPARAM)&g);
		::SendMessage(h_scintilla, SCI_GETTAG, 3, (LPARAM)&b);

		int ir = strtol(r, NULL, 10);
		int ig = strtol(g, NULL, 10);
		int ib = strtol(b, NULL, 10);

        int target_start = target_pos;
        int target_end = ::SendMessage(h_scintilla, SCI_GETTARGETEND, 0, 0);
        int target_length = target_end - target_start;

		// invalid color value
		if(ir>255 || ig>255 || ib>255){
			search_start = target_end; // move on
			continue;
		}

        // parse hex color string to COLORREF
		COLORREF color = RGB(ir, ig, ib);

		bool can_proceed = HighlightCode(h_scintilla, color, target_start, target_end);

		// exceeded the indicator count
		if(!can_proceed)
			break;

		search_start = target_end; // move on
        match_count++;

    }

}


bool HighlightCode(const HWND h_scintilla, const COLORREF color, const int start, int end){

	if(_indicator_count > INDIC_MAX)
		return false;

	int length = end - start;

	// mark text with indicateors
    int indicator_id = -1;

	// search for exist indicator with same color
	int index = INDIC_CONTAINER;
    while (index <= _indicator_count) {
        if (color == _indicator_colors[index]) {
            indicator_id = index;
            break;
        }
        index++;
    }

	// not found - create new indicator
    if (indicator_id == -1 ) {

		indicator_id = _indicator_count;

		::SendMessage(h_scintilla, SCI_INDICSETSTYLE, indicator_id, INDIC_PLAIN);
		::SendMessage(h_scintilla, SCI_INDICSETUNDER, indicator_id, (LPARAM)true);
		::SendMessage(h_scintilla, SCI_INDICSETFORE, indicator_id, (LPARAM)color);
		::SendMessage(h_scintilla, SCI_INDICSETALPHA, indicator_id, (LPARAM)255);

		_indicator_colors[_indicator_count] = color;
        _indicator_count++;

    }

	// set indicator
    ::SendMessage(h_scintilla, SCI_SETINDICATORCURRENT, indicator_id, 0);
    ::SendMessage(h_scintilla, SCI_INDICATORFILLRANGE, start, length);

	return true;

}

void RemoveColorHighlight() {

    HWND h_scintilla = GetScintilla();
    
    int length = ::SendMessage(h_scintilla, SCI_GETTEXT, 0, 0);
    int i = INDIC_CONTAINER;
    while (i <= _indicator_count) {
		_indicator_colors[i] = -1;
        ::SendMessage(h_scintilla, SCI_SETINDICATORCURRENT, i, 0);
        ::SendMessage(h_scintilla, SCI_INDICATORCLEARRANGE, 0, length);
        i++;
    }

	_indicator_count = INDIC_CONTAINER;

}
