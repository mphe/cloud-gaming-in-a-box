#!/usr/bin/env bash
FPS=60
OUT_DISPLAY=:99
SINK_NAME=fakecloudgame
SINK_ID=
VIDEO_OUT='rtp://127.0.0.1:5004'
AUDIO_OUT='rtp://127.0.0.1:4004'
SYNCINPUT_IP='127.0.0.1'
SYNCINPUT_PORT=9090
BINARY_PATH="build"

cleanup() {
    echo "Cleanup"
    [ -n "$SINK_ID" ] && pactl unload-module "$SINK_ID"
    kill $(jobs -p)
}

main() {
    trap cleanup 0  # EXIT

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

        echo "Video stream at $VIDEO_OUT"
        ffmpeg -f x11grab -video_size "${WIDTH}x${HEIGHT}" -framerate "$FPS" -i "$OUT_DISPLAY" -draw_mouse 1 \
            -pix_fmt yuv420p \
            -preset ultrafast -tune zerolatency \
            -c:v libx264 \
            -an \
            -quality realtime -f rtp "$VIDEO_OUT" \
            -sdp_file video.sdp \
            > /dev/null 2>&1 &

        echo "Audio stream at $AUDIO_OUT"
        ffmpeg -f pulse -fragment_size 16 -i "$SINK_NAME.monitor" \
            -preset ultrafast -tune zerolatency \
            -c:a libopus -b:a 48000 \
            -payload_type 111 -f rtp -max_delay 0 "$AUDIO_OUT" -sdp_file audio.sdp \
            > /dev/null 2>&1 &

        echo "App"
        PULSE_SINK="$SINK_NAME" DISPLAY="$OUT_DISPLAY" vglrun "$APP_PATH" &
        sleep 1
    fi

    if [ -z "$command" ] || [ "$command" == "syncinput" ]; then
        echo "syncinput"
        DISPLAY="$OUT_DISPLAY" "$BINARY_PATH/syncinput" "$APP_TITLE" "$SYNCINPUT_IP" "$SYNCINPUT_PORT" &
        sleep 1
    fi

    # Normally, wait until frontend quits, then kill all child processes.
    # But, if the frontend was not started, wait for child processes to end.
    if [ -n "$command" ]; then
        wait
    fi

    if [ -z "$command" ] || [ "$command" == "frontend" ]; then
        echo "Frontend"
        "$BINARY_PATH/frontend" video.sdp audio.sdp "$SYNCINPUT_IP" "$SYNCINPUT_PORT"
    fi
}

main "$@"
