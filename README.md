# Virtual Cursor
## Unreal Engine 4 Mouse & Gamepad Cursor Management Plugin

This plugin enables seamless mouse/gamepad cursor input.

For example usage, see [UE4GamepadUMG](https://github.com/bphelan/UE4GamepadUMG).

Original authors: [Nick Darnell and Nick A. at Epic Games](https://forums.unrealengine.com/development-discussion/c-gameplay-programming/58427-example-virtual-analog-cursor-in-umg-slate-destiny-style)

Original plugin creator: [Rama (EverNewJoy)](https://forums.unrealengine.com/community/community-content-tools-and-tutorials/58523-gamepad-friendly-umg-~-control-cursor-with-gamepad-analog-stick-easily-click-buttons)

[Other contributors](https://github.com/bphelan/VirtualCursor/graphs/contributors)

## Notes About This Fork

This fork of the VirtualCursor was modified with the intent to provide support for
local multiplayer without the need for any engine modifications.

### Important Note
If you do not want to make any engine modifications, make sure set `Hide Cursor During Capture`
to `false` when setting the PlayerController's input mode to `UI` or `Game and UI`.

Setting `Hide Cursor During Capture` to `true` will cause Player 1's cursor to jump to the
location of other cursors whenever they click.

This is caused by the following lines in `FSceneViewport::AcquireFocusAndCapture` (present in 4.25)

```
if ( ViewportClient->HideCursorDuringCapture() && bShouldShowMouseCursor )
{
   bCursorHiddenDueToCapture = true;
   MousePosBeforeHiddenDueToCapture = MousePosition;
}
```

And subsequent uses of `MousePosBeforeHiddenDueToCapture`.

Some of these may be removable without engine modification by creating a child of `UGameViewportClient`
and overriding the input functions (such as `OnMouseButtonUp`) to remove references of
`SetMousePos(MousePosBeforeHiddenDueToCapture)`, but this is up to the individual developer.
