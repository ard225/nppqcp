
#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H


#include "PluginInterface.h"

const TCHAR NPP_PLUGIN_NAME[] = TEXT("Quick Color Picker +");
const TCHAR NPP_PLUGIN_VER[] = TEXT("1.0");

const int _command_count = 3;

void AttachDll(HANDLE module);

void PluginInit();
void PluginCleanUp();


void LoadConfig();
void SaveConfig();

void InitCommandMenu();
void commandMenuCleanUp();
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);


////////////////////////////////////////////
void ToggleQCP();
void VisitWebsite();

HWND GetScintilla();

void CreateMessageWindow();
void DestroyMessageWindow();
LRESULT CALLBACK MessageWindowWinproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

void AddNppSubclass();
void RemoveNppSubclass();
LRESULT CALLBACK NppSubclassProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

void CreateColorPicker();
bool ShowColorPicker();
void HideColorPicker();
void ToggleColorPicker();
void WriteColorCodeToEditor(COLORREF color);

void LoadRecentColor();
void SaveRecentColor();

bool is_hex(char* str);
bool pad_hex(char* out, const char* in);

COLORREF revert_color_order(COLORREF color);
COLORREF hex2color(char* str);

void HighlightColorCode();
void RemoveColorHighlight();
char* substr(char* string, int begin, int end);


#endif //PLUGINDEFINITION_H
