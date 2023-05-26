# ffplay -infbuf -max_delay 1000000 -protocol_whitelist "file,rtp,udp" -i stream.sdp
ffplay -protocol_whitelist "file,rtp,udp" -i stream.sdp
