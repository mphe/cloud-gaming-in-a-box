#!/bin/bash
#Run game or given command in environment

cd "/home/$USER/.local/share/Steam/steamapps/common/HatinTime/Binaries/Win64" || exit 1
DEF_CMD=("/home/$USER/.local/share/Steam/steamapps/common/HatinTime/Binaries/Win64/HatinTimeGame.exe")
	# WINEDEBUG="-all" \
PATH="/home/$USER/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/:/usr/bin:/bin" \
	TERM="xterm" \
	WINEDLLPATH="/home/$USER/.local/share/Steam/steamapps/common/Proton - Experimental/files/lib64//wine:/home/$USER/.local/share/Steam/steamapps/common/Proton - Experimental/files/lib//wine" \
	LD_LIBRARY_PATH="/home/$USER/.local/share/Steam/ubuntu12_64/video/:/home/$USER/.local/share/Steam/ubuntu12_32/video/:/home/$USER/.local/share/Steam/steamapps/common/Proton - Experimental/files/lib64/:/home/$USER/.local/share/Steam/steamapps/common/Proton - Experimental/files/lib/:/usr/lib/pressure-vessel/overrides/lib/x86_64-linux-gnu/aliases:/usr/lib/pressure-vessel/overrides/lib/i386-linux-gnu/aliases" \
	WINEPREFIX="/home/$USER/.local/share/Steam/steamapps/compatdata/253230/pfx/" \
	WINEESYNC="1" \
	WINEFSYNC="1" \
	SteamGameId="253230" \
	SteamAppId="253230" \
	WINEDLLOVERRIDES="steam.exe=b;dotnetfx35.exe=b;dotnetfx35setup.exe=b;beclient.dll=b,n;beclient_x64.dll=b,n;d3d11=n;d3d10core=n;d3d9=n;dxgi=n;d3d12=n;d3d12core=n" \
	STEAM_COMPAT_CLIENT_INSTALL_PATH="/home/$USER/.local/share/Steam" \
	WINE_LARGE_ADDRESS_AWARE="1" \
	GST_PLUGIN_SYSTEM_PATH_1_0="/home/$USER/.local/share/Steam/steamapps/common/Proton - Experimental/files/lib64/gstreamer-1.0:/home/$USER/.local/share/Steam/steamapps/common/Proton - Experimental/files/lib/gstreamer-1.0" \
	WINE_GST_REGISTRY_DIR="/home/$USER/.local/share/Steam/steamapps/compatdata/253230/gstreamer-1.0/" \
	MEDIACONV_AUDIO_DUMP_FILE="/home/$USER/.local/share/Steam/steamapps/shadercache/253230/fozmediav1/audiov2.foz" \
	MEDIACONV_AUDIO_TRANSCODED_FILE="/home/$USER/.local/share/Steam/steamapps/shadercache/253230/transcoded_audio.foz" \
	MEDIACONV_VIDEO_DUMP_FILE="/home/$USER/.local/share/Steam/steamapps/shadercache/253230/fozmediav1/video.foz" \
	MEDIACONV_VIDEO_TRANSCODED_FILE="/home/$USER/.local/share/Steam/steamapps/shadercache/253230/transcoded_video.foz" \
	"/home/$USER/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wine64" c:\\windows\\system32\\steam.exe "${@:-${DEF_CMD[@]}}"
