// ui/auth.h - sign-up / sign-in styled password gate (unity-build include from main.c).
#ifndef PB_UI_AUTH_H
#define PB_UI_AUTH_H

#include "../auth/auth.h"
#include "../res/resource.h"
#include "../loc/loc.h"

static BOOL g_authSignup = FALSE;

static void AuthStyleField(HWND dlg, int id)
{
    HWND h = GetDlgItem(dlg, id);
    if (h) SetWindowTheme(h, L"DarkMode_CFD", NULL);
}

static void AuthApplyMode(HWND dlg)
{
    g_authSignup = !Auth_HasPassword();
    SetDlgItemTextW(dlg, IDC_AUTH_TITLE,
                    g_authSignup ? L"Create your account" : L"Welcome back");
    SetDlgItemTextW(dlg, IDC_AUTH_SUBTITLE,
                    g_authSignup ? L"Sign up to get started with Doggie"
                                 : L"Sign in to continue");
    SetDlgItemTextW(dlg, IDC_AUTH_SUBMIT,
                    g_authSignup ? L"Sign Up" : L"Sign In");

    ShowWindow(GetDlgItem(dlg, IDC_AUTH_CONFIRM_LABEL), g_authSignup ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dlg, IDC_AUTH_CONFIRM),     g_authSignup ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dlg, IDC_AUTH_EMAIL_LABEL), g_authSignup ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dlg, IDC_AUTH_EMAIL),       g_authSignup ? SW_SHOW : SW_HIDE);

    SetDlgItemTextW(dlg, IDC_AUTH_PASS, L"");
    SetDlgItemTextW(dlg, IDC_AUTH_CONFIRM, L"");
    if (g_authSignup)
        SetDlgItemTextW(dlg, IDC_AUTH_EMAIL, L"");
    SetFocus(GetDlgItem(dlg, g_authSignup ? IDC_AUTH_EMAIL : IDC_AUTH_PASS));
}

static INT_PTR CALLBACK AuthDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        InitDarkMode(dlg);
        EnumChildWindows(dlg, DarkThemeChild, 0);
        AuthStyleField(dlg, IDC_AUTH_EMAIL);
        AuthStyleField(dlg, IDC_AUTH_PASS);
        AuthStyleField(dlg, IDC_AUTH_CONFIRM);
        AuthApplyMode(dlg);
        return FALSE;
    }
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wp, TRANSPARENT);
        SetTextColor((HDC)wp, C_TEXT);
        return (INT_PTR)g_brBg;
    case WM_COMMAND:
        if (LOWORD(wp) == IDC_AUTH_SUBMIT || LOWORD(wp) == IDOK)
        {
            wchar_t pass[128] = {0}, confirm[128] = {0};
            GetDlgItemTextW(dlg, IDC_AUTH_PASS, pass, 128);
            if (!pass[0])
            {
                MessageBoxW(dlg, L"Please enter your password.", L"Doggie", MB_OK | MB_ICONWARNING);
                return TRUE;
            }
            if (g_authSignup)
            {
                GetDlgItemTextW(dlg, IDC_AUTH_CONFIRM, confirm, 128);
                if (lstrcmpW(pass, confirm) != 0)
                {
                    MessageBoxW(dlg, L"Passwords do not match.", L"Doggie", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }
                if (lstrlenW(pass) < 4)
                {
                    MessageBoxW(dlg, L"Password must be at least 4 characters.", L"Doggie", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }
                if (!Auth_SetPassword(pass))
                {
                    MessageBoxW(dlg, L"Could not save your account.", L"Doggie", MB_OK | MB_ICONERROR);
                    return TRUE;
                }
                EndDialog(dlg, IDOK);
                return TRUE;
            }
            if (!Auth_Verify(pass))
            {
                MessageBoxW(dlg, L"Incorrect password. Please try again.", L"Doggie", MB_OK | MB_ICONERROR);
                SetDlgItemTextW(dlg, IDC_AUTH_PASS, L"");
                SetFocus(GetDlgItem(dlg, IDC_AUTH_PASS));
                return TRUE;
            }
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL)
        {
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL Auth_ShowDialog(HINSTANCE hInst, HWND parent)
{
    INT_PTR r = DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_AUTH), parent, AuthDlgProc);
    return r == IDOK;
}

#endif // PB_UI_AUTH_H
