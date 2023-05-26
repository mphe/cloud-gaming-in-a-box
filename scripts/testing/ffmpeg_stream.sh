# WIDTH=800
# HEIGHT=600
FPS=30
# FPS=60
WIDTH=1920
HEIGHT=1080

ffmpeg -f x11grab -video_size "${WIDTH}x${HEIGHT}" -framerate "$FPS" -i :0 -draw_mouse 1 \
    -pix_fmt yuv420p \
    -preset ultrafast -tune zerolatency \
    -c:v libx264 \
    -an \
    -quality realtime -f rtp rtp://127.0.0.1:5004
# ffmpeg -framerate "$FPS" -r "$FPS" -f x11grab -draw_mouse 1 -s "${WIDTH}x${HEIGHT}" -i :0 -pix_fmt yuv420p -tune zerolatency -filter:v "crop=$WIDTH:$HEIGHT:0:0" -c:v libx264 -quality realtime -f rtp rtp://127.0.0.1:5004
