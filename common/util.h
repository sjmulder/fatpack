#define LEN(a) (sizeof(a)/sizeof(*(a)))

void warn(const TCHAR *info);
void warnx(const TCHAR *message);
void err(const TCHAR *info);
void errx(const TCHAR *message);

BOOL makeusedefault(HWND window);
