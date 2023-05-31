# Cloud Gaming in a Box

## Setup

<!-- cspell: disable -->

1. `git submodule update --init --recursive`
2. Install requirements
    ```sh
    # Ubuntu
    sudo apt install tmux golang build-essential make cmake libsdl2-dev libsdl2-2.0-0 ffmpeg libavcodec-dev libavutil-dev libavformat-dev libxtst-dev xvfb
    # In addition, download and install virtualgl. See <https://virtualgl.org/vgldoc/2_1_3/#hd004001>.

    # Arch
    sudo pacman -S tmux go base-devel cmake xorg-server-xvfb virtualgl ffmpeg sdl2
    ```

3. Build toxiproxy
    ```sh
    cd toxiproxy
    make build
    cd ..
    ```
4. Build frontend and syncinput
    ```sh
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ../src
    make -j
    ```

<!-- cspell: disable -->

## Usage

* `start_toxiproxy.sh [latency_ms]`: Start TCP proxy for input stream with the given delay in milliseconds.
* `start_udp_proxies.sh [delay_s]`: Start all UDP proxies with the given delay in seconds.

## Running

TODO
