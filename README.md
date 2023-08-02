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
        sudo pacman -S go base-devel cmake xorg-server-xvfb virtualgl ffmpeg sdl2 xdotool
        ```
3. `./build.sh`
<!-- cspell: enable -->

## Usage

Basic usage: `./run.sh <application> [subsystems]`.

See also `./run.sh -h` for advanced usage.


**Example:**
```sh
# Warsow
./run.sh warsow

# Libre Office Calc with additional arguments
./run.sh localc "" --norestore --show
```

`./scenarios/` contains wrapper scripts for various network scenarios.
`./apps/` contains some example scripts for starting applications.
`./cfg/` contains some wrapper scripts for run-configurations, like vsync.

**Example:**
```sh
# Warsow with Vsync enabled
cfg/vsync.sh ./run.sh warsow

# Warsow with Vsync enabled and a delay scenario
scenarios/delay1.sh cfg/vsync.sh ./run.sh warsow

# Libre Office Calc
./run.sh apps/localc.sh

# A Hat in Time with Vsync enabled
cfg/vsync.sh cfg/proton.sh ./run.sh apps/hatintime.sh
```

`run.sh` also supports running only specific subsystems. This can be used to specifically start and
kill only selected parts of the testbed.

For example, when testing multiple network scenarios, the application and streaming components can
be run in one session, and the remaining subsystems in another session.
This allows to quickly kill and restart the "frontend" with another configuration without having to
restart the whole game.

When starting only certain subsystems, obsolete parameters have no effect and can be left empty.

**Example:**
```sh
# Run A Hat in Time, but only the application and streaming components ("backend")
cfg/proton.sh ./run.sh apps/hatintime.sh app,stream

# (In another terminal) Run remaining components with delay scenario 1.
scenarios/delay1.sh cfg/vsync.sh ./run.sh "" proxy,syncinput,frontend

# Press ctrl-c anytime

# Attach again with delay scenario 3, without killing the game process.
scenarios/delay3.sh cfg/vsync.sh ./run.sh "" proxy,syncinput,frontend
```

## Compatibility

The testbed only works on Linux, but all tools are written in a platform independant manner, so they
can be ported to other platforms rather easily.

The `syncinput` tool needs additional backends for platforms other than Linux/X11.


## Running Steam Proton Games

1. Edit the game's launch configuration in steam: `PROTON_DUMP_DEBUG_COMMANDS=1 PROTON_DEBUG_DIR=$HOME %command%`
2. Run the game
3. Steam dumps corresponding run scripts to `~/proton_<user>/` (`gdb_attach`, `gdb_run`, `run`, `winedbg`, `winedbg_run`)
4. Use the generated `run` script as execution target for `run.sh`
    - Optionally, remove the line with `WINEDEBUG="-all"` from `run` script.
5. Set `USE_VIRTUALGL=false` when executing `run.sh` to disable VirtualGL.
    - VirtualGL does not work with Vulkan
    - Proton uses Vulkan
    - Proton/Wine seems to work similarly to VirtualGL in terms of allowing hardware acceleration in `Xvfb`
6. `cfg/proton.sh` is a wrapper script that sets `USE_VIRTUALGL=false` and can be used for simplicity.


## Troubleshooting

### Corrupted video

This usually happens with high resolution and/or high quality streams where the data rate is larger
than the maximum OS socket buffer size, hence causing packet loss and corrupted videos.

All relevant tools (`frontend`, `udp-proxy`, and `run.sh`) already set the application socket buffer
size to 20MB, which should suffice in most cases.

To increase the system's maximum socket buffer size to 20MB, run:
```sh
$ echo 20971520 | sudo tee /proc/sys/net/core/rmem_max
```

To make this setting permanent, open `/etc/sysctl.conf` or create a file
`/etc/sysctl.d/50-socketsize.conf` and add the following content.

```sh
# Increase maximum socket buffer size to 20MB
net.core.rmem_max = 20971520
```

Then run `sudo sysctl --system` to load the new config.
