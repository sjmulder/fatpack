#include <windows.h>
#include <tchar.h>
#include "../common/util.h"
#include "resource.h"

static BOOL
getlbstring(HWND listbox, int idx, TCHAR *buf, size_t bufsz)
{
	LRESULT len;
	LRESULT res;

	len = SendMessage(listbox, LB_GETTEXTLEN, (WPARAM)idx, 0);
	if (len == LB_ERR) {
		warnx(_T("Failed to retrieve list box item text ")
		    _T("length."));
		return FALSE;
	}

	if ((size_t)(len + 1) >= bufsz) {
		warnx(_T("The list box item does not fit in the ")
		    _T("allocated buffer."));
		return FALSE;
	}

	res = SendMessage(listbox, LB_GETTEXT, (WPARAM)idx, (LPARAM)buf);
	if (res == LB_ERR) {
		warnx(_T("Failed to get list box entry text."));
		return FALSE;
	}

	return TRUE;
}

static void
addfile(HWND dialog)
{
	TCHAR path[4096];
	TCHAR *name;
	OPENFILENAME ofn;
	size_t pathlen, namelen;
	HWND listbox;

	if (!(listbox = GetDlgItem(dialog, IDC_EXELIST))) {
		warn(_T("Failed to get listbox handle"));
		return;
	}

	path[0] = _T('\0');

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = dialog;
	ofn.lpstrFilter =
	    _T("Executable (*.exe)\0*.exe\0")
	    _T("All Files (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = path;
	ofn.nMaxFile = LEN(path);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

	if (!GetOpenFileName(&ofn))
		return;

	pathlen = _tcsclen(path);
	name = &path[pathlen+1];

	if (*name) {
		/* Mulitiple selection: dir\0file\0file...
		   `path` is the directory
		   `name` the filename */
		path[pathlen] = _T('\\');
		while (*name) {
			namelen = _tcslen(name);
			memcpy(&path[pathlen+1], name,
			    sizeof(TCHAR) * (namelen + 1));
			SendMessage(listbox, LB_ADDSTRING, 0, (LPARAM)path);
			name += namelen+1;
		}
	} else {
		/* Single selection, `path` is the full path */
		SendMessage(listbox, LB_ADDSTRING, 0, (LPARAM)path);
	}
}

static void
removefile(HWND dialog)
{
	HWND listbox;
	LRESULT idx;

	if (!(listbox = GetDlgItem(dialog, IDC_EXELIST))) {
		warn(_T("Failed to get listbox handle"));
		return;
	}

	idx = SendMessage(listbox, LB_GETCURSEL, 0, 0);
	if (idx == LB_ERR)
		return;

	idx = SendMessage(listbox, LB_DELETESTRING, (WPARAM)idx, 0);
	if (idx == LB_ERR) {
		warnx(_T("Failed to remove list box item."));
		return;
	}
}

static void
movefile(HWND dialog, int offset)
{
	HWND listbox;
	LRESULT idx;
	LRESULT count;
	LRESULT res;
	TCHAR path[4096];

	if (!(listbox = GetDlgItem(dialog, IDC_EXELIST))) {
		warn(_T("Failed to get list box handle"));
		return;
	}

	idx = SendMessage(listbox, LB_GETCURSEL, 0, 0);
	if (idx == LB_ERR || idx + offset < 0)
		return;

	if (offset > 0) {
		count = SendMessage(listbox, LB_GETCOUNT, 0, 0);
		if (count == LB_ERR) {
			warnx(_T("Failed to get list box item count."));
			return;
		}

		if (idx + offset >= count)
			return;
	}

	if (!getlbstring(listbox, (int)idx, path, LEN(path)))
		return;

	res = SendMessage(listbox, LB_DELETESTRING, (WPARAM)idx, 0);
	if (idx == LB_ERR) {
		warnx(_T("The entry could not be removed from the list ")
		    _T("box."));
		return;
	}

	res = SendMessage(listbox, LB_INSERTSTRING, (WPARAM)(idx+offset),
	    (LPARAM)path);
	if (res == LB_ERR || res == LB_ERRSPACE) {
		warnx(_T("The entry could not be added to the list box."));
		return;
	}

	SendMessage(listbox, LB_SETCURSEL, (WPARAM)(idx+offset), 0);
}

static BOOL
packinit(const TCHAR *path)
{
	HINSTANCE instance;
	HANDLE resinfo;
	HGLOBAL reshandle;
	void *resdata;
	size_t resdatasz;
	HANDLE file;

	instance = GetModuleHandle(NULL);

	resinfo = FindResource(instance, MAKEINTRESOURCE(IDR_LOADER),
	    RT_RCDATA);
	if (!resinfo ||
	    !(reshandle = LoadResource(instance, resinfo)) ||
	    !(resdata = LockResource(reshandle)) ||
	    !(resdatasz = SizeofResource(instance, resinfo))) {
		warn(_T("Failed to load embbed launcher resource"));
		return FALSE;
	}

	file = CreateFile(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
	    FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (!file) {
		warn(_T("Failed to open temporary file for writing"));
		return FALSE;
	}

	if (!WriteFile(file, resdata, (DWORD)resdatasz, NULL, NULL)) {
		warn(_T("Failed to write data to temporary file"));
		CloseHandle(file);
		return FALSE;
	}

	CloseHandle(file);
	return TRUE;
}

static BOOL
packadd(HANDLE resupdate, int i, const TCHAR *path)
{
	HANDLE file;
	HANDLE mapping;
	HANDLE view;
	DWORD sz;
	BOOL ok;

	file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
	    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file) {
		warn(_T("Failed to open input file for reading"));
		return FALSE;
	}

	sz = GetFileSize(file, NULL);
	if (sz == INVALID_FILE_SIZE) {
		warn(_T("Failed to get input file size"));
		CloseHandle(file);
		return FALSE;
	}

	mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, (DWORD)sz,
	    NULL);
	if (!mapping) {
		warn(_T("Failed to create memory mapping for input file"));
		CloseHandle(file);
		return FALSE;
	}

	view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, (DWORD)sz);
	if (!view) {
		warn(_T("Failed to create mapping view of input file"));
		CloseHandle(mapping);
		CloseHandle(file);
		return FALSE;
	}

	ok = UpdateResource(resupdate, RT_RCDATA, MAKEINTRESOURCE(1000+i),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), view, (DWORD)sz);
	if (!ok) {
		warn(_T("Failed to update resource in output file"));
		UnmapViewOfFile(view);
		CloseHandle(mapping);
		CloseHandle(file);
		return FALSE;
	}

	UnmapViewOfFile(view);
	CloseHandle(mapping);
	CloseHandle(file);
	return TRUE;
}

