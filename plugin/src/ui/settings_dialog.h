#pragma once

#include <windows.h>
#include "plugin_main.h"

/// @brief Show the settings dialog (modal). Returns IDOK if user saved, IDCANCEL otherwise.
/// @param parent Parent window handle
/// @param settings Settings struct to read from / write into on save
int show_settings_dialog(HWND parent, s_plugin_settings& settings);
