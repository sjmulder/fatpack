#include <windows.h>
#include <tchar.h>
#include "util.h"

#define DUMMYCLASSNAME	_T("Dummy")

void
warn(const TCHAR *info)
{
	DWORD num;
	DWORD dw;
	TCHAR buf[4096];
	TCHAR *errstr;

	num = GetLastError();

	dw = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM, 0, num, 0, (LPTSTR)&errstr, 0, NULL);
	if (dw) {
		_sntprintf_s(buf, LEN(buf), _TRUNCATE,
		    _T("%s:\n\n%s\nError: 0x%X\n"), info, errstr, num);
		LocalFree(errstr);
	} else {
		_sntprintf_s(buf, LEN(buf), _TRUNCATE,
		    _T("%s:\n\nerror 0x%X\n"), info, num);
	}

	warnx(buf);
}

void
warnx(const TCHAR *message)
{
	MessageBox(NULL, message, _T(PROGNAME), MB_OK | MB_ICONEXCLAMATION);
}

__declspec(noreturn) void
err(const TCHAR *info)
{
	warn(info);
	ExitProcess(1);
}

__declspec(noreturn) void
errx(const TCHAR *message)
{
	warnx(message);
	ExitProcess(1);
}

BOOL
makeusedefault(HWND window)
{
	static int registered;

	HINSTANCE instance;
	WNDCLASS wc;
	RECT rect, tempwndrect;
	HWND tempwnd;
	BOOL ok;

	instance = GetModuleHandle(NULL);

	if (!registered) {
		ZeroMemory(&wc, sizeof(wc));
		wc.lpfnWndProc = DefWindowProc;
		wc.hInstance = instance;
		wc.lpszClassName = DUMMYCLASSNAME;

		if (!RegisterClass(&wc))
			return FALSE;
	}

	if (!GetWindowRect(window, &rect))
		return FALSE;

	tempwnd = CreateWindow(DUMMYCLASSNAME, _T(""), WS_OVERLAPPEDWINDOW,
	    CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
	    rect.bottom - rect.top, NULL, NULL, instance, 0);
	if (!tempwnd)
		return FALSE;

	if (!GetWindowRect(tempwnd, &tempwndrect))
		return FALSE;

	ok = SetWindowPos(window, NULL, tempwndrect.left, tempwndrect.top,
	    rect.right, rect.bottom, SWP_NOZORDER | SWP_NOACTIVATE);

	ok = DestroyWindow(tempwnd) && ok;
	return ok;
}
