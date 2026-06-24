#include "stdafx.h"
#include "ytdlp_process.h"
#include "config.h"
#include <thread>

namespace ytdlp {

	pfc::string8 exe_path() {
		pfc::string8 p = mixcloud_cfg::ytdlp_path.get();
		if (p.is_empty()) p = "yt-dlp";
		return p;
	}

	void append_cookie_args(pfc::list_t<pfc::string8>& args) {
		switch (mixcloud_cfg::cookie_mode.get()) {
		case mixcloud_cfg::cookie_mode_file:
			if (!pfc::string8(mixcloud_cfg::cookie_file.get()).is_empty()) {
				args.add_item("--cookies");
				args.add_item(mixcloud_cfg::cookie_file.get());
			}
			break;
		case mixcloud_cfg::cookie_mode_browser:
			if (!pfc::string8(mixcloud_cfg::cookie_browser.get()).is_empty()) {
				args.add_item("--cookies-from-browser");
				args.add_item(mixcloud_cfg::cookie_browser.get());
			}
			break;
		default:
			break;
		}
	}

	// Quotes a single argv element per the algorithm CommandLineToArgvW (and
	// therefore every CRT's argv parser) expects, so paths/URLs with spaces,
	// quotes or backslashes survive the round trip intact.
	static void append_quoted(pfc::string8& out, const char* arg) {
		out.add_char('"');
		int backslashes = 0;
		for (const char* p = arg; *p; ++p) {
			if (*p == '\\') {
				++backslashes;
			} else if (*p == '"') {
				for (int i = 0; i < backslashes * 2 + 1; ++i) out.add_char('\\');
				out.add_char('"');
				backslashes = 0;
			} else {
				for (int i = 0; i < backslashes; ++i) out.add_char('\\');
				backslashes = 0;
				out.add_char(*p);
			}
		}
		for (int i = 0; i < backslashes * 2; ++i) out.add_char('\\');
		out.add_char('"');
	}

	static pfc::string8 build_command_line(const char* exePath, const pfc::list_t<pfc::string8>& args) {
		pfc::string8 cmd;
		append_quoted(cmd, exePath);
		for (t_size i = 0; i < args.get_count(); ++i) {
			cmd.add_char(' ');
			append_quoted(cmd, args[i]);
		}
		return cmd;
	}

	pfc::string8 run(const char* exePath, const pfc::list_t<pfc::string8>& args, abort_callback& p_abort) {
		pfc::string8 cmdLine = build_command_line(exePath, args);

		pfc::stringcvt::string_os_from_utf8 cmdLineOS(cmdLine.get_ptr());

		SECURITY_ATTRIBUTES sa = {};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;

		HANDLE readPipe = nullptr, writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) {
			throw exception_ytdlp("Could not create a pipe to communicate with yt-dlp.");
		}
		// The read end must not be inherited by the child.
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		si.hStdOutput = writePipe;
		si.hStdError = writePipe;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

		PROCESS_INFORMATION pi = {};

		pfc::array_t<wchar_t> mutableCmd;
		t_size len = wcslen(cmdLineOS) + 1;
		mutableCmd.set_size(len);
		memcpy(mutableCmd.get_ptr(), cmdLineOS.get_ptr(), len * sizeof(wchar_t));

		BOOL ok = CreateProcessW(
			nullptr, mutableCmd.get_ptr(),
			nullptr, nullptr,
			TRUE /*inherit handles*/,
			CREATE_NO_WINDOW,
			nullptr, nullptr,
			&si, &pi
		);

		CloseHandle(writePipe);

		if (!ok) {
			CloseHandle(readPipe);
			pfc::string8 msg = "Could not start yt-dlp (";
			msg << exePath << "). Make sure it is installed and the path in preferences is correct.";
			throw exception_ytdlp(msg.get_ptr());
		}

		CloseHandle(pi.hThread);

		// Anonymous pipes lose buffered-but-unread bytes if the reader relies on
		// PeekNamedPipe racing against process exit (the pipe can report "broken"
		// before the last chunk is drained). A dedicated thread doing plain
		// blocking ReadFile calls until EOF avoids that race entirely; the main
		// thread only watches for abort/process-exit and never touches the pipe.
		pfc::string8 output;
		std::thread reader([readPipe, &output] {
			char buf[4096];
			for (;;) {
				DWORD got = 0;
				if (!ReadFile(readPipe, buf, (DWORD)sizeof(buf), &got, nullptr) || got == 0) break;
				output.add_string(buf, got);
			}
		});

		bool killed = false;
		for (;;) {
			DWORD waitResult = WaitForSingleObject(pi.hProcess, 50);
			if (waitResult == WAIT_OBJECT_0) break;

			try {
				p_abort.check();
			} catch (...) {
				TerminateProcess(pi.hProcess, 1);
				killed = true;
				break;
			}
		}

		WaitForSingleObject(pi.hProcess, 2000);
		reader.join();
		CloseHandle(readPipe);

		if (killed) {
			CloseHandle(pi.hProcess);
			throw exception_aborted();
		}

		DWORD exitCode = 0;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);

		if (!killed && exitCode != 0) {
			pfc::string8 msg = "yt-dlp exited with an error:\n";
			msg << output;
			throw exception_ytdlp(msg.get_ptr());
		}

		return output;
	}
}
