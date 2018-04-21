#include <windows.h>
#include <tchar.h>
#include "util.h"

void
err(const TCHAR *info)
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

	MessageBox(NULL, buf, _T("Loader"), MB_OK | MB_ICONEXCLAMATION);
	ExitProcess(1);
}
