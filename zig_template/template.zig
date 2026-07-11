const std = @import("std");

const eqnx = @cImport({
    @cInclude("plug_api.h");
    @cInclude("theme.h");
});

// On parallelism: As the plugin system is single-thread by design, calling api
// functions from other threads may derive in unexpected results. It's permitted
// to use more than one thread, if just the main one do the api calls. For
// example, another thread can modify globals asynchronously, and the render
// function can use this globals to update the window. Updating the window from
// another thread is considered unexpected behaviour.

// TLDR: The api callbacks are defined in `plugin_api.h` and you can only call
// api functions from this callbacks. Doing this from other threads is UB.

// If you define this globals, they are assigned at plugin creation.
pub export var self_window: *eqnx.Window = undefined;
pub export var self_plugin: *eqnx.Plugin = undefined;

// This function is called when window is resized. Top left corner is on px,py
// pixels, with pw,ph pixels width and height. Use window_px_to_coords() to
// get the window size in chars.
pub export fn resize(px: i32, py: i32, pw: i32, ph: i32) callconv(.c) i32 {
    // (How to) get size in chars
    var cx: i32 = undefined;
    var cy: i32 = undefined;
    var cw: i32 = undefined;
    var ch: i32 = undefined;
    eqnx.window_px_to_coords(px, py, &cx, &cy);
    eqnx.window_px_to_coords(pw, ph, &cw, &ch);
    0
}

// Keypress event. A key has been pressed
pub export fn kp_event(sym: i32, mods: i32) callconv(.c) i32 {
    std.debug.print("Pressed: {} | {}\n", .{ sym, mods });
    0
}

// Pointer event. A mouse event (movement, click, scroll) has happened.
pub export fn pointer_event(e: eqnx.Pointer_Event) callconv(.c) i32 {
    std.debug.print("Pointer event: {}\n", .{e.type});
    0
}

// This function is called when the program request a new frame. You can request
// a new frame from other functions using ask_for_redraw().
pub export fn render() callconv(.c) i32 {
    // Draw stuff in the window here
    std.debug.print("window clear?\n", .{});
    eqnx.window_clear(self_window, eqnx.BACKGROUND, eqnx.BACKGROUND);
    std.debug.print("done\n", .{});
    std.debug.print("window_printf?\n", .{});
    _ = eqnx.window_printf(self_window, 0, 0, eqnx.FOREGROUND, eqnx.BACKGROUND, @constCast("%s"), "Hello from zig");
    std.debug.print("done\n", .{});
    0
}

// Main function - entry point.
pub export fn main(argc: usize, argv: [*c][*c]u8) callconv(.c) i32 {
    for (0..argc) |i| {
        std.debug.print("arg[{}] = {s}\n", .{ i, argv[i] });
    }

    // Your initializations here

    // Start receiving events. This is a blocking function.
    eqnx.mainloop();

    // Your deinitializations here
    return 0;
}
