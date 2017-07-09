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
- Cut by half but still needs improvement.

2. Currently a memcpy is issued each time a surface is committed. Needs a way
   to actually only upload texture when necessary aka when surface is absolutely
   visible.

## Improve model
- Pointer / keyboard model could be improved by using seat to track focus instead
  of on per client basis. So that pointer won't remain when leaving the surface.

## Out of order closing of multiple windows will crash the compositor

# Done / Partially Done:
- Copy / Paste device (seems to be working now...)
- Popup grab and proactive dismiss
- Take screenshot: Has basic support with Super + P
- Automatically decide screen resolution: partly done, needs code refactor
- scale: partly done, needs to test for non-scalable apps
- keyboard enter to sub surfaces: not perfect.. subsurface is not focused by default
- Surfaces not completely rendered (e.g. QT's title bar)
- Switching workspace may have a frame remain..
