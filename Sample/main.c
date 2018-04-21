#include <windows.h>
#include <tchar.h>

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int cmdshow)
{
	MessageBox(NULL, _T("Hello from " PLATFORM "!"), _T("Sample"),
	    MB_OK | MB_ICONINFORMATION);

	return 0;
}
