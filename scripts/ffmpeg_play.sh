ffplay -protocol_whitelist "file,rtp,udp" -i "${1:-stream.sdp}"
