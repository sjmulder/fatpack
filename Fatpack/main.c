#include <windows.h>
#include <tchar.h>
#include "../common/util.h"
#include "resource.h"

static INT_PTR CALLBACK
dialogproc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_CLOSE:
		DestroyWindow(wnd);
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int cmdshow)
{
	HWND dialog;
	MSG msg;
	BOOL ret;

	dialog = CreateDialog(instance, MAKEINTRESOURCE(IDD_FATPACK), NULL,
	    dialogproc);
	if (!dialog)
		err(_T("CreateDialog"));

	ShowWindow(dialog, cmdshow);

	while (ret = GetMessage(&msg, NULL, 0, 0) > 0) {
		if (!IsDialogMessage(dialog, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (ret == -1)
		err(_T("GetMessage()"));

	return 0;
}
