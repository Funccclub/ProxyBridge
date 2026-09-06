// ui/auth.h - sign-up / sign-in gate matching native ProxyBridge dialog style.
#ifndef PB_UI_AUTH_H
#define PB_UI_AUTH_H

#include "../auth/auth.h"
#include "../res/resource.h"
#include "../loc/loc.h"

static BOOL g_authSignup = FALSE;
static HFONT g_authTitleFont = NULL;

static void AuthShow(HWND h, int id, BOOL show)
{
    HWND w = GetDlgItem(h, id);
    if (w) ShowWindow(w, show ? SW_SHOW : SW_HIDE);
}

static void AuthPlace(HWND h, int id, int x, int y, int w, int ht)
{
    HWND ctrl = GetDlgItem(h, id);
    if (ctrl) SetWindowPos(ctrl, NULL, x, y, w, ht, SWP_NOZORDER);
}

static void AuthApplyMode(HWND dlg)
{
    g_authSignup = !Auth_HasPassword();

    SetWindowTextW(dlg, g_authSignup ? T(S_AUTH_SIGNUP_CAP) : T(S_AUTH_SIGNIN_CAP));
    SetDlgItemTextW(dlg, IDC_AUTH_TITLE, APP_TITLE);
    SetDlgItemTextW(dlg, IDC_AUTH_SUBTITLE,
                    g_authSignup ? T(S_AUTH_SIGNUP_SUB) : T(S_AUTH_SIGNIN_SUB));
    SetDlgItemTextW(dlg, IDC_AUTH_G_ACCOUNT, T(S_AUTH_GROUP));
    SetDlgItemTextW(dlg, IDC_AUTH_EMAIL_LABEL, T(S_AUTH_EMAIL));
    SetDlgItemTextW(dlg, IDC_AUTH_PASS_LABEL, T(S_L_PASS));
    SetDlgItemTextW(dlg, IDC_AUTH_CONFIRM_LABEL, T(S_AUTH_CONFIRM));
    SetDlgItemTextW(dlg, IDOK, g_authSignup ? T(S_AUTH_SIGNUP_BTN) : T(S_AUTH_SIGNIN_BTN));
    SetDlgItemTextW(dlg, IDCANCEL, T(S_BTN_CANCEL));

    AuthShow(dlg, IDC_AUTH_EMAIL_LABEL, g_authSignup);
    AuthShow(dlg, IDC_AUTH_EMAIL,       g_authSignup);
    AuthShow(dlg, IDC_AUTH_CONFIRM_LABEL, g_authSignup);
    AuthShow(dlg, IDC_AUTH_CONFIRM,     g_authSignup);

    if (g_authSignup)
    {
        AuthPlace(dlg, IDC_AUTH_G_ACCOUNT, 8, 40, 304, 88);
        AuthPlace(dlg, IDC_AUTH_PASS_LABEL, 18, 74, 44, 10);
        AuthPlace(dlg, IDC_AUTH_PASS, 66, 72, 238, 12);
        AuthPlace(dlg, IDOK, 204, 136, 50, 14);
        AuthPlace(dlg, IDCANCEL, 262, 136, 50, 14);
    }
    else
    {
        AuthPlace(dlg, IDC_AUTH_G_ACCOUNT, 8, 40, 304, 52);
        AuthPlace(dlg, IDC_AUTH_PASS_LABEL, 18, 56, 44, 10);
        AuthPlace(dlg, IDC_AUTH_PASS, 66, 54, 238, 12);
        AuthPlace(dlg, IDOK, 204, 100, 50, 14);
        AuthPlace(dlg, IDCANCEL, 262, 100, 50, 14);
        SetWindowPos(dlg, NULL, 0, 0, 320, 132, SWP_NOMOVE | SWP_NOZORDER);
    }

    if (g_authSignup)
        SetWindowPos(dlg, NULL, 0, 0, 320, 168, SWP_NOMOVE | SWP_NOZORDER);

    SetDlgItemTextW(dlg, IDC_AUTH_EMAIL, L"");
    SetDlgItemTextW(dlg, IDC_AUTH_PASS, L"");
    SetDlgItemTextW(dlg, IDC_AUTH_CONFIRM, L"");
    SetFocus(GetDlgItem(dlg, g_authSignup ? IDC_AUTH_EMAIL : IDC_AUTH_PASS));
}

static INT_PTR CALLBACK AuthDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        if (!g_authTitleFont)
            g_authTitleFont = CreateFontW(-22, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendDlgItemMessageW(dlg, IDC_AUTH_TITLE, WM_SETFONT, (WPARAM)g_authTitleFont, TRUE);
        InitDarkMode(dlg);
        AuthApplyMode(dlg);
        return TRUE;

    case WM_CTLCOLORDLG:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        return DlgCtlColor(wp);

    case WM_CTLCOLORSTATIC:
    {
        HDC dc = (HDC)wp;
        int id = GetDlgCtrlID((HWND)lp);
        SetBkColor(dc, C_BG);
        if (id == IDC_AUTH_TITLE)
            SetTextColor(dc, C_ACCENT);
        else if (id == IDC_AUTH_SUBTITLE)
            SetTextColor(dc, C_DIM);
        else
            SetTextColor(dc, C_TEXT);
        return (INT_PTR)g_brBg;
    }

    case WM_COMMAND:
        if (LOWORD(wp) == IDOK)
        {
            wchar_t pass[128] = {0}, confirm[128] = {0};
            GetDlgItemTextW(dlg, IDC_AUTH_PASS, pass, 128);
            if (!pass[0])
            {
                MessageBoxW(dlg, T(S_AUTH_ERR_PASS), APP_TITLE, MB_OK | MB_ICONWARNING);
                return TRUE;
            }
            if (g_authSignup)
            {
                GetDlgItemTextW(dlg, IDC_AUTH_CONFIRM, confirm, 128);
                if (lstrcmpW(pass, confirm) != 0)
                {
                    MessageBoxW(dlg, T(S_AUTH_ERR_MATCH), APP_TITLE, MB_OK | MB_ICONWARNING);
                    return TRUE;
                }
                if (lstrlenW(pass) < 4)
                {
                    MessageBoxW(dlg, T(S_AUTH_ERR_SHORT), APP_TITLE, MB_OK | MB_ICONWARNING);
                    return TRUE;
                }
                if (!Auth_SetPassword(pass))
                {
                    MessageBoxW(dlg, T(S_AUTH_ERR_SAVE), APP_TITLE, MB_OK | MB_ICONERROR);
                    return TRUE;
                }
                EndDialog(dlg, IDOK);
                return TRUE;
            }
            if (!Auth_Verify(pass))
            {
                MessageBoxW(dlg, T(S_AUTH_ERR_BAD), APP_TITLE, MB_OK | MB_ICONERROR);
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
