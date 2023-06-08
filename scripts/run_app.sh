#!/usr/bin/env bash
BUILD_DIR="build"
WIDTH=1920
HEIGHT=1080
FPS=60
OUT_DISPLAY=:99
SINK_NAME=fakecloudgame
SINK_ID=
FFMPEG_VIDEO_PORT=5004
FFMPEG_AUDIO_PORT=4004
VIDEO_OUT="rtp://127.0.0.1:$FFMPEG_VIDEO_PORT"
AUDIO_OUT="rtp://127.0.0.1:$FFMPEG_AUDIO_PORT"
SYNCINPUT_IP='127.0.0.1'
SYNCINPUT_PORT=9090
SYNCINPUT_PROTOCOL=udp
FRONTEND_SYNCINPUT_PORT=9091
FRONTEND_VIDEO_PORT=6004
FRONTEND_AUDIO_PORT=7004
# FRONTEND_VIDEO_PORT="$FFMPEG_VIDEO_PORT"
# FRONTEND_AUDIO_PORT="$FFMPEG_AUDIO_PORT"
SERVER_DELAY=0
SERVER_JITTER=0
SERVER_LOSS_START=0.0
SERVER_LOSS_STOP=1.0
CLIENT_DELAY_MS=0
CLIENT_JITTER_MS=0
CLIENT_LOSS_START=0.0
CLIENT_LOSS_STOP=1.0
COMMAND=""

cleanup() {
    echo "Cleanup"

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "stream" ]; then
        [ -n "$SINK_ID" ] && pactl unload-module "$SINK_ID"
    fi

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "proxy" ]; then
        tmux kill-session -t fakecloudgame
    fi

    [ -n "$(jobs -p)" ] && kill $(jobs -p)
}

run_proxies() {
    # UDP
    echo "UDP proxy"
    cd udp-proxy || exit 1
    go build
    local server_args="-d $SERVER_DELAY -j $SERVER_JITTER --loss-start $SERVER_LOSS_START --loss-stop $SERVER_LOSS_STOP"
    tmux new-session -d -s "$SINK_NAME" ./udp-proxy -l "$FFMPEG_AUDIO_PORT" -r "$FRONTEND_AUDIO_PORT" $server_args
    tmux new-window -t "$SINK_NAME" ./udp-proxy   -l "$FFMPEG_VIDEO_PORT" -r "$FRONTEND_VIDEO_PORT" $server_args
    tmux new-window -t "$SINK_NAME" ./udp-proxy   -l "$((FFMPEG_AUDIO_PORT + 1))" -r "$((FRONTEND_AUDIO_PORT + 1))" $server_args
    tmux new-window -t "$SINK_NAME" ./udp-proxy   -l "$((FFMPEG_VIDEO_PORT + 1))" -r "$((FRONTEND_VIDEO_PORT + 1))" $server_args
    tmux set-option -t "$SINK_NAME:0" remain-on-exit failed
    tmux set-option -t "$SINK_NAME:1" remain-on-exit failed
    tmux set-option -t "$SINK_NAME:2" remain-on-exit failed
    tmux set-option -t "$SINK_NAME:3" remain-on-exit failed

    # syncinput UDP
    if [ "$SYNCINPUT_PROTOCOL" == "udp" ]; then
        tmux new-window -t "$SINK_NAME" ./udp-proxy -l "$FRONTEND_SYNCINPUT_PORT" -r "$SYNCINPUT_PORT" -d "$CLIENT_DELAY_MS" -j "$CLIENT_JITTER_MS" --loss-start "$CLIENT_LOSS_START" --loss-stop "$CLIENT_LOSS_STOP"
        tmux set-option -t "$SINK_NAME:4" remain-on-exit failed
    fi

    tmux select-layout tiled
    cd ..

    # syncinput TCP
    if [ "$SYNCINPUT_PROTOCOL" == "tcp" ]; then
        echo "TCP proxy"
        cd toxiproxy || exit 1
        make build
        cd dist || exit 1

        ./toxiproxy-server &
        sleep 1
        ./toxiproxy-cli create --listen "localhost:$FRONTEND_SYNCINPUT_PORT" --upstream "localhost:$SYNCINPUT_PORT" input_proxy
        ./toxiproxy-cli toxic add --type latency --attribute latency="$CLIENT_DELAY_MS" --attribute jitter="$CLIENT_JITTER_MS" input_proxy
        cd ../..
    fi
}

