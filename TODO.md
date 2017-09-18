# Things Todo
## Popups
- Tooltip positions are not correct.. needs investigation. Some of them might be
  related to incomplete xdg\_shell\_v6 popup support for toolkit (like QT). Hopefully
  next release of 5.10 in Nov, 2017 would fix it.

- Popups that have parents are not movable atm.. 
- Window stack order hint.. for apps like Cairo Dock, we'll need to pin it on top of
  everything else.

## panel:
The features on top of my head:

- workspace indicator with workspace snapshot: we have basic workspace atm. Panel
  needs some customizability though.

## Notification
- Use D-Bus to connect to notification daemon and present the surface. But with XWayland
  currently in action, I doubt if it's worth trying this since Dunst seems to work
  quite well with it.

## Improve compositor
Try to use repaint by damage but need to resolve problems with alpha composition.
Compositor view can be maintained inside each surface instead of needing to be
created each time during rendering.

## Improve model
- Pointer / keyboard model could be improved by using seat to track focus instead
  of on per client basis. So that pointer won't remain when leaving the surface. Currently
  we have Seat abstraction. So the event would go in the way:

    WindowManager -> Seat -> Keyboard

## XWayland
- Some surface positioning in XWayland is not correct. There appears to be a race
  in XWayland when pop up surfaces are created and when they are actually positioned
  thus positions of the surfaces are not taken into consideration during actual
  drawing.

- Also X surfaces are positioned top-left corner and moved correctly by compositor.
  This causes issue for popups which uses relative positioning to its parent.

- Hit test for some clients are not correctly when 2 X windows are stacked (e.g. Intellij).

- For Intel graphics card, XWayland seems to run in soft rendering mode.

- Copy paste between X11 windows and Wayland windows.

- No managed toplevel floating window support.. this is needed for apps like
  Lazarus.

## Config
  Refactor code (especially key handlers) in <tt>manage\_hook.cc</tt> to configuration
  files that could be easily modified.

## Other feature

# Done / Partially Done:
- Copy / Paste device (seems to be working now...)
- Popup grab and proactive dismiss
- Take screenshot: Has basic support with Super + P
- Automatically decide screen resolution: partly done, needs code refactor
- scale: partly done, needs to test for non-scalable apps
- keyboard enter to sub surfaces: not perfect.. subsurface is not focused by default
- Surfaces not completely rendered (e.g. QT's title bar)
- Switching workspace may have a frame remain..
- ~~systray~~ Wayland removed the support for systray. But with XWayland, it's
  possible to use any third party systrays like stalonetray or trayer.
- ~~Input method support~~. Text input protocols are not stable in Wayland. I've hacked
  to be able to run Weston sample but not sure how far away it is from real world. Other
  input method can be used if XWayland is enabled.

