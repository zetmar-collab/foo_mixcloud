# foo_mixcloud

A native foobar2000 input plugin that streams shows from Mixcloud using [yt-dlp](https://github.com/yt-dlp/yt-dlp).

## Features

- Paste a Mixcloud show URL via **File > Mixcloud > Add show or URL...** and play it directly, no login required for public content.
- Load all uploads from a user's profile via **File > Mixcloud > Add user's uploads...**.
- Optional cookie-based login (cookies.txt file or browser cookie store) for private/favorited shows, configurable in Preferences > Tools > Mixcloud.
- Configurable yt-dlp path and audio format selector.

## Installation

1. Make sure [yt-dlp](https://github.com/yt-dlp/yt-dlp) is installed and available on `PATH` (or note its full path to enter later in Preferences).
2. Download the latest `foo_mixcloud-vX.Y.fb2k-component` file from the [Releases](https://github.com/zetmar-collab/foo_mixcloud/releases) page.
3. In foobar2000, open **File > Preferences > Components > Install...** and select the downloaded `.fb2k-component` file.
4. Confirm the restart prompt.
5. (Optional) In **Preferences > Tools > Mixcloud**, set the yt-dlp path if it isn't on `PATH`, and configure cookie login if you need private/favorited shows.
6. Use **File > Mixcloud > Add show or URL...** to paste a Mixcloud link and play it, or **File > Mixcloud > Add user's uploads...** to load a user's whole profile.

## Building from source

Requires:
- Visual Studio 2022 Build Tools (v143 toolset) with the VC/ATL component
- [foobar2000 SDK](https://www.foobar2000.org/SDK) extracted to `sdk/`
- [WTL](https://sourceforge.net/projects/wtl/) extracted to `wtl/`

Build `foo_mixcloud/foo_mixcloud.sln` (not the bare `.vcxproj` — the solution maps `pfc` to the `Debug FB2K`/`Release FB2K` configurations required to avoid duplicate-symbol link errors).

Deploy the resulting `foo_mixcloud.dll` to foobar2000's `user-components-x64\foo_mixcloud\` folder (use the in-app **Install...** dialog once so foobar2000 registers the component).

## Requirements at runtime

- [yt-dlp](https://github.com/yt-dlp/yt-dlp) installed and on `PATH` (or point to it in Preferences).
