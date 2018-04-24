#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include "../common/util.h"

static BOOL
tryrun(HINSTANCE instance, HANDLE resinfo, const TCHAR *exepath)
{
	HGLOBAL reshandle;
	HANDLE exefile;
	HANDLE exefilero;
	void *data;
	DWORD datasz;
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	BOOL ran;

	ZeroMemory(&startup, sizeof(startup));
	startup.cb = sizeof(startup);

	if (!(reshandle = LoadResource(instance, resinfo)) ||
	    !(data = LockResource(reshandle)) ||
	    !(datasz = SizeofResource(instance, resinfo)))
		err(_T("Failed to load embedded resource"));

	exefile = CreateFile(exepath, GENERIC_WRITE, FILE_SHARE_READ,
	    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL |
	    FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (exefile == INVALID_HANDLE_VALUE)
		err(_T("Failed to create temporary file"));
	if (!(WriteFile(exefile, data, datasz, NULL, NULL)))
		err(_T("Failed to write to temporary file"));

	exefilero = CreateFile(exepath, GENERIC_READ, FILE_SHARE_READ,
	    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
	    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);

	CloseHandle(exefile);

	ran = CreateProcess(exepath, NULL, NULL, NULL, TRUE, 0, NULL,
	    NULL, &startup, &process);
	if (!ran && GetLastError() != ERROR_EXE_MACHINE_TYPE_MISMATCH)
		err(_T("Failed to launch embedded executable"));

	if (ran) {
		WaitForSingleObject(process.hProcess, INFINITE);
		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
	}

	CloseHandle(exefilero);
	return ran;
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int cmdshow)
{
	int id;
	size_t sz;
	TCHAR modulepath[MAX_PATH+1];
	TCHAR tempdir[MAX_PATH+1];
	TCHAR exepath[MAX_PATH+1];
	HRSRC resinfo;

	if (!(GetModuleFileName(instance, modulepath, LEN(modulepath))))
		err(_T("Failed to get program filename"));
	if (!(GetTempPath(LEN(tempdir), tempdir)))
		err(_T("Failed to get temporary directory"));

	sz = _sntprintf_s(exepath, LEN(exepath), _TRUNCATE, _T("%s\\%s"),
	    tempdir, PathFindFileName(modulepath));
	if (sz == -1)
		err(_T("_sntprintf_s()"));

	/* Prevent error message box when we try to launch an incompatible
	   binary `*/
	SetErrorMode(SEM_FAILCRITICALERRORS);

	for (id = 1000; ; id++) {
		resinfo = FindResource(instance, MAKEINTRESOURCE(id),
		    RT_RCDATA);
		if (!resinfo)
			break;

		if (tryrun(instance, resinfo, exepath))
			return 0;
	}

	errx(_T("No suitable versions of this program are available for ")
	    _T("your system."));
}
