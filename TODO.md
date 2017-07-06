# Things Todo
## tooltip
Tooltip positions are not correct.. needs investigation.

## panel:
The features on top of my head:

- workspace indicator with workspace snapshot 
- systray

## Notification

## Improve compositor
Currently repaints damage. But needs a mechanism to force redraw.

1. Uses too much CPU (6%) when drawing border + global damage for popup surfaces
2. Surfaces not completely rendered (e.g. QT's title bar)
3. Switching workspace may have a frame remain..


## Out of order closing of multiple windows will crash the compositor

# Done / Partially Done:
- Copy / Paste device (seems to be working now...)
- Popup grab and proactive dismiss
- Take screenshot: Has basic support with Super + P
- Automatically decide screen resolution: partly done, needs code refactor
- scale: partly done, needs to test for non-scalable apps
- keyboard enter to sub surfaces: not perfect.. subsurface is not focused by default
