#include "ui/settings_dialog.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

// Control IDs
static constexpr WORD IDC_HOST_LABEL   = 100;
static constexpr WORD IDC_HOST_EDIT    = 101;
static constexpr WORD IDC_PORT_LABEL   = 102;
static constexpr WORD IDC_PORT_EDIT    = 103;
static constexpr WORD IDC_AUTOSTART    = 104;
static constexpr WORD IDC_SEPARATOR    = 105;
static constexpr WORD IDC_SAVE_BTN     = IDOK;
static constexpr WORD IDC_CANCEL_BTN   = IDCANCEL;

// ============================================================================
// In-memory dialog template builder
// ============================================================================

static LPWORD align_dword(LPWORD ptr) {
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    addr = (addr + 3) & ~static_cast<uintptr_t>(3);
    return reinterpret_cast<LPWORD>(addr);
}

static LPWORD write_wide_string(LPWORD ptr, const char* str) {
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, reinterpret_cast<LPWSTR>(ptr), 128);
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

/// @brief Build the settings dialog template with Segoe UI 9pt font
static LPDLGTEMPLATE build_settings_template() {
    static BYTE buffer[2048];
    std::memset(buffer, 0, sizeof(buffer));

    auto* dlg = reinterpret_cast<LPDLGTEMPLATE>(buffer);
    dlg->style = DS_MODALFRAME | DS_CENTER | DS_SETFONT
               | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dlg->dwExtendedStyle = 0;
    dlg->cdit = 8;  // 2 labels + 2 edits + checkbox + separator + 2 buttons
    dlg->x = 0;
    dlg->y = 0;
    dlg->cx = 160;
    dlg->cy = 82;

    auto* ptr = reinterpret_cast<LPWORD>(dlg + 1);

    // Menu (none)
    *ptr++ = 0;
    // Class (default)
    *ptr++ = 0;
    // Title
    ptr = write_wide_string(ptr, "MCP Server Settings");

    // Font (DS_SETFONT): point size + face name
    *ptr++ = 9;  // 9pt
    ptr = write_wide_string(ptr, "Segoe UI");

    // --- Controls ---
    // Row 1: Host
    ptr = add_dialog_item(ptr,
        SS_RIGHT, 7, 9, 22, 8,
        IDC_HOST_LABEL, 0x0082, "Host:");

    ptr = add_dialog_item(ptr,
        ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 33, 7, 120, 12,
        IDC_HOST_EDIT, 0x0081, "");

    // Row 2: Port
    ptr = add_dialog_item(ptr,
        SS_RIGHT, 7, 25, 22, 8,
        IDC_PORT_LABEL, 0x0082, "Port:");

    ptr = add_dialog_item(ptr,
        ES_AUTOHSCROLL | ES_NUMBER | WS_BORDER | WS_TABSTOP, 33, 23, 40, 12,
        IDC_PORT_EDIT, 0x0081, "");

    // Row 3: Auto-start checkbox
    ptr = add_dialog_item(ptr,
        BS_AUTOCHECKBOX | WS_TABSTOP, 7, 41, 146, 10,
        IDC_AUTOSTART, 0x0080, "Auto-start server on plugin load");

    // Etched separator line
    ptr = add_dialog_item(ptr,
        SS_ETCHEDHORZ, 7, 56, 146, 1,
        IDC_SEPARATOR, 0x0082, "");

    // Buttons row (right-aligned)
    ptr = add_dialog_item(ptr,
        BS_DEFPUSHBUTTON | WS_TABSTOP, 56, 63, 45, 14,
        IDC_SAVE_BTN, 0x0080, "Save");

    ptr = add_dialog_item(ptr,
        BS_PUSHBUTTON | WS_TABSTOP, 106, 63, 45, 14,
        IDC_CANCEL_BTN, 0x0080, "Cancel");

    return dlg;
}

// ============================================================================
// Dialog procedure
// ============================================================================

static INT_PTR CALLBACK settings_dlg_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_INITDIALOG: {
        auto* settings = reinterpret_cast<s_plugin_settings*>(lparam);
        SetWindowLongPtrA(hdlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

        SetDlgItemTextA(hdlg, IDC_HOST_EDIT, settings->host);

        char port_buf[16];
        std::snprintf(port_buf, sizeof(port_buf), "%u", settings->port);
        SetDlgItemTextA(hdlg, IDC_PORT_EDIT, port_buf);

        CheckDlgButton(hdlg, IDC_AUTOSTART,
            settings->auto_start ? BST_CHECKED : BST_UNCHECKED);

        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDC_SAVE_BTN: {
            auto* settings = reinterpret_cast<s_plugin_settings*>(
                GetWindowLongPtrA(hdlg, GWLP_USERDATA));

            GetDlgItemTextA(hdlg, IDC_HOST_EDIT, settings->host, sizeof(settings->host));

            char port_buf[16];
            GetDlgItemTextA(hdlg, IDC_PORT_EDIT, port_buf, sizeof(port_buf));
            int port_val = std::atoi(port_buf);
            if (port_val < 1 || port_val > 65535) {
                MessageBoxA(hdlg, "Port must be between 1 and 65535.",
                    "Invalid Port", MB_OK | MB_ICONWARNING);
                return TRUE;
            }
            settings->port = static_cast<uint16_t>(port_val);

            settings->auto_start =
                (IsDlgButtonChecked(hdlg, IDC_AUTOSTART) == BST_CHECKED);

            EndDialog(hdlg, IDOK);
            return TRUE;
        }

        case IDC_CANCEL_BTN:
            EndDialog(hdlg, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hdlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// Public API
// ============================================================================

int show_settings_dialog(HWND parent, s_plugin_settings& settings) {
    s_plugin_settings working_copy = settings;

    auto* dlg_template = build_settings_template();
    auto result = DialogBoxIndirectParamA(
        GetModuleHandleA(nullptr),
        dlg_template,
        parent,
        settings_dlg_proc,
        reinterpret_cast<LPARAM>(&working_copy)
    );

    if (result == IDOK) {
        settings = working_copy;
    }

    return static_cast<int>(result);
}
