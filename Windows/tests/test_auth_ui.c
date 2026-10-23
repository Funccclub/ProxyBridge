// Real password page + credential storage regression tests. No engine is started.
#define wWinMain pb_real_wWinMain
#include "../gui/main.c"
#undef wWinMain
#include "pbtest.h"

static void WriteCredential(const wchar_t* path, const char* text)
{
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD n = 0;
    CHECK(h != INVALID_HANDLE_VALUE);
    if (h != INVALID_HANDLE_VALUE) { CHECK(WriteFile(h, text, (DWORD)strlen(text), &n, NULL)); CloseHandle(h); }
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE prev, PWSTR cmd, int show)
{
    (void)prev; (void)show;
    g_hInst = hInst; g_lang = 1;
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES}; InitCommonControlsEx(&icc);
    CreateDarkBrushes();
    wchar_t temp[MAX_PATH], sub[80], data[MAX_PATH], path[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    _snwprintf_s(sub, ARRAYSIZE(sub), _TRUNCATE, L"pbtest_auth_%lu", GetCurrentProcessId());
    wcscat_s(temp, MAX_PATH, sub); CreateDirectoryW(temp, NULL);
    SetEnvironmentVariableW(L"APPDATA", temp); PB_InitStorage(); Auth_Init();
    PB_GetDataDir(data, MAX_PATH);
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%s\\auth.dat", data);

    if (cmd && wcsstr(cmd, L"--preview")) {
        if (wcsstr(cmd, L"--preview-unlock")) Auth_SetPassword(L"preview-only");
        Auth_ShowDialog(hInst, NULL);
    } else {
        CHECK(!Auth_HasPassword());
        CHECK(!Auth_SetPassword(L"abc"));
        AuthView setup = {0};
        HWND dlg = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_AUTH), NULL, AuthDlgProc, (LPARAM)&setup);
        CHECK(dlg != NULL && setup.setup);
        CHECK(SendDlgItemMessageW(dlg, IDC_AUTH_CODE, EM_GETLIMITTEXT, 0, 0) == AUTH_PASSWORD_MAX);
        CHECK(!AuthSubmit(dlg, TRUE));
        SetDlgItemTextW(dlg, IDC_AUTH_CODE, L"测试 pass 123");
        SetDlgItemTextW(dlg, IDC_AUTH_CONFIRM, L"different");
        CHECK(!AuthSubmit(dlg, TRUE) && !Auth_HasPassword());
        SetDlgItemTextW(dlg, IDC_AUTH_CONFIRM, L"测试 pass 123");
        CHECK(AuthSubmit(dlg, TRUE));
        DestroyWindow(dlg);
        CHECK(Auth_Verify(L"测试 pass 123"));
        CHECK(!Auth_Verify(L"wrong-password"));
        Auth_Init(); CHECK(Auth_Verify(L"测试 pass 123"));

        AuthView unlock = {0};
        dlg = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_AUTH), NULL, AuthDlgProc, (LPARAM)&unlock);
        CHECK(dlg != NULL && !unlock.setup);
        CHECK(!(GetWindowLongW(GetDlgItem(dlg, IDC_AUTH_CONFIRM), GWL_STYLE) & WS_VISIBLE));
        SetDlgItemTextW(dlg, IDC_AUTH_CODE, L"wrong-password"); CHECK(!AuthSubmit(dlg, FALSE));
        SetDlgItemTextW(dlg, IDC_AUTH_CODE, L"测试 pass 123"); CHECK(AuthSubmit(dlg, FALSE));
        DestroyWindow(dlg);

        // Malformed, oversized and badly padded input must fail closed, with no buffer overrun/reset.
        const char* bad[] = {
            "{\"salt\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\",\"hash\":\"AAAA\"}",
            "{\"salt\":\"AAAAAAAAAAAAAAAAAAAAAA==\",\"hash\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\"}",
            "{\"salt\":\"=AAA\",\"hash\":\"AAAA\"}",
            "{\"salt\":\"AA=A\",\"hash\":\"AAAA\"}",
            "{\"salt\":\"AAAA====\",\"hash\":\"AAAA\"}",
            "not json"
        };
        for (int i = 0; i < ARRAYSIZE(bad); i++) {
            WriteCredential(path, bad[i]); Auth_Init();
            CHECK(Auth_HasPassword()); CHECK(!Auth_Verify(L"测试 pass 123"));
        }
    }
    DeleteFileW(path); RemoveDirectoryW(data); RemoveDirectoryW(temp);
    return pbtest_done("test_auth_ui");
}
