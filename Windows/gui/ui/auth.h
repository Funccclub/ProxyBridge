// Local password page. Uses dialog units and native keyboard navigation.
#ifndef PB_UI_AUTH_H
#define PB_UI_AUTH_H

#include "../auth/auth.h"
#include "../res/resource.h"
#include "../loc/loc.h"

typedef struct {
    BOOL setup;
    HFONT heading, brand;
} AuthView;

static void AuthError(HWND dlg, const wchar_t* text, int field)
{
    SetDlgItemTextW(dlg, IDC_AUTH_ERROR, text);
    SetFocus(GetDlgItem(dlg, field));
    SendDlgItemMessageW(dlg, field, EM_SETSEL, 0, -1);
}

static BOOL AuthSubmit(HWND dlg, BOOL setup)
{
    wchar_t code[AUTH_PASSWORD_MAX + 1] = {0}, confirm[AUTH_PASSWORD_MAX + 1] = {0};
    BOOL ok = FALSE;
    GetDlgItemTextW(dlg, IDC_AUTH_CODE, code, ARRAYSIZE(code));
    GetDlgItemTextW(dlg, IDC_AUTH_CONFIRM, confirm, ARRAYSIZE(confirm));
    if (!code[0]) AuthError(dlg, T(S_AUTH_ERR_EMPTY), IDC_AUTH_CODE);
    else if (lstrlenW(code) < 4) AuthError(dlg, T(S_AUTH_ERR_SHORT), IDC_AUTH_CODE);
    else if (setup && wcscmp(code, confirm)) AuthError(dlg, T(S_AUTH_MISMATCH), IDC_AUTH_CONFIRM);
    else if (setup) {
        ok = Auth_SetPassword(code);
        if (!ok) AuthError(dlg, T(S_AUTH_ERR_SAVE), IDC_AUTH_CODE);
    } else {
        ok = Auth_Verify(code);
        if (!ok) AuthError(dlg, T(S_AUTH_ERR_BAD), IDC_AUTH_CODE);
    }
    SecureZeroMemory(code, sizeof(code));
    SecureZeroMemory(confirm, sizeof(confirm));
    return ok;
}

static LRESULT CALLBACK AuthButtonProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR ref)
{
    (void)ref;
    if (m == WM_MOUSEMOVE) {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, h, 0 };
        TrackMouseEvent(&tme);
        InvalidateRect(h, NULL, FALSE);
    }
    if (m == WM_MOUSELEAVE || m == WM_SETFOCUS || m == WM_KILLFOCUS)
        InvalidateRect(h, NULL, FALSE);
    if (m == WM_NCDESTROY) RemoveWindowSubclass(h, AuthButtonProc, id);
    return DefSubclassProc(h, m, w, l);
}

static void AuthDrawButton(const DRAWITEMSTRUCT* di)
{
    RECT r = di->rcItem;
    BOOL primary = di->CtlID == IDOK;
    POINT cursor; GetCursorPos(&cursor); ScreenToClient(di->hwndItem, &cursor);
    BOOL hot = PtInRect(&r, cursor);
    COLORREF color = primary ? (hot ? RGB(0x89,0xBC,0xFF) : C_ACCENT) : (hot ? C_TABHOT : C_PANEL);
    if (di->itemState & ODS_SELECTED) color = primary ? RGB(0x47,0x8C,0xDC) : C_TAB;
    FillRect(di->hDC, &r, g_brBg);
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, primary ? color : C_LINE);
    HGDIOBJ oldBrush = SelectObject(di->hDC, brush), oldPen = SelectObject(di->hDC, pen);
    RoundRect(di->hDC, r.left, r.top, r.right, r.bottom, 12, 12);
    SelectObject(di->hDC, oldBrush); SelectObject(di->hDC, oldPen);
    DeleteObject(brush); DeleteObject(pen);
    HFONT font = (HFONT)SendMessageW(di->hwndItem, WM_GETFONT, 0, 0);
    HGDIOBJ oldFont = SelectObject(di->hDC, font);
    wchar_t text[96]; GetWindowTextW(di->hwndItem, text, ARRAYSIZE(text));
    SetBkMode(di->hDC, TRANSPARENT); SetTextColor(di->hDC, primary ? C_BG : C_TEXT);
    DrawTextW(di->hDC, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(di->hDC, oldFont);
    if ((di->itemState & ODS_FOCUS) && !(di->itemState & ODS_NOFOCUSRECT)) {
        InflateRect(&r, -5, -5); DrawFocusRect(di->hDC, &r);
    }
}

