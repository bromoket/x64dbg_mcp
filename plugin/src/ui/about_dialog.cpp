#include "ui/about_dialog.h"
#include "plugin_main.h"

#include <cstdio>
#include <cstring>
#include <shellapi.h>

// Control IDs
static constexpr WORD IDC_TITLE_LABEL   = 200;
static constexpr WORD IDC_VER_LABEL     = 201;
static constexpr WORD IDC_STATUS_LABEL  = 202;
static constexpr WORD IDC_SEPARATOR     = 203;
static constexpr WORD IDC_URL_LABEL     = 204;
static constexpr WORD IDC_DISCORD_LABEL = 205;
static constexpr WORD IDC_OK_BTN        = IDOK;

// ============================================================================
// In-memory dialog template builder
// ============================================================================

static LPWORD align_dword(LPWORD ptr) {
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    addr = (addr + 3) & ~static_cast<uintptr_t>(3);
    return reinterpret_cast<LPWORD>(addr);
}

static LPWORD write_wide_string(LPWORD ptr, const char* str) {
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, reinterpret_cast<LPWSTR>(ptr), 256);
    return ptr + len;
}

static LPWORD add_dialog_item(
    LPWORD ptr,
    DWORD style,
    short x, short y, short cx, short cy,
    WORD id,
    WORD class_atom,
    const char* text
) {
    ptr = align_dword(ptr);

    auto* item = reinterpret_cast<DLGITEMTEMPLATE*>(ptr);
    item->style = style | WS_CHILD | WS_VISIBLE;
    item->dwExtendedStyle = 0;
    item->x = x;
    item->y = y;
    item->cx = cx;
    item->cy = cy;
    item->id = id;

    ptr = reinterpret_cast<LPWORD>(item + 1);

    *ptr++ = 0xFFFF;
    *ptr++ = class_atom;

    ptr = write_wide_string(ptr, text);

    // Creation data (none)
    *ptr++ = 0;

    return ptr;
}

// ============================================================================
// About dialog state (passed via lParam)
// ============================================================================

struct s_about_state {
    bool is_running;
    char status_text[128];
};

/// @brief Build the about dialog template with Segoe UI 9pt font
static LPDLGTEMPLATE build_about_template() {
    static BYTE buffer[2048];
    std::memset(buffer, 0, sizeof(buffer));

    auto* dlg = reinterpret_cast<LPDLGTEMPLATE>(buffer);
    dlg->style = DS_MODALFRAME | DS_CENTER | DS_SETFONT
               | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dlg->dwExtendedStyle = 0;
    dlg->cdit = 7;  // title + version + status + separator + url + discord + OK
    dlg->x = 0;
    dlg->y = 0;
    dlg->cx = 160;
    dlg->cy = 100;

    auto* ptr = reinterpret_cast<LPWORD>(dlg + 1);

    // Menu (none)
    *ptr++ = 0;
    // Class (default)
    *ptr++ = 0;
    // Title
    ptr = write_wide_string(ptr, "About");

    // Font (DS_SETFONT): point size + face name
    *ptr++ = 9;
    ptr = write_wide_string(ptr, "Segoe UI");

    // --- Controls ---

    // Title (centered, bolded in WM_INITDIALOG)
    ptr = add_dialog_item(ptr,
        SS_CENTER, 7, 8, 146, 10,
        IDC_TITLE_LABEL, 0x0082, PLUGIN_NAME);

    // Version (centered)
    ptr = add_dialog_item(ptr,
        SS_CENTER, 7, 21, 146, 8,
        IDC_VER_LABEL, 0x0082, "");

    // Status (centered)
    ptr = add_dialog_item(ptr,
        SS_CENTER, 7, 33, 146, 8,
        IDC_STATUS_LABEL, 0x0082, "");

    // Etched separator
    ptr = add_dialog_item(ptr,
        SS_ETCHEDHORZ, 7, 47, 146, 1,
        IDC_SEPARATOR, 0x0082, "");

    // GitHub URL (centered, clickable — SS_NOTIFY enables STN_CLICKED)
    ptr = add_dialog_item(ptr,
        SS_CENTER | SS_NOTIFY, 7, 53, 146, 8,
        IDC_URL_LABEL, 0x0082, "github.com/bromoket/x64dbg_mcp");

    // Discord (centered, grey)
    ptr = add_dialog_item(ptr,
        SS_CENTER, 7, 65, 146, 8,
        IDC_DISCORD_LABEL, 0x0082, "");

    // OK button (centered)
    ptr = add_dialog_item(ptr,
        BS_DEFPUSHBUTTON | WS_TABSTOP, 55, 80, 50, 14,
        IDC_OK_BTN, 0x0080, "OK");

    return dlg;
}

// ============================================================================
// Dialog procedure
// ============================================================================

