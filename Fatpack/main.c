#include <windows.h>
#include <tchar.h>
#include "../common/util.h"
#include "resource.h"

static void
addfile(HWND dialog)
{
	TCHAR path[4096];
	TCHAR *name;
	OPENFILENAME ofn;
	size_t pathlen, namelen;
	HWND listbox;

	if (!(listbox = GetDlgItem(dialog, IDC_EXELIST)))
		err(_T("GetDlgItem(IDC_EXELIST)"));

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
		warnx(_T("Failed to get listbox handle"));
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
	LRESULT pathlen;
	LRESULT res;
	const TCHAR path[4096];

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

	pathlen = SendMessage(listbox, LB_GETTEXTLEN, (WPARAM)idx, 0);
	if (pathlen == LB_ERR) {
		warnx(_T("Failed to retrieve list box item text length."));
		return;
	}

	if (pathlen+1 >= LEN(path)) {
		warnx(_T("The list box item does not fit in the allocated ")
		    _T("buffer."));
		return;
	}

	res = SendMessage(listbox, LB_GETTEXT, (WPARAM)idx, (LPARAM)path);
	if (res == LB_ERR) {
		warnx(_T("Failed to get list box entry text."));
		return;
	}

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

static void
pack(HWND dialog)
{
	HINSTANCE instance;
	HWND listbox;
	LRESULT count;
	OPENFILENAME ofn;
	TCHAR path[4096];
	HANDLE resinfo;
	HGLOBAL reshandle;
	void *resdata;
	size_t resdatasz;
	TCHAR tmpdir[MAX_PATH+1];
	TCHAR tmppath[MAX_PATH+1];
	HANDLE tmpfile;
	HANDLE resupdate;
	LRESULT srcpathlen;
	TCHAR srcpath[4096];
	HANDLE srcfile;
	HANDLE srcfilemap;
	HANDLE srcfileview;
	DWORD srcfilesz;
	int i;
	LRESULT res;
	BOOL ok;

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

	resinfo = FindResource(instance, MAKEINTRESOURCE(IDR_LOADER),
	    RT_RCDATA);
	if (!resinfo ||
	    !(reshandle = LoadResource(instance, resinfo)) ||
	    !(resdata = LockResource(reshandle)) ||
	    !(resdatasz = SizeofResource(instance, resinfo))) {
		warn(_T("Failed to load embbed launcher resource"));
		return;
	}

	if (!GetTempPath(LEN(tmpdir), tmpdir)) {
		warn(_T("Failed to get temporary directory path"));
		return;
	}

	if (!GetTempFileName(tmpdir, _T("Fpk"), 0, tmppath)) {
		warn(_T("Failed to get temporary file path"));
		return;
	}

	tmpfile = CreateFile(tmppath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
	    FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (!tmpfile) {
		warn(_T("Failed to open temporary file for writing"));
		return;
	}

	if (!WriteFile(tmpfile, resdata, (DWORD)resdatasz, NULL, NULL)) {
		warn(_T("Failed to write data to temporary file"));
		DeleteFile(tmppath);
		return;
	}

	CloseHandle(tmpfile);

	if (!(resupdate = BeginUpdateResource(tmppath, FALSE))) {
		warn(_T("Failed to open the temporary file for resource ")
		    _T("editing"));
		DeleteFile(tmppath);
		return;
	}

	for (i = 0; i < count; i++) {
		srcpathlen = SendMessage(listbox, LB_GETTEXTLEN, (WPARAM)i,
		    0);
		if (srcpathlen == LB_ERR) {
			warnx(_T("Failed to retrieve list box item text ")
			    _T("length."));
			EndUpdateResource(resupdate, TRUE);
			DeleteFile(tmppath);
			return;
		}

		if (srcpathlen +1 >= LEN(srcpath)) {
			warnx(_T("The list box item does not fit in the ")
			    _T("allocated buffer."));
			EndUpdateResource(resupdate, TRUE);
			DeleteFile(tmppath);
			return;
		}

		res = SendMessage(listbox, LB_GETTEXT, (WPARAM)i,
		    (LPARAM)srcpath);
		if (res == LB_ERR) {
			warnx(_T("Failed to get list box entry text."));
			EndUpdateResource(resupdate, TRUE);
			DeleteFile(tmppath);
			return;
		}

		srcfile = CreateFile(srcpath, GENERIC_READ, FILE_SHARE_READ,
		    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!srcfile) {
			warn(_T("Failed to open input file for reading"));
			EndUpdateResource(resupdate, TRUE);
			DeleteFile(tmppath);
			return;
		}

		srcfilesz = GetFileSize(srcfile, NULL);
		if (srcfilesz == INVALID_FILE_SIZE) {
			warn(_T("Failed to get input file size"));
			EndUpdateResource(resupdate, TRUE);
			DeleteFile(tmppath);
			return;
		}

		srcfilemap = CreateFileMapping(srcfile, NULL, PAGE_READONLY,
		    0, (DWORD)srcfilesz, NULL);
		if (!srcfilemap) {
			warn(_T("Failed to create memory mapping for input ")
			    _T("file"));
			CloseHandle(srcfile);
			EndUpdateResource(resupdate, TRUE);
			DeleteFile(tmppath);
			return;
		}

		srcfileview = MapViewOfFile(srcfilemap, FILE_MAP_READ, 0, 0,
		    (DWORD)srcfilesz);
		if (!srcfileview) {
			warn(_T("Failed to create memory mapping view of ")
			    _T("input file"));
			CloseHandle(srcfilemap);
			CloseHandle(srcfile);
			EndUpdateResource(resupdate, TRUE);
			DeleteFile(tmppath);
			return;
		}

		ok = UpdateResource(resupdate, RT_RCDATA,
		    MAKEINTRESOURCE(1000+i), MAKELANGID(LANG_NEUTRAL,
		    SUBLANG_NEUTRAL), srcfileview, (DWORD)srcfilesz);
		if (!ok) {
			warn(_T("Failed to update resource in output file"));
			UnmapViewOfFile(srcfileview);
			CloseHandle(srcfilemap);
			CloseHandle(srcfile);
			EndUpdateResource(resupdate, TRUE);
			DeleteFile(tmppath);
			return;
		}

		UnmapViewOfFile(srcfileview);
		CloseHandle(srcfilemap);
		CloseHandle(srcfile);
	}

	EndUpdateResource(resupdate, FALSE);

	if (!CopyFile(tmppath, path, FALSE))
		warn(_T("Failed to write output file"));

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
