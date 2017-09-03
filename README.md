# NaiveWM
Wayland-based  tiling window manager.

![Desktop](https://raw.githubusercontent.com/kkspeed/NaiveWM/master/images/screenshot_0.png)
![Screenshot](https://raw.githubusercontent.com/kkspeed/NaiveWM/master/images/screenshot_1.png)

## Compilation and Test
### Basics
You'll need the following dependencies:

    libdrm, EGL, GLES2, libinput, libwayland, pixman, udev, xkbcommon, libpng, glog,
    glm, dbus, cairomm, xlib, Xcomposite

The packages needed under Fedora are:

    libdrm, egl-utils, mesa-libGLES-devel, libinput-devel, mesa-libwayland-egl, 
	pixman, systemd-udev, libxkbcommon, glog-devel, mesa-libgbm-devel, glflags-devel,


Then

    mkdir build && cd build
    cmake ..
    make 

If all goes well, an executable called <tt>naive</tt> should be generated.

Switch to another tty (usually with Ctrl + Alt + F[1..9]), cd to the build
directory and run <tt>./naive 2> server\_err</tt>. The IO redirection is 
necessary as currently it generates a large amount of logging information,
which drastically slows down the WM.

If you are unable to move mouse or type anything, it's likely that you are
not in <tt>input</tt> group. Add yourself by <tt>gpasswd -a your\_name 
input</tt>.

### Reduce CPU Load by Using More Efficient Compositor
On my Thinkpad X1 Carbon 2016, it uses roughly 6% CPU when idle. It is
because that it redraws fullscreen each frame. A more efficient compositor
is being developed but is currently disabled by default. To enable it, remove
the <tt>-DNAIVE\_COMPOISTOR</tt> from <tt>CMAKE\_CXX\_FLAGS</tt> in 
<tt>CMakeLists.txt</tt>.

## Recommended Configurations
It's recommended that you do the following things:

1. You'll need a wayland-compatible terminal. I'd recommend lxterminal
   with Gtk3. To use Gtk3 in Wayland, you'll also need to set 2 environment 
   variables: <tt>GDK\_BACKEND=wayland</tt> and <tt>CLUTTER\_BACKEND=wayland</tt>.
   You can change the command for your terminal in <tt>manage\_hook.cc</tt>

2. You'll need a browser for most of the time. Currently I recommend qutebrowser (built
   with Qt5). To launch it, you can press Win + C in the window manager or type
   <tt>qutebrowser --qt-args platform wayland</tt> in the terminal.

3. You'll need to specify a wallpaper file via variable <tt>kWallpaperPath</tt> in 
   <tt>manage\_hook.cc</tt> and uncomment the code in <tt>PostWmInitialize</tt> to
   enable wallpaper.

4. You can uncomment the panel code in <tt>manage\_hook.cc</tt> to enable panel.
   You can modify <tt>panel.cc</tt> to tweak it (sorry for not having good configration
   file). Currently it offers a battery indicator (needs UPower), a clock and
   a workspace indicator.

## Default Configurations
Here are a list of key bindings and actions. They can be changed in manage\_hook.cc

| Key                 | Action                                                                      |
|---------------------|-----------------------------------------------------------------------------|
|Super + C            | Launches browser (default is qutebrowser)                                   |
|Super + T            | Launches terminal (default is lxterminal                                    |
|Super + D            | Launches drop down terminal (default is lxterminal)                         |
|Super + J            | Focus next window                                                           |
|Super + K            | Focus previous window                                                       |
|Super + Shift + Q    | Exit                                                                        |
|Super + Enter        | Bumps current window to head of the list                                    |
|Super + P            | Save screenshot. Currently it saves to /tmp/screnshot-\<current\_time\>.png |
|Super + 1..9         | Switch workspace to 1..9                                                    |
|Super + Shift + 1..9 | Move current window to workspace 1..9                                       |
|Super + x            | Start XWayland (Needs to be compiled with <tt>NO\_XWAYLAND</tt>. See below) |
|Super + Tab          | Jump to previous tag                                                        |

NaiveWM should be able to guess the scale of your screen for high res screens.

## Run X Apps
NaiveWM currently has limited built-in XWayland support. The surfaces seems to be placed correctly
but that's about it.. some edge cases like transient child windows are not handled correctly. 

![XWaylandMix](https://raw.githubusercontent.com/kkspeed/NaiveWM/master/images/xwayland_mix.png)

Alternatively you can use XWayland like a remote desktop. In such case, please compile
with <tt> -DNO\_XWAYLAND</tt>.

To start XWayland, run <tt>Xwayland +iglx :1</tt> or press <tt>Super + X</tt>.

Likely you'll need a X window manager to manage X windows. I suggest using DWM:

    export DISPLAY=:1
    dwm &

And you can see dwm is running in XWayland. Press <tt>Alt + P</tt> to start dmenu,
which can help you launch X applications:

![XWayland](https://raw.githubusercontent.com/kkspeed/NaiveWM/master/images/xwayland.png)

## Run NaiveWM Under X Server
NaiveWM comes with an X server backend that runs as a regular window in any X desktop.
This makes debugging a little bit easier.

To use this mode, simply compile and run <tt>./naive -x11 :0</tt>. <tt>:0</tt> is the display
for your X server, which you can get from <tt>DISPLAY</tt> environment variable. 
You should not use <tt>:1</tt> as it's reserved for Xwayland.  Note that I have swapped
*Super* and *Alt* key in order not to interfere with my X11's tiling window setup.

Inside the session, you'll need to set <tt>DISPLAY</tt> to <tt>:1</tt> in order to use
XWayland (otherwise it just connects to your regular X11 session).

Also, you'll need to tweak the resolution for your screen. It's defined as 
<tt>kWidthPixels, kHeightPixels, kScaleFactor</tt> in
<tt>backend/x11\_backend/x11\_backend.cc</tt>.

![X11 Backend](https://raw.githubusercontent.com/kkspeed/NaiveWM/master/images/x11_backend.png)

## Disclaimer
This project is VERY far from useable. So you may not try it unless you know
what you are doing!

For a list of things I'm working on, please see <tt>TODO.md</tt>.

## Credits
Sincerely thanks to all the open source libraries that back up this project. 

Part of the Wayland server implementation is heavily based on the 
[Chromium project](https://www.chromium.org/).

The compositor implementation is heavily based on 
[kmscube](https://github.com/robclark/kmscube/blob/master/kmscube.c). 

The PNG encoder is almost (shamelessly) copied from
[https://www.lemoda.net/c/write-png/](https://www.lemoda.net/c/write-png/).

