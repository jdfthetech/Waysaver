# WaySaver

# THIS IS AI GENERATED USE AT YOUR OWN RISK
# CODE IS HERE TO TEST OTHER AGENTS ABILITY TO READ THIS CODE AND MAKE CHANGES

WaySaver is a C++20 and Qt 6 screensaver service for Hyprland and other modern
Wayland compositors that implement `ext-idle-notify-v1`. It supports image
folders, declarative `.waysaver` files, and trusted Windows `.scr` programs
through Wine.

## What is implemented

* A per-user D-Bus background service with a 1 to 60 minute idle timer
* Native Wayland idle and resume detection with no KDE Framework dependency
* Automatic respect for Wayland idle inhibitors used by browsers and players
* Fullscreen image slideshows on every connected display
* Immediate dismissal on compositor-reported mouse or keyboard activity
* MPRIS detection as a second guard while a media player is playing
* 16 bit NE, 32 bit PE, and 64 bit PE `.scr` format detection
* Wine-based `.scr` execution and the standard `/c` configuration command
* A Qt Widgets settings interface
* A ready-to-copy Waybar module and Hyprland configuration fragment
* An optional Plasma 6 widget for users who also run Plasma
* A versioned JSON package API and command line package creator

## Hyprland behavior

WaySaver connects directly to Hyprland through the standard
`ext-idle-notify-v1` Wayland protocol. It does not run a second `hypridle`
instance and does not replace your existing `hypridle.conf`.

The protocol's normal idle notification honors active Wayland idle inhibitors.
This prevents activation while applications such as browsers and video players
request inhibition. WaySaver also checks all MPRIS players before activation
and rechecks every five seconds until playback stops or the user resumes input.

WaySaver uses the compositor resume event to dismiss both Qt slideshows and
external Wine screensavers when any mouse or keyboard activity is detected.
The local fullscreen windows also dismiss themselves on received input.

## Important limits

Media suppression is cooperative. A video application that provides neither a
Wayland idle inhibitor nor an MPRIS player cannot be identified reliably under
Wayland. Standard browsers and media players normally provide at least one.

Windows `.scr` files are executable programs. Import only files you trust.
WaySaver copies imports with owner-only permissions and starts them with Wine.
Wine can run many 32 bit screensavers under XWayland. Classic 16 bit NE savers
need a separately configured WineVDM-style environment and are not guaranteed
to work. Foreign Windows windows cannot be embedded inside a Qt Wayland surface,
so Wine uses a virtual desktop sized to the complete display area.

WaySaver is a visual saver, not a screen locker. Use `hyprlock` for security.

## Build on Arch Linux

Required packages:

* Qt 6 Core, Gui, Widgets, and D-Bus
* Wayland client libraries, protocols, and scanner
* CMake 3.24 or newer
* GCC with C++20 support

```bash
sudo pacman -S --needed base-devel cmake qt6-base wayland wayland-protocols
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

The quickest user-local installation is:

```bash
./install-local.sh
```

For Windows `.scr` support, also install Wine and its 32 bit dependencies. On
Arch, `wine` requires the `multilib` repository for broad 32 bit compatibility.

## Enable the service

```bash
systemctl --user daemon-reload
systemctl --user enable --now waysaver-daemon.service
```

Open **WaySaver** from your application launcher. If Hyprland is launched with
UWSM, the user service inherits the required Wayland session environment. For a
traditional Hyprland launch, the desktop session should import `WAYLAND_DISPLAY`
and `XDG_RUNTIME_DIR` into the systemd user environment. If it does not, add this
line near the start of `hyprland.conf`:

```ini
exec-once = systemctl --user import-environment WAYLAND_DISPLAY XDG_CURRENT_DESKTOP XDG_RUNTIME_DIR
```

An optional startup and shortcut fragment is included at
`integrations/hyprland/waysaver.conf`.

## Waybar module

Copy the `custom/waysaver` object from
`integrations/waybar/config.jsonc` into your Waybar configuration and add
`"custom/waysaver"` to the desired modules list. Append the included CSS to
your Waybar stylesheet.

The module provides these controls:

* Left click starts the screensaver
* Right click opens settings
* Middle click stops the screensaver

It uses `waysaver --status-json`, which is also suitable for other status bars.

## Local-prefix installation

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build -j"$(nproc)"
cmake --install build
systemctl --user daemon-reload
systemctl --user enable --now waysaver-daemon.service
```

The generated D-Bus and systemd service files contain the selected installation
prefix.

## Package API

A `.waysaver` file is a UTF-8 JSON manifest. Version 1 supports an image
directory or a native executable. Paths may be absolute, but portable packages
should use paths relative to the manifest.

```json
{
  "schema": "org.waysaver.package/v1",
  "id": "org.example.family-photos",
  "name": "Family Photos",
  "version": 1,
  "saver": {
    "type": "images",
    "source": "photos",
    "slideSeconds": 12
  }
}
```

Create the file through the command line API:

```bash
waysaver --create-package \
  --name "Family Photos" \
  --id org.example.family-photos \
  --type images \
  --source photos \
  --slide-seconds 12 \
  --output family-photos.waysaver
```

For a program-driven saver, use `"type": "executable"`. WaySaver starts the
program with `--waysaver-fullscreen`. The program must create its own fullscreen
Wayland surfaces and exit when it receives SIGTERM. This is an execution API,
so packages from untrusted sources must not be imported.

The same API is exposed in C++ by `PackageApi::create()` and
`PackageApi::validate()` in `src/packageapi.h`. A formal JSON Schema is included
at `api/waysaver-package-v1.schema.json`.

## D-Bus API

Service: `org.waysaver.Service`  
Object: `/WaySaver`  
Interface: `org.waysaver.Service`

Methods: `Activate`, `Deactivate`, `Reload`, `IsActive`, and `Status`.

```bash
qdbus6 org.waysaver.Service /WaySaver org.waysaver.Service.Activate
```

## License

GPL-3.0-or-later. See `LICENSE`.

