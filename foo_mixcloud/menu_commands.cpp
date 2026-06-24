#include "stdafx.h"
#include "ytdlp_process.h"
#include "input_dialog.h"
#include <functional>
#include <memory>

namespace {

	constexpr char kProtocol[] = "mixcloud://";

	pfc::string8 to_mixcloud_path(const pfc::string8& httpsUrl) {
		pfc::string8 out = kProtocol;
		const char* p = httpsUrl.get_ptr();
		if (pfc::strcmp_partial(p, "https://") == 0) p += 8;
		else if (pfc::strcmp_partial(p, "http://") == 0) p += 7;
		out += p;
		return out;
	}

	// Splits text into trimmed, non-empty lines.
	void for_each_line(const pfc::string8& text, std::function<void(const char*, t_size)> f) {
		const char* base = text.get_ptr();
		t_size pos = 0;
		while (pos <= text.length()) {
			t_size eol = text.find_first('\n', pos);
			t_size end = (eol == pfc::infinite_size) ? text.length() : eol;
			t_size start = pos;
			auto isSpace = [&](t_size i) { return base[i] == ' ' || base[i] == '\r' || base[i] == '\t'; };
			while (start < end && isSpace(start)) ++start;
			t_size trimmedEnd = end;
			while (trimmedEnd > start && isSpace(trimmedEnd - 1)) --trimmedEnd;
			if (trimmedEnd > start) f(base + start, trimmedEnd - start);
			if (eol == pfc::infinite_size) break;
			pos = eol + 1;
		}
	}

	void AddSingleUrl(const char* rawUrl) {
		pfc::string8 url = rawUrl;
		if (url.is_empty()) return;
		if (pfc::strcmp_partial(url.get_ptr(), "http://") != 0 && pfc::strcmp_partial(url.get_ptr(), "https://") != 0) {
			pfc::string8 fixed = "https://";
			fixed += url;
			url = fixed;
		}
		pfc::string8 path = to_mixcloud_path(url);

		pfc::list_t<const char*> paths;
		paths.add_item(path.get_ptr());
		static_api_ptr_t<playlist_manager> pm;
		pm->activeplaylist_add_locations(paths, true, core_api::get_main_window());
	}

	// Accepts either a bare username or a full Mixcloud profile/uploads URL
	// (with or without scheme, trailing slash, or "/uploads") and returns
	// just the username part.
	pfc::string8 extract_username(const char* raw) {
		pfc::string8 s = raw;
		const char* p = s.get_ptr();
		if (pfc::strcmp_partial(p, "https://") == 0) p += 8;
		else if (pfc::strcmp_partial(p, "http://") == 0) p += 7;
		if (pfc::strcmp_partial(p, "www.mixcloud.com/") == 0) p += 17;
		else if (pfc::strcmp_partial(p, "mixcloud.com/") == 0) p += 13;

		pfc::string8 out = p;
		while (out.length() > 0 && out.get_ptr()[out.length() - 1] == '/') out.truncate(out.length() - 1);

		const char* suffix = "/uploads";
		t_size suffixLen = strlen(suffix);
		if (out.length() >= suffixLen && strcmp(out.get_ptr() + out.length() - suffixLen, suffix) == 0) {
			out.truncate(out.length() - suffixLen);
		}

		t_size slash = out.find_first('/');
		if (slash != pfc::infinite_size) out.truncate(slash);

		return out;
	}

