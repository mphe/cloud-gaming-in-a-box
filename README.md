# Cloud Gaming in a Box

## Setup

<!-- cspell: disable -->
1. `git submodule update --init --recursive`
2. Install requirements
    - NOTE: Requires ffmpeg 5+.
    - Ubuntu
        ```sh
        sudo apt install golang build-essential make cmake libsdl2-dev libsdl2-2.0-0 ffmpeg libavcodec-dev libavutil-dev libavformat-dev libxtst-dev xvfb
        ```
        In addition, download and install virtualgl. See <https://virtualgl.org/vgldoc/2_1_3/#hd004001>.
    - Arch
        ```sh
        sudo pacman -S go base-devel cmake xorg-server-xvfb virtualgl ffmpeg sdl2
        ```
3. `./build.sh`
<!-- cspell: enable -->

## Usage

Basic usage: `./run.sh <application> <application title>`.

See also `./run.sh -h`.


**Example:**
```sh
# On Linux, application title is unused and can be left empty.
# Warsow
./run.sh warsow ""

# Libre Office Calc
./run.sh localc ""
```

## Steam Proton Games

1. Edit the game's launch configuration in steam: `PROTON_DUMP_DEBUG_COMMANDS=1 PROTON_DEBUG_DIR=$HOME %command%`
2. Run the game
3. Steam dumps corresponding run scripts to `~/proton_<user>/` (`gdb_attach`, `gdb_run`, `run`, `winedbg`, `winedbg_run`)
4. Use the generated `run` script as execution target for `run.sh`
    * Optionally, remove the line with `WINEDEBUG="-all"` from `run` script.
6. Set `USE_VIRTUALGL=false` when executing `run.sh` to disable VirtualGL.
    * VirtualGL does not work with Vulkan
    * Proton uses Vulkan
    * Proton/Wine seems to work similarly to VirtualGL in terms of allowing headless rendering without
      using software-rendering.
7. `proton_run.sh` is wrapper for `run.sh` that sets `USE_VIRTUALGL=false`


## Troubleshooting

### Corrupted video

This usually happens with high resolution and/or high quality streams where the data rate is larger
than the maximum OS socket buffer size, hence causing packet loss and corrupted videos.

To increase the system's maximum socket buffer size to 20MB, run:
```sh
$ echo 20971520 | sudo tee /proc/sys/net/core/rmem_max
```

All relevant tools (`frontend`, `udp-proxy`, and `run.sh`) already set the application socket buffer
size to 20MB, which should suffice in most cases.