main() {
    trap cleanup 0  # EXIT

    if [ $# -lt 2 ]; then
        echo "Usage: run_app.sh <app path> <app window title> [command]"
        echo -e "    <app path>: Path to the application executable."
        echo -e "    <app window title>: Window title of the application. Used by syncinput to send input events to that window."
        echo -e "    [command]: (Optional) Either 'stream', 'syncinput', 'proxy', or 'frontend'. Only run the specified sub system instead of the whole stack."
        return 0
    fi

    local APP_PATH="$1"
    local APP_TITLE="$2"
    COMMAND="$5"

    echo "Build frontend and syncinput"
    cd "$BUILD_DIR" || exit 1
    make -j
    cd ..

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "stream" ]; then
        echo "PA sink"
        # Create a new pulseaudio sink for the application to prevent playing audio directly to speakers.
        # Check if the sink already exists and remove it to ensure a fresh non-bugged sink
        SINK_ID="$(pactl list sinks | grep -A 6 "Name: $SINK_NAME" | grep 'Owner Module:' | cut -d ' ' -f 3)"
        [ -n "$SINK_ID" ] && pactl unload-module "$SINK_ID"
        SINK_ID="$(pactl load-module module-null-sink sink_name="$SINK_NAME")"

        echo "Xvfb"
        Xvfb "$OUT_DISPLAY" -screen 0 "${WIDTH}x${HEIGHT}x24" &

        echo "App"
        # Flags mentioned in the arch wiki to improve virtualgl performance.
        # https://wiki.archlinux.org/title/VirtualGL#rendering_glitches,_unusually_poor_performance,_or_application_errors
        # VGL_ALLOWINDIRECT=1
        # VGL_FORCEALPHA=1
        # VGL_GLFLUSHTRIGGER=0
        # VGL_READBACK=pbo
        # VGL_SPOILLAST=0
        # VGL_SYNC=1

        # Warsow
        # VGL_ALLOWINDIRECT seems to work good for warsow at least.
        # VGL_ALLOWINDIRECT=1 VGL_REFRESHRATE="$FPS" PULSE_SINK="$SINK_NAME" DISPLAY="$OUT_DISPLAY" vglrun "$APP_PATH" &

        # LibreOffice Calc
        VGL_ALLOWINDIRECT=1 VGL_REFRESHRATE="$FPS" PULSE_SINK="$SINK_NAME" DISPLAY="$OUT_DISPLAY" vglrun "$APP_PATH" --norestore --show &
        sleep 1
        DISPLAY="$OUT_DISPLAY" xdotool search --class libreoffice windowsize %@ 100% 100%  # maximize

        sleep 1

        # Presets and crf
        # https://superuser.com/questions/1556953/why-does-preset-veryfast-in-ffmpeg-generate-the-most-compressed-file-compared
        # https://trac.ffmpeg.org/wiki/Encode/H.264
        #
        # NVidia hardware acceleration
        # ffmpeg -help encoder=hevc_nvenc | less
        # -c:v h264_nvenc -preset llhq -tune hq \
        echo "Video stream at $VIDEO_OUT"
        ffmpeg -threads 0 -r "$FPS" -f x11grab -video_size "${WIDTH}x${HEIGHT}" -framerate "$FPS" -i "$OUT_DISPLAY" -draw_mouse 1 \
            -pix_fmt yuv420p \
            -c:v libx264 -preset ultrafast -tune zerolatency -crf 17 \
            -an \
            -f rtp "$VIDEO_OUT" \
            -sdp_file video.sdp \
            > video.log 2>&1 &

        sleep 1
        sed -i -r "s/$FFMPEG_VIDEO_PORT/$FRONTEND_VIDEO_PORT/" video.sdp

        echo "Audio stream at $AUDIO_OUT"
        ffmpeg -f pulse -fragment_size 16 -i "$SINK_NAME.monitor" \
            -preset ultrafast -tune zerolatency \
            -c:a libopus -b:a 48000 \
            -payload_type 111 -f rtp -max_delay 0 "$AUDIO_OUT" -sdp_file audio.sdp \
            > audio.log 2>&1 &

        sleep 1
        sed -i -r "s/$FFMPEG_AUDIO_PORT/$FRONTEND_AUDIO_PORT/" audio.sdp
    fi

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "syncinput" ]; then
        echo "syncinput"
        DISPLAY="$OUT_DISPLAY" "$BUILD_DIR/syncinput" "$APP_TITLE" "$SYNCINPUT_IP" "$SYNCINPUT_PORT" "$SYNCINPUT_PROTOCOL" 2>&1 | tee syncinput.log &
        sleep 1
    fi

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "proxy" ]; then
        run_proxies
        [ "$COMMAND" == "proxy" ] && tmux attach -t "$SINK_NAME"
    fi

    if [ -z "$COMMAND" ] || [ "$COMMAND" == "frontend" ]; then
        echo "Frontend"
        "$BUILD_DIR/frontend" video.sdp audio.sdp "$SYNCINPUT_IP" "$FRONTEND_SYNCINPUT_PORT" "$SYNCINPUT_PROTOCOL" 2>&1 | tee frontend.log
    else
        # Normally, wait until frontend quits, then kill all child processes.
        # But if the frontend was not started, wait for child processes to end.
        wait
    fi
}

main "$@" 2>&1 | tee main.log