	void AddUserUploads(const char* username) {
		pfc::string8 user = extract_username(username);
		if (user.is_empty()) return;

		auto state = std::make_shared<pfc::string8>();

		auto cb = threaded_process_callback_lambda::create(
			[](fb2k::hwnd_t) {},
			[user, state](threaded_process_status& status, abort_callback& abort) {
				status.set_item("Listing shows from Mixcloud...");
				pfc::string8 profileUrl = "https://www.mixcloud.com/";
				profileUrl += user;
				profileUrl += "/uploads/";

				pfc::list_t<pfc::string8> args;
				args.add_item("--no-warnings");
				args.add_item("--flat-playlist");
				args.add_item("--print");
				args.add_item("%(webpage_url)s");
				ytdlp::append_cookie_args(args);
				args.add_item(profileUrl);

				try {
					*state = ytdlp::run(ytdlp::exe_path(), args, abort);
				} catch (ytdlp::exception_ytdlp& e) {
					*state = "";
					popup_message::g_show(e.what(), "Mixcloud: could not list uploads");
				}
			},
			[state](fb2k::hwnd_t wnd, bool aborted) {
				if (aborted || state->is_empty()) return;

				pfc::list_t<pfc::string8> mixPaths;
				for_each_line(*state, [&](const char* s, t_size len) {
					pfc::string8 url;
					url.set_string(s, len);
					mixPaths.add_item(to_mixcloud_path(url));
				});
				if (mixPaths.get_count() == 0) {
					popup_message::g_show("No shows were found for this Mixcloud account / username.", "Mixcloud");
					return;
				}

				pfc::list_t<const char*> paths;
				for (t_size i = 0; i < mixPaths.get_count(); ++i) paths.add_item(mixPaths[i].get_ptr());

				static_api_ptr_t<playlist_manager> pm;
				pm->activeplaylist_add_locations(paths, false, wnd);
			}
		);

		threaded_process::g_run_modeless(cb, threaded_process::flag_show_abort | threaded_process::flag_show_item, core_api::get_main_window(), "Loading Mixcloud uploads");
	}

	static const GUID guid_mainmenu_group_id = { 0x9d4e2a11, 0x7c3f, 0x4b8a, { 0xae, 0x5d, 0x10, 0x2b, 0x33, 0x9f, 0x71, 0x06 } };
	static const GUID guid_cmd_add_url = { 0x3f6b8e02, 0x1a9c, 0x4d77, { 0x90, 0x1e, 0x44, 0x7a, 0xcb, 0x12, 0x8e, 0x55 } };
	static const GUID guid_cmd_add_user = { 0x7e1c4f93, 0x2d8b, 0x4e16, { 0xb3, 0x4a, 0x9f, 0x6e, 0x21, 0xd0, 0x8c, 0x37 } };

	static mainmenu_group_popup_factory g_mainmenu_group(guid_mainmenu_group_id, mainmenu_groups::file, mainmenu_commands::sort_priority_dontcare, "Mixcloud");

	class mainmenu_commands_mixcloud : public mainmenu_commands {
	public:
		enum {
			cmd_add_url = 0,
			cmd_add_user,
			cmd_total
		};
		t_uint32 get_command_count() override { return cmd_total; }
		GUID get_command(t_uint32 p_index) override {
			switch (p_index) {
			case cmd_add_url: return guid_cmd_add_url;
			case cmd_add_user: return guid_cmd_add_user;
			default: uBugCheck();
			}
		}
		void get_name(t_uint32 p_index, pfc::string_base& p_out) override {
			switch (p_index) {
			case cmd_add_url: p_out = "Add show or URL..."; break;
			case cmd_add_user: p_out = "Add user's uploads..."; break;
			default: uBugCheck();
			}
		}
		bool get_description(t_uint32 p_index, pfc::string_base& p_out) override {
			switch (p_index) {
			case cmd_add_url: p_out = "Adds a single Mixcloud show URL to the current playlist."; return true;
			case cmd_add_user: p_out = "Lists every show uploaded by a Mixcloud account and adds it to the current playlist."; return true;
			default: return false;
			}
		}
		GUID get_parent() override { return guid_mainmenu_group_id; }
		void execute(t_uint32 p_index, service_ptr_t<service_base>) override {
			switch (p_index) {
			case cmd_add_url: {
				pfc::string8 value;
				if (ShowMixcloudInputDialog(core_api::get_main_window(), "Mixcloud", "Mixcloud show URL (e.g. https://www.mixcloud.com/user/show-name/):", "", value)) {
					AddSingleUrl(value.get_ptr());
				}
				break;
			}
			case cmd_add_user: {
				pfc::string8 value;
				if (ShowMixcloudInputDialog(core_api::get_main_window(), "Mixcloud", "Mixcloud username (the part after mixcloud.com/ in their profile URL):", "", value)) {
					AddUserUploads(value.get_ptr());
				}
				break;
			}
			default:
				uBugCheck();
			}
		}
	};

	static mainmenu_commands_factory_t<mainmenu_commands_mixcloud> g_mainmenu_commands_mixcloud_factory;
}
