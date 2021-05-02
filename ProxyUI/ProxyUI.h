#pragma once

#include "resource.h"
BOOL SetAutoRun(HWND hwnd);
BOOL SetNoAutoRun(HWND hwnd);
void BuildTrayIcon(HWND hwnd);
void DestroyTrayIcon(HWND hwnd);