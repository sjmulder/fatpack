#include <windows.h>
#include <tchar.h>

#define LEN(a)	(sizeof(a)/sizeof(*(a)))

const WCHAR * const executables[] = {
	_T("Sample ARM.exe"),
	_T("Sample x64.exe"),
	_T("Sample Win32.exe"),
};

int
main(void)
{
	int i;
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	BOOL ok;
	DWORD dw;
	DWORD err;
	TCHAR *errstr;
	
	_putts(_T("Launcher ") _T(PLATFORM));

	SetErrorMode(SEM_FAILCRITICALERRORS);

	ZeroMemory(&startup, sizeof(startup));
	startup.cb = sizeof(startup);

	for (i = 0; i < LEN(executables); i++) {
		ok = CreateProcess(executables[i], NULL, NULL, NULL, TRUE, 0,
		   NULL, NULL, &startup, &process);
		if (ok) {
			WaitForSingleObject(process.hProcess, INFINITE);
			CloseHandle(process.hProcess);
			CloseHandle(process.hThread);
			continue;
		}

		err = GetLastError();
		dw = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		    FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, (LPTSTR)&errstr, 0,
		    NULL);
		if (dw) {
			_tprintf(_T("Failed to start '%s': %s (0x%X)\n"),
			    executables[i], errstr, err);
			LocalFree(errstr);
		} else
			_tprintf(_T("Failed to start '%s': error 0x%X\n"),
			    executables[i], err);
	}

	return 0;
}
