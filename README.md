# Cloud Gaming in a Box

## Setup

<!-- cspell: disable -->

1. `git submodule update --init --recursive`
2. Install requirements
    ```sh
    sudo apt install curl tmux golang build-essential make cmake libsdl2-dev libsdl2-2.0-0 ffmpeg libavcodec-dev libavutil-dev libavformat-dev libxtst-dev xvfb
    ```
    Install virtualgl: <https://virtualgl.org/vgldoc/2_1_3/#hd004001>

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

* `change_cloud_morph_ports.sh`: Redirect streaming ports.
* `change_cloud_morph_ports.sh reset`: Reset streaming ports to their original value.
* `start_toxiproxy.sh [latency_ms]`: Start TCP proxy for input stream with the given delay in milliseconds.
* `run_cloud_morph.sh`: Start cloud-morph at <http://localhost:8080>.
* `udp_proxy.py`: Start UDP proxy. See also `udp_proxy.py -h`.
* `start_udp_proxies.sh [delay_s]`: Start all UDP proxies with the given delay in seconds.

## Running

1. `./change_cloud_morph_ports.sh`
2. `./start_toxiproxy.sh`
3. `./start_udp_proxies.sh`
4. `./run_cloud_morph.sh`
