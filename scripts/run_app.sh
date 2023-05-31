#!/usr/bin/env bash
FPS=144
OUT_DISPLAY=:99
SINK_NAME=fakecloudgame
SINK_ID=
VIDEO_OUT='rtp://127.0.0.1:5004'
AUDIO_OUT='rtp://127.0.0.1:4004'
SYNCINPUT_IP='127.0.0.1'
SYNCINPUT_PORT=9090
BINARY_PATH="build"
PROTOCOL=udp

cleanup() {
    echo "Cleanup"
    [ -n "$SINK_ID" ] && pactl unload-module "$SINK_ID"
    kill $(jobs -p)
}

main() {
    trap cleanup 0  # EXIT

    if [ $# -lt 4 ]; then
        echo "Usage: run_app.sh <app path> <app window title> <width> <height> [command]"
        echo -e "    <app path>: Path to the application executable."
        echo -e "    <app window title>: Window title of the application."
        echo -e "    <width>: Width of the virtual display."
        echo -e "    <height>: Height of the virtual display."
        echo -e "    [command]: (Optional) One of either stream, syncinput, frontend. Only run the specified sub system instead of the whole stack."
        return 0
    fi

    local APP_PATH="$1"
    local APP_TITLE="$2"
    local WIDTH="$3"
    local HEIGHT="$4"
    local command="$5"

    if [ -z "$command" ] || [ "$command" == "stream" ]; then
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

        # VGL_ALLOWINDIRECT seems to work good for warsow at least.
        VGL_ALLOWINDIRECT=1 VGL_REFRESHRATE="$FPS" PULSE_SINK="$SINK_NAME" DISPLAY="$OUT_DISPLAY" vglrun "$APP_PATH" &

        sleep 1

        echo "Video stream at $VIDEO_OUT"
                    # -crf 0 \
        ffmpeg -threads 2 -r "$FPS" -f x11grab -video_size "${WIDTH}x${HEIGHT}" -framerate "$FPS" -i "$OUT_DISPLAY" -draw_mouse 1 \
            -pix_fmt yuv420p \
            -preset ultrafast -tune zerolatency \
            -c:v libx264 \
            -quality realtime \
            -an \
            -f rtp "$VIDEO_OUT" \
            -sdp_file video.sdp \
            > /dev/null 2>&1 &

        echo "Audio stream at $AUDIO_OUT"
        ffmpeg -f pulse -fragment_size 16 -i "$SINK_NAME.monitor" \
            -preset ultrafast -tune zerolatency \
            -c:a libopus -b:a 48000 \
            -payload_type 111 -f rtp -max_delay 0 "$AUDIO_OUT" -sdp_file audio.sdp \
            > /dev/null 2>&1 &
    fi

    if [ -z "$command" ] || [ "$command" == "syncinput" ]; then
        echo "syncinput"
        DISPLAY="$OUT_DISPLAY" "$BINARY_PATH/syncinput" "$APP_TITLE" "$SYNCINPUT_IP" "$SYNCINPUT_PORT" "$PROTOCOL" &
        sleep 1
    fi

    # Normally, wait until frontend quits, then kill all child processes.
    # But, if the frontend was not started, wait for child processes to end.
    if [ -n "$command" ]; then
        wait
    fi

    if [ -z "$command" ] || [ "$command" == "frontend" ]; then
        echo "Frontend"
        "$BINARY_PATH/frontend" video.sdp audio.sdp "$SYNCINPUT_IP" "$SYNCINPUT_PORT" "$PROTOCOL"
    fi
}

main "$@"
