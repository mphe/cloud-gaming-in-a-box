ffmpeg -r 30 -f x11grab -draw_mouse 0 -s 800x600 -i :0 -pix_fmt yuv420p -tune zerolatency -filter:v "crop=800:600:0:0" -c:v libx264 -quality realtime -f rtp rtp://127.0.0.1:5004
