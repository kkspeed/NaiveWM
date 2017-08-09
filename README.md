# NaiveWM
Wayland-based  tiling window manager.

![Desktop](https://raw.githubusercontent.com/kkspeed/NaiveWM/master/images/screenshot_0.png)
![Screenshot](https://raw.githubusercontent.com/kkspeed/NaiveWM/master/images/screenshot_1.png)

## Compilation and Test
### Basics
You'll need the following dependencies:

    libdrm, EGL, GLES2, libinput, libwayland, pixman, udev, xkbcommon, libpng, glog

The packages needed under Fedora are:

    libdrm, egl-utils, mesa-libGLES-devel, libinput-devel, mesa-libwayland-egl, 
	pixman, systemd-udev, libxkbcommon, glog-devel, mesa-libgbm-devel, glflags-devel

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

1. You'll need a wayland-compatible terminal. I'd recommend gnome-terminal
   with Gtk3. To use Gtk3 in Wayland, you'll also need to set 2 environment 
   variables: <tt>GDK\_BACKEND=wayland</tt> and <tt>CLUTTER\_BACKEND=wayland</tt>.
   You can change the command for your terminal in <tt>manage\_hook.cc</tt>

2. You'll need a browser for most of the time. Currently I recommend qutebrowser (built
   with Qt5). To launch it, you can press Win + C in the window manager or type
   <tt>qutebrowser --qt-args platform wayland</tt> in the terminal.

3. You'll need to specify a wallpaper file via variable <tt>kWallpaperPath</tt> in 
   <tt>manage\_hook.cc</tt> and uncomment the code in <tt>PostWmInitialize</tt> to
   enable wallpaper.

## Default Configurations
Here are a list of key bindings and actions. They can be changed in manage\_hook.cc

| Key                 | Action                                                 |
|---------------------|--------------------------------------------------------|
|Super + C            | Launches browser (default is qutebrowser)              |
|Super + T            | Launches terminal (default is gnome-terminal           |
|Super + J            | Focus next window                                      |
|Super + K            | Focus previous window                                  |
|Super + Shift + Q    | Exit                                                   |
|Super + Enter        | Bumps current window to head of the list               |
|Super + P            | Save screenshot. Currently it saves to /tmp/output.png |
|Super + 1..9         | Switch workspace to 1..9                               |
|Super + Shift + 1..9 | Move current window to workspace 1..9                  |

NaiveWM should be able to guess the scale of your screen for high res screens.

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

