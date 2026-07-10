#[allow(
    non_upper_case_globals,
    non_camel_case_types,
    non_snake_case,
    dead_code,
    unnecessary_transmutes
)]
mod eqnx {
    include!("../bindings/eqnx.rs");
}

use std::{ffi::CStr, ptr, slice};

/* On parallelism: As the plugin system is single-thread by design, calling api
 * functions from other threads may derive in unexpected results. It's permitted
 * to use more than one thread, if just the main one do the api calls. For
 * example, another thread can modify globals asynchronously, and the render
 * function can use this globals to update the window. Updating the window from
 * another thread is considered unexpected behaviour.
 *
 * TLDR: The api callbacks are defined in `plugin_api.h` and you can only call
 * api functions from this callbacks. Doing this from other threads is UB.
 */

// If you define this globals, they are assigned at plugin creation.
#[unsafe(no_mangle)]
pub static mut self_window: *mut eqnx::Window = ptr::null_mut();

// Exporting a second mutable void* global
#[unsafe(no_mangle)]
pub static mut self_plugin: *mut eqnx::Plugin = ptr::null_mut();

// This function is called when window is resized. Top left corner is on x,y
// pixels, with w and h pixels width and height. Use window_px_to_coords() to
// get the window size in chars.
#[unsafe(no_mangle)]
extern "C" fn resize(x: i32, y: i32, w: i32, h: i32) {
    // (How to) get size in chars
    let mut cx: i32 = 0;
    let mut cy: i32 = 0;
    let mut cw: i32 = 0;
    let mut ch: i32 = 0;
    unsafe {
        eqnx::window_px_to_coords(x, y, &mut cx, &mut cy);
    }
    unsafe {
        eqnx::window_px_to_coords(w, h, &mut cw, &mut ch);
    }
}

// Keypress event. A key has been pressed
#[unsafe(no_mangle)]
extern "C" fn kp_event(sym: i32, _mods: u32) {
    println!("Keypress {sym}");
    unsafe {
        eqnx::ask_for_redraw();
    }
}

// Pointer event. A mouse event (movement, click, scroll) has happened.
#[unsafe(no_mangle)]
extern "C" fn pointer_event(e: eqnx::Pointer_Event) {
    let a: u32 = e.type_;
    println!("Pointer event {a}");
}

// This function is called when the program request a new frame. You can request
// a new frame from other functions using ask_for_redraw().
#[unsafe(no_mangle)]
extern "C" fn render() {
    // Draw stuff in the window here
    unsafe {
        eqnx::window_clear(self_window, eqnx::BACKGROUND, eqnx::BACKGROUND);
        eqnx::window_puts(
            self_window,
            0,
            0,
            eqnx::FOREGROUND,
            eqnx::BACKGROUND,
            c"Hello from rust!".as_ptr().cast_mut(),
        );
    }
}

// Main function - entry point.
#[unsafe(no_mangle)]
extern "C" fn main(argc: i32, argv: *const *const i8) -> i32 {
    let args_slice = unsafe { slice::from_raw_parts(argv, argc as usize) };

    for (i, &arg_ptr) in args_slice.iter().enumerate() {
        unsafe {
            let c_str = CStr::from_ptr(arg_ptr);
            let s = c_str.to_string_lossy();
            println!("arg[{i}] = {s}");
        }
    }
    /* Your initializations here */

    // Start receiving events. This is a blocking function.
    unsafe {
        eqnx::mainloop();
    }

    /* Your deinitializations here */
    return 0;
}