static void
pack(HWND dialog)
{
	HINSTANCE instance;
	HWND listbox;
	LRESULT count;
	OPENFILENAME ofn;
	TCHAR path[4096];
	TCHAR tmpdir[MAX_PATH+1];
	TCHAR tmppath[MAX_PATH+1];
	HANDLE resupdate = NULL;
	TCHAR srcpath[4096];
	int i;

	tmppath[0] = '\0';
	instance = GetModuleHandle(NULL);

	if (!(listbox = GetDlgItem(dialog, IDC_EXELIST))) {
		warn(_T("Failed to get list box handle"));
		return;
	}

	count = SendMessage(listbox, LB_GETCOUNT, 0, 0);
	if (count == LB_ERR) {
		warnx(_T("Failed to get list box item count."));
		return;
	}

	if (!count) {
		warnx(_T("Add one or more executables to pack into a ")
		    _T("'fat' universal binary."));
		return;
	}

	path[0] = _T('\0');

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = dialog;
	ofn.hInstance = instance;
	ofn.lpstrFilter =
	    _T("Executable (*.exe)\0*.exe\0")
	    _T("All Files (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = path;
	ofn.nMaxFile = LEN(path);
	ofn.lpstrDefExt = _T("exe");
	ofn.Flags = OFN_OVERWRITEPROMPT;

	if (!GetSaveFileName(&ofn))
		return;

	if (!GetTempPath(LEN(tmpdir), tmpdir)) {
		warn(_T("Failed to get temporary directory path"));
		return;
	}

	if (!GetTempFileName(tmpdir, _T("Fpk"), 0, tmppath)) {
		warn(_T("Failed to get temporary file path"));
		return;
	}

	if (!packinit(tmppath))
		return;

	if (!(resupdate = BeginUpdateResource(tmppath, FALSE))) {
		warn(_T("Failed to open the temporary file for resource ")
		    _T("editing"));
		goto cleanup;
	}

	for (i = 0; i < count; i++) {
		if (!getlbstring(listbox, i, srcpath, LEN(srcpath)))
			goto cleanup;
		if (!packadd(resupdate, i, srcpath))
			goto cleanup;
	}

	EndUpdateResource(resupdate, FALSE);
	resupdate = NULL;

	if (!CopyFile(tmppath, path, FALSE))
		warn(_T("Failed to write output file"));

cleanup:
	if (resupdate)
		EndUpdateResource(resupdate, TRUE);
	if (tmppath[0])
		DeleteFile(tmppath);
}

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

	case WM_COMMAND:
		switch (wparam) {
		case IDC_ADD:
			addfile(wnd);
			return TRUE;
		case IDC_REMOVE:
			removefile(wnd);
			return TRUE;
		case IDC_MOVEUP:
			movefile(wnd, -1);
			return TRUE;
		case IDC_MOVEDOWN:
			movefile(wnd, 1);
			return TRUE;
		case IDC_PACK:
			pack(wnd);
			return TRUE;
		}
		break;
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
		err(_T("Failed to create main window"));

	if (!makeusedefault(dialog))
		err(_T("Failed to adjust window position"));

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
