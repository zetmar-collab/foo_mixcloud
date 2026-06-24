#pragma once

// Shared configuration storage for the component. Declared in one place so
// the preferences page, the input decoder and the menu commands all agree on
// where settings live and what their defaults are.

namespace mixcloud_cfg {

	enum cookie_mode {
		cookie_mode_none = 0,
		cookie_mode_file = 1,
		cookie_mode_browser = 2,
	};

	// Path (or bare name relying on PATH) to the yt-dlp executable.
	extern cfg_string ytdlp_path;

	// How to authenticate yt-dlp as the user's logged-in Mixcloud account so
	// that private/favorited/exclusive shows can be resolved, not just public
	// uploads. "None" only works for fully public content.
	extern cfg_uint cookie_mode;

	// Used when cookie_mode == cookie_mode_file: path to a Netscape-format
	// cookies.txt exported from the user's browser while logged into Mixcloud.
	extern cfg_string cookie_file;

	// Used when cookie_mode == cookie_mode_browser: a browser name yt-dlp
	// understands for --cookies-from-browser, e.g. "chrome", "firefox", "edge".
	extern cfg_string cookie_browser;

	// Preferred yt-dlp format selector for resolving the audio stream.
	extern cfg_string format_selector;
}