static HFONT s_bold_font = nullptr;
static HFONT s_link_font = nullptr;
static HCURSOR s_hand_cursor = nullptr;

static INT_PTR CALLBACK about_dlg_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_INITDIALOG: {
        auto* state = reinterpret_cast<s_about_state*>(lparam);

        // Store state pointer for WM_CTLCOLORSTATIC
        SetWindowLongPtrA(hdlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

        // Version
        char ver_buf[64];
        std::snprintf(ver_buf, sizeof(ver_buf), "Version %s", PLUGIN_VERSION_STR);
        SetDlgItemTextA(hdlg, IDC_VER_LABEL, ver_buf);

        // Status
        SetDlgItemTextA(hdlg, IDC_STATUS_LABEL, state->status_text);

        // Discord
        char discord_buf[64];
        std::snprintf(discord_buf, sizeof(discord_buf), "Discord:  %s", PLUGIN_AUTHOR);
        SetDlgItemTextA(hdlg, IDC_DISCORD_LABEL, discord_buf);

        // Derive bold font for title
        HFONT dlg_font = reinterpret_cast<HFONT>(SendMessageA(hdlg, WM_GETFONT, 0, 0));
        if (dlg_font) {
            LOGFONTA lf;
            GetObjectA(dlg_font, sizeof(lf), &lf);

            // Bold for title
            lf.lfWeight = FW_BOLD;
            s_bold_font = CreateFontIndirectA(&lf);
            if (s_bold_font) {
                SendDlgItemMessageA(hdlg, IDC_TITLE_LABEL, WM_SETFONT,
                    reinterpret_cast<WPARAM>(s_bold_font), TRUE);
            }

            // Underline for link
            lf.lfWeight = FW_NORMAL;
            lf.lfUnderline = TRUE;
            s_link_font = CreateFontIndirectA(&lf);
            if (s_link_font) {
                SendDlgItemMessageA(hdlg, IDC_URL_LABEL, WM_SETFONT,
                    reinterpret_cast<WPARAM>(s_link_font), TRUE);
            }
        }

        // Cache hand cursor for the link
        s_hand_cursor = LoadCursorA(nullptr, IDC_HAND);

        return TRUE;
    }

    case WM_CTLCOLORSTATIC: {
        auto ctrl_id = GetDlgCtrlID(reinterpret_cast<HWND>(lparam));
        auto hdc = reinterpret_cast<HDC>(wparam);

        if (ctrl_id == IDC_STATUS_LABEL) {
            // Green if running, red if stopped
            auto* state = reinterpret_cast<s_about_state*>(
                GetWindowLongPtrA(hdlg, GWLP_USERDATA));
            if (state && state->is_running) {
                SetTextColor(hdc, RGB(34, 139, 34));
            } else {
                SetTextColor(hdc, RGB(200, 50, 50));
            }
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
        }
        if (ctrl_id == IDC_URL_LABEL) {
            // Blue underlined link
            SetTextColor(hdc, RGB(0, 102, 204));
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
        }
        if (ctrl_id == IDC_DISCORD_LABEL) {
            // Grey text
            SetTextColor(hdc, RGB(100, 100, 100));
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
        }
        break;
    }

    case WM_SETCURSOR: {
        // Show hand cursor when hovering the link
        auto ctrl_id = GetDlgCtrlID(reinterpret_cast<HWND>(wparam));
        if (ctrl_id == IDC_URL_LABEL && s_hand_cursor) {
            SetCursor(s_hand_cursor);
            SetWindowLongPtrA(hdlg, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wparam) == IDC_URL_LABEL && HIWORD(wparam) == STN_CLICKED) {
            ShellExecuteA(hdlg, "open", PLUGIN_REPO_URL, nullptr, nullptr, SW_SHOWNORMAL);
            return TRUE;
        }
        if (LOWORD(wparam) == IDC_OK_BTN) {
            EndDialog(hdlg, IDOK);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hdlg, IDOK);
        return TRUE;

    case WM_DESTROY:
        if (s_bold_font) {
            DeleteObject(s_bold_font);
            s_bold_font = nullptr;
        }
        if (s_link_font) {
            DeleteObject(s_link_font);
            s_link_font = nullptr;
        }
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// Public API
// ============================================================================

void show_about_dialog(HWND parent, bool is_server_running, const char* host, uint16_t port) {
    s_about_state state{};
    state.is_running = is_server_running;

    if (is_server_running) {
        std::snprintf(state.status_text, sizeof(state.status_text),
            "Status:  Running on %s:%u", host, port);
    } else {
        std::snprintf(state.status_text, sizeof(state.status_text),
            "Status:  Stopped");
    }

    auto* dlg_template = build_about_template();
    DialogBoxIndirectParamA(
        GetModuleHandleA(nullptr),
        dlg_template,
        parent,
        about_dlg_proc,
        reinterpret_cast<LPARAM>(&state)
    );
}
