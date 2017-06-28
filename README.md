# NaiveWM
Wayland-based  tiling window manager.

## Compilation and Test
You'll need the following dependencies:

    libdrm, EGL, GLES2, libinput, libwayland, pixman, udev, xkbcommon

Then

    mkdir build && cd build
    cmake ..
    make 

If all goes well, an executable called <tt>naive</tt> should be generated.

Switch to another tty (usually with Ctrl + Alt + F[1..9]), cd to the build
directory and run <tt>./naive 2> server\_err</tt>. The IO redirection is 
necessary as currently it generates a large amount of logging information,
which drastically slows down the WM.

## Recommended Configurations
It's recommended that you do the following things:

1. You'll need a wayland-compatible terminal. I'd recommend gnome-terminal
   with Gtk3. To use Gtk3 in Wayland, you'll also need to set 2 environment 
   variables: <tt>GDK\_BACKEND=wayland</tt> and <tt>CLUTTER\_BACKEND=wayland</tt>.
   You can change the command for your terminal in <tt>manage\_hook.cc</tt>

2. You'll need a browser for most of the time. Currently I recommend qutebrowser (built
   with Qt5). To launch it, you can press Win + C in the window manager or type
   <tt>qutebrowser --qt-args platform wayland</tt> in the terminal.

3. It's very likely that your screen resolution is different from mine. unfortunately
   they are hard coded paramters. Just search for 2560 and 1440 in the code and change
   them to your screen's width and height.

## Disclaimer
This project is VERY far from useable. So you may not try it unless you know
what you are doing!
