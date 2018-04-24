#include <windows.h>
#include <tchar.h>
#include "util.h"

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

void
err(const TCHAR *info)
{
	warn(info);
	ExitProcess(1);
}

void
errx(const TCHAR *message)
{
	warnx(message);
	ExitProcess(1);
}
