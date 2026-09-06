// ui/auth.h - password gate presented as an activation-code dialog.
#ifndef PB_UI_AUTH_H
#define PB_UI_AUTH_H

#include "../auth/auth.h"
#include "../res/resource.h"
#include "../loc/loc.h"

static HFONT g_authTitleFont = NULL;

static void AuthApplyUi(HWND dlg)
{
    SetWindowTextW(dlg, T(S_AUTH_CAP));
    SetDlgItemTextW(dlg, IDC_AUTH_TITLE, APP_TITLE);
    SetDlgItemTextW(dlg, IDC_AUTH_PROMPT, T(S_AUTH_PROMPT));
    SetDlgItemTextW(dlg, IDC_AUTH_CODE_LABEL, T(S_AUTH_CODE));
    SetDlgItemTextW(dlg, IDOK, T(S_AUTH_BTN));
    SetDlgItemTextW(dlg, IDCANCEL, T(S_BTN_CANCEL));
    SetDlgItemTextW(dlg, IDC_AUTH_CODE, L"");
    SetFocus(GetDlgItem(dlg, IDC_AUTH_CODE));
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
        AuthApplyUi(dlg);
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
        else if (id == IDC_AUTH_PROMPT)
            SetTextColor(dc, C_DIM);
        else
            SetTextColor(dc, C_TEXT);
        return (INT_PTR)g_brBg;
    }

    case WM_COMMAND:
        if (LOWORD(wp) == IDOK)
        {
            wchar_t code[128] = {0};
            GetDlgItemTextW(dlg, IDC_AUTH_CODE, code, 128);

            if (!code[0])
            {
                MessageBoxW(dlg, T(S_AUTH_ERR_EMPTY), APP_TITLE, MB_OK | MB_ICONWARNING);
                return TRUE;
            }
            if (lstrlenW(code) < 4)
            {
                MessageBoxW(dlg, T(S_AUTH_ERR_SHORT), APP_TITLE, MB_OK | MB_ICONWARNING);
                return TRUE;
            }

            if (!Auth_HasPassword())
            {
                if (!Auth_SetPassword(code))
                {
                    MessageBoxW(dlg, T(S_AUTH_ERR_SAVE), APP_TITLE, MB_OK | MB_ICONERROR);
                    return TRUE;
                }
                EndDialog(dlg, IDOK);
                return TRUE;
            }

            if (!Auth_Verify(code))
            {
                MessageBoxW(dlg, T(S_AUTH_ERR_BAD), APP_TITLE, MB_OK | MB_ICONERROR);
                SetDlgItemTextW(dlg, IDC_AUTH_CODE, L"");
                SetFocus(GetDlgItem(dlg, IDC_AUTH_CODE));
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
