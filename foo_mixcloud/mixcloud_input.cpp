#include "stdafx.h"
#include "ytdlp_process.h"
#include "config.h"

namespace {

	constexpr char kProtocol[] = "mixcloud://";
	constexpr size_t kProtocolLen = sizeof(kProtocol) - 1;

	// Delimiter for the single-line --print template below; chosen because it
	// can never legitimately appear in a Mixcloud title/uploader/URL.
	constexpr char kFieldSep = '\x01';

	// input_mixcloud is a thin decoder *proxy*: it resolves a mixcloud:// page
	// URL to a real (expiring) CDN audio URL via yt-dlp, then hands off all
	// actual decoding work to whatever input foobar2000 already uses for that
	// resolved URL (its bundled HTTP + AAC/MP3 decoders). This means we don't
	// have to implement any audio codec ourselves.
	class input_mixcloud : public input_stubs {
	public:
		void open(service_ptr_t<file> p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort) {
			if (p_reason == input_open_info_write) throw exception_tagging_unsupported();

			resolve(p_path, p_abort);
			input_entry::g_open_for_decoding(m_decoder, nullptr, m_resolvedUrl.get_ptr(), p_abort);
		}

		void get_info(file_info& p_info, abort_callback& p_abort) {
			m_decoder->get_info(0, p_info, p_abort);
			if (m_title.length()) p_info.meta_set("title", m_title);
			if (m_uploader.length()) p_info.meta_set("artist", m_uploader);
			if (m_durationSeconds > 0) p_info.set_length(m_durationSeconds);
			p_info.info_set("mixcloud_url", m_pageUrl);
		}

		t_filestats2 get_stats2(uint32_t f, abort_callback& a) {
			(void)f; (void)a;
			return filestats2_invalid;
		}

		void decode_initialize(unsigned p_flags, abort_callback& p_abort) {
			m_decoder->initialize(0, p_flags, p_abort);
		}
		bool decode_run(audio_chunk& p_chunk, abort_callback& p_abort) {
			return m_decoder->run(p_chunk, p_abort);
		}
		void decode_seek(double p_seconds, abort_callback& p_abort) {
			m_decoder->seek(p_seconds, p_abort);
		}
		bool decode_can_seek() {
			return m_decoder->can_seek();
		}
		bool decode_get_dynamic_info(file_info& p_out, double& p_timestamp_delta) {
			return m_decoder->get_dynamic_info(p_out, p_timestamp_delta);
		}
		bool decode_get_dynamic_info_track(file_info& p_out, double& p_timestamp_delta) {
			return m_decoder->get_dynamic_info_track(p_out, p_timestamp_delta);
		}
		void decode_on_idle(abort_callback& p_abort) {
			m_decoder->on_idle(p_abort);
		}

		void retag(const file_info&, abort_callback&) { throw exception_tagging_unsupported(); }
		void remove_tags(abort_callback&) { throw exception_tagging_unsupported(); }

		static bool g_is_our_content_type(const char*) { return false; }
		static bool g_is_our_path(const char* p_path, const char*) {
			return pfc::strcmp_partial(p_path, kProtocol) == 0;
		}
		static const char* g_get_name() { return "Mixcloud"; }
		static GUID g_get_guid() {
			static const GUID guid = { 0x2c8e9a3f, 0x4d1b, 0x4a7c, { 0x8e, 0x21, 0x6f, 0x9a, 0x3d, 0x55, 0x10, 0x42 } };
			return guid;
		}

	private:
		void resolve(const char* p_path, abort_callback& p_abort) {
			m_pageUrl = "https://";
			m_pageUrl += (p_path + kProtocolLen);

			pfc::list_t<pfc::string8> args;
			args.add_item("--no-warnings");
			args.add_item("--no-playlist");
			args.add_item("-f");
			args.add_item(mixcloud_cfg::format_selector.get());
			args.add_item("--print");
			pfc::string8 tmpl;
			tmpl << "%(title)s"; tmpl.add_byte(kFieldSep);
			tmpl << "%(uploader)s"; tmpl.add_byte(kFieldSep);
			tmpl << "%(duration)s"; tmpl.add_byte(kFieldSep);
			tmpl << "%(urls)s";
			args.add_item(tmpl);
			ytdlp::append_cookie_args(args);
			args.add_item(m_pageUrl);

			pfc::string8 output;
			try {
				output = ytdlp::run(ytdlp::exe_path(), args, p_abort);
			} catch (ytdlp::exception_ytdlp& e) {
				throw exception_io_data(e.what());
			}

			parse_print_line(output);

			if (m_resolvedUrl.is_empty()) {
				throw exception_io_data("yt-dlp did not return a playable stream URL for this Mixcloud item.");
			}
		}

		// Picks the last non-empty line containing our field separator (yt-dlp
		// may print informational lines before the one we asked for via
		// --print, even with --no-warnings, e.g. when cookies are stale).
		void parse_print_line(const pfc::string8& output) {
			pfc::string8 bestLine;
			t_size pos = 0;
			while (pos <= output.length()) {
				t_size eol = output.find_first('\n', pos);
				pfc::string8 line;
				if (eol == pfc::infinite_size) {
					line = output.get_ptr() + pos;
					pos = output.length() + 1;
				} else {
					line.set_string(output.get_ptr() + pos, eol - pos);
					pos = eol + 1;
				}
				if (line.find_first(kFieldSep) != pfc::infinite_size) bestLine = line;
			}

			pfc::string8 fields[4];
			int idx = 0;
			t_size start = 0;
			for (t_size i = 0; i <= bestLine.length() && idx < 4; ++i) {
				if (i == bestLine.length() || bestLine[i] == kFieldSep) {
					fields[idx].set_string(bestLine.get_ptr() + start, i - start);
					++idx;
					start = i + 1;
				}
			}

			m_title = fields[0];
			m_uploader = fields[1];
			m_durationSeconds = atof(fields[2].get_ptr());
			m_resolvedUrl = fields[3];
		}

		pfc::string8 m_pageUrl, m_resolvedUrl, m_title, m_uploader;
		double m_durationSeconds = 0;
		service_ptr_t<input_decoder> m_decoder;
	};

	static input_singletrack_factory_t<input_mixcloud> g_input_mixcloud_factory;
}
