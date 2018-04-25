/* Copyright (c) 2018, Sijmen J. Mulder. See LICENSE.md. */

#define LEN(a) (sizeof(a)/sizeof(*(a)))

void warn(const TCHAR *info);
void warnx(const TCHAR *message);
__declspec(noreturn) void err(const TCHAR *info);
__declspec(noreturn) void errx(const TCHAR *message);

BOOL makeusedefault(HWND window);
