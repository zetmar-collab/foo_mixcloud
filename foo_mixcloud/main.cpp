#include "stdafx.h"

DECLARE_COMPONENT_VERSION(
	"Mixcloud Streaming",
	"1.0",
	"Streams shows from your Mixcloud account into foobar2000 using yt-dlp.\n"
	"https://www.mixcloud.com/\n"
);

VALIDATE_COMPONENT_FILENAME("foo_mixcloud.dll");

FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
