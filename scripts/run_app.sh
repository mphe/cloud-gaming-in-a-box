#!/usr/bin/env bash
APP_PATH="$1"
APP_NAME="$2"
HWKEYS="$3"
WIDTH="$4"
HEIGHT="$5"
FPS=30
OUT_DISPLAY=:99
SINK_NAME=fakecloudgame
SINK_ID=
VIDEO_OUT='rtp://127.0.0.1:5004'
AUDIO_OUT='rtp://127.0.0.1:4004'

cleanup() {
    echo "Cleanup"
    [ -n "$SINK_ID" ] && pactl unload-module "$SINK_ID"
    kill $(jobs -p)
}

main() {
    trap cleanup 0  # EXIT

    echo "Xvfb"
    Xvfb "$OUT_DISPLAY" -screen 0 "${WIDTH}x${HEIGHT}x24" &

    echo "PA sink"
    # Create a new pulseaudio sink for the application to prevent playing audio directly to speakers.
    # Check if the sink already exists and remove it to ensure a fresh non-bugged sink
    SINK_ID="$(pactl list sinks | grep -A 6 "Name: $SINK_NAME" | grep 'Owner Module:' | cut -d ' ' -f 3)"
    [ -n "$SINK_ID" ] && pactl unload-module "$SINK_ID"
    SINK_ID="$(pactl load-module module-null-sink sink_name="$SINK_NAME")"

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
    ffmpeg -f pulse -i "$SINK_NAME.monitor" -c:a libopus -b:a 48000 -ssrc 1 \
        -payload_type 111 -f rtp -max_delay 0 -application lowdelay "$AUDIO_OUT" -sdp_file audio.sdp \
        > /dev/null 2>&1 &

    # ffmpeg -f pulse -i cloudmorph.monitor test.wav

    # echo "syncinput"
    # DISPLAY="$OUT_DISPLAY" ./syncinput "$APP_NAME" > syncinput.txt 2>&1 &

    echo "App (ctrl-c to quit)"
    PULSE_SINK="$SINK_NAME" DISPLAY="$OUT_DISPLAY" vglrun "$APP_PATH" "$HWKEYS"
}

main "$@"