static INT_PTR CALLBACK AuthDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    AuthView* view = (AuthView*)GetWindowLongPtrW(dlg, GWLP_USERDATA);
    switch (msg) {
    case WM_INITDIALOG: {
        view = (AuthView*)lp;
        SetWindowLongPtrW(dlg, GWLP_USERDATA, lp);
        view->setup = !Auth_HasPassword();
        view->heading = CreateFontW(-MulDiv(26, g_dpi, 96), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        view->brand = CreateFontW(-MulDiv(14, g_dpi, 96), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        InitDarkMode(dlg);
        SendDlgItemMessageW(dlg, IDC_AUTH_HEADING, WM_SETFONT, (WPARAM)view->heading, TRUE);
        SendDlgItemMessageW(dlg, IDC_AUTH_TITLE, WM_SETFONT, (WPARAM)view->brand, TRUE);
        SetWindowTextW(dlg, T(S_AUTH_CAP));
        SetDlgItemTextW(dlg, IDC_AUTH_TITLE, APP_TITLE);
        SetDlgItemTextW(dlg, IDC_AUTH_HEADING, T(view->setup ? S_AUTH_SETUP_HEADING : S_AUTH_HEADING));
        SetDlgItemTextW(dlg, IDC_AUTH_PROMPT, T(view->setup ? S_AUTH_SETUP_PROMPT : S_AUTH_PROMPT));
        SetDlgItemTextW(dlg, IDC_AUTH_CODE_LABEL, T(S_AUTH_CODE));
        SetDlgItemTextW(dlg, IDC_AUTH_CONFIRM_LABEL, T(S_AUTH_CONFIRM));
        SetDlgItemTextW(dlg, IDOK, T(view->setup ? S_AUTH_SETUP_BTN : S_AUTH_BTN));
        SetDlgItemTextW(dlg, IDCANCEL, T(S_BTN_CANCEL));
        SetDlgItemTextW(dlg, IDC_AUTH_FOOTER, T(S_AUTH_FOOTER));
        SendDlgItemMessageW(dlg, IDC_AUTH_CODE, EM_SETLIMITTEXT, AUTH_PASSWORD_MAX, 0);
        SendDlgItemMessageW(dlg, IDC_AUTH_CONFIRM, EM_SETLIMITTEXT, AUTH_PASSWORD_MAX, 0);
        SetWindowSubclass(GetDlgItem(dlg, IDOK), AuthButtonProc, 1, 0);
        SetWindowSubclass(GetDlgItem(dlg, IDCANCEL), AuthButtonProc, 1, 0);
        if (!view->setup) {
            ShowWindow(GetDlgItem(dlg, IDC_AUTH_CONFIRM_LABEL), SW_HIDE);
            ShowWindow(GetDlgItem(dlg, IDC_AUTH_CONFIRM), SW_HIDE);
            RECT shift = {0, 0, 0, 53}; MapDialogRect(dlg, &shift);
            const int ids[] = { IDC_AUTH_ERROR, IDOK, IDCANCEL, IDC_AUTH_FOOTER };
            for (int i = 0; i < ARRAYSIZE(ids); i++) {
                HWND h = GetDlgItem(dlg, ids[i]); RECT r; GetWindowRect(h, &r);
                MapWindowPoints(NULL, dlg, (POINT*)&r, 2);
                SetWindowPos(h, NULL, r.left, r.top - shift.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            RECT r; GetWindowRect(dlg, &r);
            SetWindowPos(dlg, NULL, r.left, r.top + shift.bottom / 2,
                r.right-r.left, r.bottom-r.top-shift.bottom, SWP_NOZORDER);
        }
        SendMessageW(dlg, DM_SETDEFID, IDOK, 0);
        SetFocus(GetDlgItem(dlg, IDC_AUTH_CODE));
        return FALSE; // keep explicit focus on the password field
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(dlg, &ps);
        RECT r; GetClientRect(dlg, &r); FillRect(dc, &r, g_brBg);
        int fields[] = { IDC_AUTH_CODE, IDC_AUTH_CONFIRM };
        for (int i = 0; i < 2; i++) {
            HWND h = GetDlgItem(dlg, fields[i]);
            if (!IsWindowVisible(h)) continue;
            RECT frame = {26, 126 + i*53, 334, 156 + i*53}; MapDialogRect(dlg, &frame);
            HPEN pen = CreatePen(PS_SOLID, GetFocus() == h ? 2 : 1, GetFocus() == h ? C_ACCENT : C_LINE);
            HGDIOBJ op = SelectObject(dc, pen), ob = SelectObject(dc, g_brPanel);
            RoundRect(dc, frame.left, frame.top, frame.right, frame.bottom, 10, 10);
            SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);
        }
        EndPaint(dlg, &ps); return TRUE;
    }
    case WM_DRAWITEM:
        if (wp == IDOK || wp == IDCANCEL) { AuthDrawButton((const DRAWITEMSTRUCT*)lp); return TRUE; }
        break;
    case WM_CTLCOLORDLG: return (INT_PTR)g_brBg;
    case WM_CTLCOLOREDIT:
        SetTextColor((HDC)wp, C_TEXT); SetBkColor((HDC)wp, C_PANEL); return (INT_PTR)g_brPanel;
    case WM_CTLCOLORSTATIC: {
        int id = GetDlgCtrlID((HWND)lp);
        SetBkColor((HDC)wp, C_BG);
        SetTextColor((HDC)wp, id == IDC_AUTH_TITLE ? C_ACCENT :
            id == IDC_AUTH_ERROR ? RGB(0xFB,0x91,0x91) :
            (id == IDC_AUTH_PROMPT || id == IDC_AUTH_FOOTER) ? C_DIM : C_TEXT);
        return (INT_PTR)g_brBg;
    }
    case WM_COMMAND:
        if ((LOWORD(wp) == IDC_AUTH_CODE || LOWORD(wp) == IDC_AUTH_CONFIRM)) {
            if (HIWORD(wp) == EN_CHANGE) SetDlgItemTextW(dlg, IDC_AUTH_ERROR, L"");
            if (HIWORD(wp) == EN_SETFOCUS || HIWORD(wp) == EN_KILLFOCUS) InvalidateRect(dlg, NULL, FALSE);
            return TRUE;
        }
        if (LOWORD(wp) == IDOK && view) {
            if (AuthSubmit(dlg, view->setup)) EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    case WM_CLOSE: EndDialog(dlg, IDCANCEL); return TRUE;
    case WM_DESTROY:
        if (view) { DeleteObject(view->heading); DeleteObject(view->brand); }
        return TRUE;
    }
    return FALSE;
}

static BOOL Auth_ShowDialog(HINSTANCE hInst, HWND parent)
{
    AuthView view = {0};
    return DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_AUTH), parent, AuthDlgProc, (LPARAM)&view) == IDOK;
}
#endif
