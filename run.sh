#!/usr/bin/env bash

# Public variables
# Can be overridden by defining respective environment variables when running this script.
WIDTH=${WIDTH:-1920}
HEIGHT=${HEIGHT:-1080}
FRONTEND_VSYNC=${FRONTEND_VSYNC:-false}
FPS=${FPS:-60}
CURRENT_KEYBOARD_LAYOUT="$(setxkbmap -query | grep layout | sed -r 's/.*\s(.+)$/\1/')"  # Not overridable
XVFB_KEYBOARD_LAYOUT="${XVFB_KEYBOARD_LAYOUT:-$CURRENT_KEYBOARD_LAYOUT}"
SYNCINPUT_PROTOCOL=${SYNCINPUT_PROTOCOL:-tcp}

# For Steam Proton games, set this option to false. They have their own vulkan translation layer and vglrun does not support vulkan.
USE_VIRTUALGL=${USE_VIRTUALGL:-true}

SERVER_DELAY_MS=${SERVER_DELAY_MS:-0}
SERVER_JITTER_MS=${SERVER_JITTER_MS:-0}
SERVER_LOSS_START=${SERVER_LOSS_START:-0.0}
SERVER_LOSS_STOP=${SERVER_LOSS_STOP:-1.0}
CLIENT_DELAY_MS=${CLIENT_DELAY_MS:-0}
CLIENT_JITTER_MS=${CLIENT_JITTER_MS:-0}
CLIENT_LOSS_START=${CLIENT_LOSS_START:-0.0}
CLIENT_LOSS_STOP=${CLIENT_LOSS_STOP:-1.0}
# VIDEO_CRF=${VIDEO_CRF:-23}
VIDEO_BITRATE=${VIDEO_BITRATE:-25M}

# Private variables
BUILD_DIR="$PWD/build"
LOG_DIR="$PWD/logs"
OUT_DISPLAY=:99
SINK_NAME=fakecloudgame
SINK_ID=
FFMPEG_VIDEO_PORT=5004
FFMPEG_AUDIO_PORT=4004
# Set socket buffer size to 20MB to allow high quality videos without artifacts.
VIDEO_OUT="rtp://127.0.0.1:$FFMPEG_VIDEO_PORT?buffer_size=20971520"
# VIDEO_OUT="rtp://127.0.0.1:$FFMPEG_VIDEO_PORT"
AUDIO_OUT="rtp://127.0.0.1:$FFMPEG_AUDIO_PORT"
SYNCINPUT_IP='127.0.0.1'
SYNCINPUT_PORT=9090
FRONTEND_SYNCINPUT_PORT=9091
FRONTEND_VIDEO_PORT=6004
FRONTEND_AUDIO_PORT=7004
COMMAND=""


cleanup() {
    echo "Cleanup"

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "app" ]; then
        [ -n "$SINK_ID" ] && pactl unload-module "$SINK_ID"
    fi

    [ -n "$(jobs -p)" ] && kill $(jobs -p)
}

