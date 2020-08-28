# xmms2-ynca

An XMMS2 client to control Yamaha AV receivers with YNCA (network control) support.

## Features

* Switches input when the track changes.
* Switches sound program to a per-track choice, "Straight" for multi-channel tracks, or some default.
* Configurable using an INI file.
* Shuts down when the XMMS2 daemon does.

## Requirements

Meson is used to build and install. The following additional dependencies are required, including their development files.

* XMMS2
* Boost
* glib

## Compilation

Run the following.

```
mkdir build
cd build
meson
ninja
```

## Installation

Install under your `~/.config/xmms2` directory using Meson.

```
ninja install
```

## Configuration

Edit `~/.config/xmms2/clients/ynca.conf` in your favourite text editor. The settings are documented there.

## Usage

xmms2-ynca can be started manually but when placed in the right directory, it will automatically start and stop with the XMMS2 daemon.

## Author

James ["Chewi"](https://github.com/chewi) Le Cuirot

## License

xmms2-ynca is free software; you can redistribute it and/or modify it under the terms of the [GNU General Public License](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
