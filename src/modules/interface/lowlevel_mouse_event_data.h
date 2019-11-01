#pragma once
#include <Windows.h>

/*
  ll_keyboard - Lowlevel Mouse Hook

  The PowerToys runner installs low-level mouse hook using SetWindowsHookEx(WH_MOUSE_LL, ...)
  See https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexw
  for details.
*/

namespace {
	const wchar_t* ll_mouse = L"ll_mouse";
}

struct LowlevelMouseEvent {
	MSLLHOOKSTRUCT* lParam;
	WPARAM wParam;
};
