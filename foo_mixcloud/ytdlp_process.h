#pragma once

// Thin helper for invoking yt-dlp.exe as a child process and capturing its
// combined stdout+stderr output, with foobar2000 abort_callback cooperation
// (so the user can cancel a stuck network request from the UI).

namespace ytdlp {

	// Thrown when yt-dlp cannot be located/started, or exits with a nonzero
	// status code. p_message contains yt-dlp's own diagnostic output when
	// available, which is far more useful to the user than a generic error.
	class exception_ytdlp : public pfc::exception {
	public:
		exception_ytdlp(const char* what) : pfc::exception(what) {}
	};

	// Runs "<exe> <args...>" and returns whatever it wrote to stdout/stderr.
	// Honors p_abort: if the operation is aborted the child process is killed.
	pfc::string8 run(const char* exePath, const pfc::list_t<pfc::string8>& args, abort_callback& p_abort);

	// Builds the --cookies / --cookies-from-browser arguments according to
	// current preferences, so every call site stays in sync automatically.
	void append_cookie_args(pfc::list_t<pfc::string8>& args);

	// Returns the configured yt-dlp executable path (defaults to "yt-dlp",
	// relying on it being available on PATH).
	pfc::string8 exe_path();
}