run_proxies() {
    # UDP
    echo "UDP proxy"
    local server_args="-d $SERVER_DELAY_MS -j $SERVER_JITTER_MS --loss-start $SERVER_LOSS_START --loss-stop $SERVER_LOSS_STOP"
    ./udp-proxy/udp-proxy -l "$FFMPEG_AUDIO_PORT" -r "$FRONTEND_AUDIO_PORT" $server_args > "$LOG_DIR/udp_audio_rtp.log" 2>&1 &
    ./udp-proxy/udp-proxy -l "$FFMPEG_VIDEO_PORT" -r "$FRONTEND_VIDEO_PORT" $server_args > "$LOG_DIR/udp_video_rtp.log" 2>&1 &
    ./udp-proxy/udp-proxy -l "$((FFMPEG_AUDIO_PORT + 1))" -r "$((FRONTEND_AUDIO_PORT + 1))" $server_args > "$LOG_DIR/udp_audio_rtcp.log" 2>&1 &
    ./udp-proxy/udp-proxy -l "$((FFMPEG_VIDEO_PORT + 1))" -r "$((FRONTEND_VIDEO_PORT + 1))" $server_args > "$LOG_DIR/udp_video_rtcp.log" 2>&1 &

    # syncinput
    if [ "$SYNCINPUT_PROTOCOL" == "udp" ]; then
        ./udp-proxy/udp-proxy -l "$FRONTEND_SYNCINPUT_PORT" -r "$SYNCINPUT_PORT" -d "$CLIENT_DELAY_MS" -j "$CLIENT_JITTER_MS" --loss-start "$CLIENT_LOSS_START" --loss-stop "$CLIENT_LOSS_STOP" \
            > "$LOG_DIR/udp_syncinput.log" 2>&1 &
    elif [ "$SYNCINPUT_PROTOCOL" == "tcp" ]; then
        echo "TCP proxy"
        cd toxiproxy/dist || exit 1
        ./toxiproxy-server 2>&1 | tee "$LOG_DIR/toxiproxy_server.log" &
        sleep 1

        (
        ./toxiproxy-cli create --listen "localhost:$FRONTEND_SYNCINPUT_PORT" --upstream "localhost:$SYNCINPUT_PORT" input_proxy
        ./toxiproxy-cli toxic add --downstream --type latency --attribute latency="$CLIENT_DELAY_MS" --attribute jitter="$CLIENT_JITTER_MS" input_proxy
        ./toxiproxy-cli toxic add --upstream --type latency --attribute latency="$CLIENT_DELAY_MS" --attribute jitter="$CLIENT_JITTER_MS" input_proxy
        )  2>&1 | tee "$LOG_DIR/toxiproxy_cli.log"

        cd ../..
    fi
}

# args: app path + arguments
run_app() {
    echo "App"
    # Flags mentioned in the arch wiki to improve virtualgl performance.
    # https://wiki.archlinux.org/title/VirtualGL#rendering_glitches,_unusually_poor_performance,_or_application_errors
    # VGL_ALLOWINDIRECT=1
    # VGL_FORCEALPHA=1
    # VGL_GLFLUSHTRIGGER=0
    # VGL_READBACK=pbo
    # VGL_SPOILLAST=0
    # VGL_SYNC=1

    local app_path="$(which "$1")"
    shift 1
    local app_dir="$(dirname "$app_path")"
    local app_bin="./${app_path##*/}"

    echo "Using additional arguments: $*"

    # Some applications need to be run inside their directory
    cd "$app_dir" || exit 1

    if "$USE_VIRTUALGL"; then
        # VGL_ALLOWINDIRECT seems to work good for warsow and no problems otherwise.
        VGL_ALLOWINDIRECT=1 VGL_REFRESHRATE="$FPS" PULSE_SINK="$SINK_NAME" DISPLAY="$OUT_DISPLAY" vglrun "$app_bin" "$@" &
    else
        PULSE_SINK="$SINK_NAME" DISPLAY="$OUT_DISPLAY" "$app_bin" "$@" &
    fi

    cd - > /dev/null || exit 1

    sleep 1

    # Maximize the window, as some applications don't maximize automatically in Xvfb.
    echo "Maximizing window"
    DISPLAY="$OUT_DISPLAY" xdotool getwindowfocus windowsize 100% 100%  # maximize

    # Set Xvfb keyboard layout to german. This needs to be set after the application started.
    echo "Setting Xvfb keyboard layout: $XVFB_KEYBOARD_LAYOUT"
    DISPLAY="$OUT_DISPLAY" setxkbmap "$XVFB_KEYBOARD_LAYOUT"
}

