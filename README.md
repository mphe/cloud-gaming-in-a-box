# Cloud Gaming in a Box

## Setup

1. `git submodule update --init --recursive`
2. Install requirements
    ```sh
    sudo apt install curl tmux golang make
    sudo cloud-morph/setup.sh
    ```
3. Build toxiproxy
    ```sh
    cd toxiproxy
    make build
    ```

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
