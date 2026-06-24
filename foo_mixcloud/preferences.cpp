#include "stdafx.h"
#include "resource.h"
#include "config.h"
#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>
#include <shared/shared.h>

namespace {

	class CMixcloudPreferences : public CDialogImpl<CMixcloudPreferences>, public preferences_page_instance {
	public:
		CMixcloudPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

		enum { IDD = IDD_MIXCLOUD_PREFERENCES };

		t_uint32 get_state();
		void apply();
		void reset();

		BEGIN_MSG_MAP_EX(CMixcloudPreferences)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDC_YTDLP_PATH, EN_CHANGE, OnEditChange)
			COMMAND_HANDLER_EX(IDC_COOKIE_FILE_PATH, EN_CHANGE, OnEditChange)
			COMMAND_HANDLER_EX(IDC_COOKIE_BROWSER_NAME, EN_CHANGE, OnEditChange)
			COMMAND_HANDLER_EX(IDC_FORMAT_SELECTOR, EN_CHANGE, OnEditChange)
			COMMAND_HANDLER_EX(IDC_COOKIE_NONE, BN_CLICKED, OnEditChange)
			COMMAND_HANDLER_EX(IDC_COOKIE_FILE, BN_CLICKED, OnEditChange)
			COMMAND_HANDLER_EX(IDC_COOKIE_BROWSER, BN_CLICKED, OnEditChange)
			COMMAND_ID_HANDLER_EX(IDC_YTDLP_BROWSE, OnBrowseYtdlp)
			COMMAND_ID_HANDLER_EX(IDC_COOKIE_FILE_BROWSE, OnBrowseCookieFile)
		END_MSG_MAP()

	private:
		BOOL OnInitDialog(CWindow, LPARAM) {
			m_dark.AddDialogWithControls(*this);

			uSetDlgItemText(m_hWnd, IDC_YTDLP_PATH, mixcloud_cfg::ytdlp_path.get());
			uSetDlgItemText(m_hWnd, IDC_COOKIE_FILE_PATH, mixcloud_cfg::cookie_file.get());
			uSetDlgItemText(m_hWnd, IDC_COOKIE_BROWSER_NAME, mixcloud_cfg::cookie_browser.get());
			uSetDlgItemText(m_hWnd, IDC_FORMAT_SELECTOR, mixcloud_cfg::format_selector.get());

			CheckRadioFor(mixcloud_cfg::cookie_mode.get());
			return TRUE;
		}

		void CheckRadioFor(t_uint32 mode) {
			CheckDlgButton(IDC_COOKIE_NONE, mode == mixcloud_cfg::cookie_mode_none);
			CheckDlgButton(IDC_COOKIE_FILE, mode == mixcloud_cfg::cookie_mode_file);
			CheckDlgButton(IDC_COOKIE_BROWSER, mode == mixcloud_cfg::cookie_mode_browser);
		}

		t_uint32 SelectedMode() {
			if (IsDlgButtonChecked(IDC_COOKIE_FILE)) return mixcloud_cfg::cookie_mode_file;
			if (IsDlgButtonChecked(IDC_COOKIE_BROWSER)) return mixcloud_cfg::cookie_mode_browser;
			return mixcloud_cfg::cookie_mode_none;
		}

		void OnEditChange(UINT, int, CWindow) { OnChanged(); }

		void OnBrowseYtdlp(UINT, int, CWindow) {
			pfc::string8 path;
			if (BrowseForFile(L"yt-dlp executable\0yt-dlp.exe\0All files\0*.*\0", path)) {
				uSetDlgItemText(m_hWnd, IDC_YTDLP_PATH, path.get_ptr());
				OnChanged();
			}
		}
		void OnBrowseCookieFile(UINT, int, CWindow) {
			pfc::string8 path;
			if (BrowseForFile(L"Cookies file (Netscape format)\0*.txt\0All files\0*.*\0", path)) {
				uSetDlgItemText(m_hWnd, IDC_COOKIE_FILE_PATH, path.get_ptr());
				OnChanged();
			}
		}