main() {
    trap cleanup 0  # EXIT

    if [ "$1" == "-h" ] || [ "$1" == "--help" ] || [ $# -lt 1 ]; then
        echo "Usage: run_app.sh <app path> [command] [app args...]"
        echo -e "    <app path>: Path to the application executable."
        echo -e "    [command]: (Optional) Either 'app', 'stream', 'syncinput', 'proxy', 'frontend', or empty (''). Only run the specified sub system instead of the whole stack."
        echo -e "    [args...]: (Optional) Additional arguments passed to the application."
        return 0
    fi

    local APP_PATH="$1"
    COMMAND="$2"
    shift 1  # Shift app path
    shift 1  # Command is optional, so we need a separate shift, otherwise, it might not shift anything when exceeding the actual argument count.

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "app" ]; then
        echo "PA sink"
        # Create a new pulseaudio sink for the application to prevent playing audio directly to speakers.
        # Check if the sink already exists and remove it to ensure a fresh non-bugged sink
        SINK_ID="$(pactl list sinks | grep -A 6 "Name: $SINK_NAME" | grep 'Owner Module:' | cut -d ' ' -f 3)"
        [ -n "$SINK_ID" ] && pactl unload-module "$SINK_ID"
        SINK_ID="$(pactl load-module module-null-sink sink_name="$SINK_NAME")"

        echo "Xvfb"
        Xvfb "$OUT_DISPLAY" -screen 0 "${WIDTH}x${HEIGHT}x24" &

        run_app "$APP_PATH" "$@"
    fi

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "stream" ]; then
        # Presets and crf
        # https://superuser.com/questions/1556953/why-does-preset-veryfast-in-ffmpeg-generate-the-most-compressed-file-compared
        # https://trac.ffmpeg.org/wiki/Encode/H.264
        # -crf "$VIDEO_CRF"
	# -g 30 \
        #
        # NVidia hardware acceleration
        # ffmpeg -help encoder=hevc_nvenc | less
        # -c:v h264_nvenc -preset llhq -tune hq \
        echo "Video stream at $VIDEO_OUT"
        ffmpeg -threads 0 -r "$FPS" -f x11grab -video_size "${WIDTH}x${HEIGHT}" -framerate "$FPS" -i "$OUT_DISPLAY" -draw_mouse 1 \
            -pix_fmt yuv420p \
            -c:v libx264 -preset ultrafast -tune zerolatency -b:v "${VIDEO_BITRATE}" \
            -an \
            -f rtp "$VIDEO_OUT" \
            -sdp_file video.sdp \
            > "$LOG_DIR/video.log" 2>&1 &

        echo "Audio stream at $AUDIO_OUT"
        ffmpeg -f pulse -fragment_size 16 -i "$SINK_NAME.monitor" \
            -preset ultrafast -tune zerolatency \
            -c:a libopus -b:a 48000 \
            -payload_type 111 -f rtp -max_delay 0 "$AUDIO_OUT" -sdp_file audio.sdp \
            > "$LOG_DIR/audio.log" 2>&1 &

        sleep 1
        sed -i -r "s/$FFMPEG_VIDEO_PORT/$FRONTEND_VIDEO_PORT/" video.sdp
        sed -i -r "s/$FFMPEG_AUDIO_PORT/$FRONTEND_AUDIO_PORT/" audio.sdp
    fi

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "syncinput" ]; then
        echo "syncinput"
        local APP_TITLE=""  # Unused on linux
        DISPLAY="$OUT_DISPLAY" "$BUILD_DIR/syncinput" "$APP_TITLE" "$SYNCINPUT_IP" "$SYNCINPUT_PORT" "$SYNCINPUT_PROTOCOL" 2>&1 | tee "$LOG_DIR/syncinput.log" &
        sleep 1
    fi

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "proxy" ]; then
        run_proxies
    fi

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "frontend" ]; then
        echo "Frontend"
        local vsync=""
        $FRONTEND_VSYNC && vsync="vsync"
        "$BUILD_DIR/frontend" video.sdp audio.sdp "$SYNCINPUT_IP" "$FRONTEND_SYNCINPUT_PORT" "$SYNCINPUT_PROTOCOL" "$vsync" 2>&1 | tee "$LOG_DIR/frontend.log"
    else
        # Normally, wait until frontend quits, then kill all child processes.
        # But if the frontend was not started, wait for child processes to end.
        wait
    fi
}


mkdir -p "$LOG_DIR"
main "$@" 2>&1 | tee "$LOG_DIR/main.log"