		bool BrowseForFile(const wchar_t* filter, pfc::string8& out) {
			wchar_t buffer[MAX_PATH] = {};
			OPENFILENAMEW ofn = {};
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = m_hWnd;
			ofn.lpstrFilter = filter;
			ofn.lpstrFile = buffer;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			if (!GetOpenFileNameW(&ofn)) return false;
			out = pfc::stringcvt::string_utf8_from_wide(buffer);
			return true;
		}

		bool HasChanged() {
			pfc::string8 buf;
			uGetDlgItemText(m_hWnd, IDC_YTDLP_PATH, buf);
			if (strcmp(buf.get_ptr(), mixcloud_cfg::ytdlp_path.get()) != 0) return true;
			uGetDlgItemText(m_hWnd, IDC_COOKIE_FILE_PATH, buf);
			if (strcmp(buf.get_ptr(), mixcloud_cfg::cookie_file.get()) != 0) return true;
			uGetDlgItemText(m_hWnd, IDC_COOKIE_BROWSER_NAME, buf);
			if (strcmp(buf.get_ptr(), mixcloud_cfg::cookie_browser.get()) != 0) return true;
			uGetDlgItemText(m_hWnd, IDC_FORMAT_SELECTOR, buf);
			if (strcmp(buf.get_ptr(), mixcloud_cfg::format_selector.get()) != 0) return true;
			if (SelectedMode() != mixcloud_cfg::cookie_mode.get()) return true;
			return false;
		}
		void OnChanged() { m_callback->on_state_changed(); }

		const preferences_page_callback::ptr m_callback;
		fb2k::CDarkModeHooks m_dark;
	};

	t_uint32 CMixcloudPreferences::get_state() {
		t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
		if (HasChanged()) state |= preferences_state::changed;
		return state;
	}

	void CMixcloudPreferences::reset() {
		uSetDlgItemText(m_hWnd, IDC_YTDLP_PATH, "yt-dlp");
		uSetDlgItemText(m_hWnd, IDC_COOKIE_FILE_PATH, "");
		uSetDlgItemText(m_hWnd, IDC_COOKIE_BROWSER_NAME, "chrome");
		uSetDlgItemText(m_hWnd, IDC_FORMAT_SELECTOR, "http/bestaudio/best");
		CheckRadioFor(mixcloud_cfg::cookie_mode_none);
		OnChanged();
	}

	void CMixcloudPreferences::apply() {
		pfc::string8 buf;
		uGetDlgItemText(m_hWnd, IDC_YTDLP_PATH, buf);
		mixcloud_cfg::ytdlp_path = buf;
		uGetDlgItemText(m_hWnd, IDC_COOKIE_FILE_PATH, buf);
		mixcloud_cfg::cookie_file = buf;
		uGetDlgItemText(m_hWnd, IDC_COOKIE_BROWSER_NAME, buf);
		mixcloud_cfg::cookie_browser = buf;
		uGetDlgItemText(m_hWnd, IDC_FORMAT_SELECTOR, buf);
		mixcloud_cfg::format_selector = buf;
		mixcloud_cfg::cookie_mode = SelectedMode();
		OnChanged();
	}

	class preferences_page_mixcloud : public preferences_page_impl<CMixcloudPreferences> {
	public:
		const char* get_name() { return "Mixcloud"; }
		GUID get_guid() {
			static constexpr GUID guid = { 0x6a1f0c92, 0x8b3e, 0x4c5d, { 0x9a, 0x77, 0x2e, 0x44, 0x9c, 0x8f, 0x10, 0x77 } };
			return guid;
		}
		GUID get_parent_guid() { return guid_tools; }
	};

	static preferences_page_factory_t<preferences_page_mixcloud> g_preferences_page_mixcloud_factory;
}
