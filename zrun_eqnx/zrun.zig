const std = @import("std");

extern var environ: [*:null]?[*:0]u8;

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

// This function is called when window is resized. Top left corner is on x,y
// pixels, with w and h pixels width and height. Use window_px_to_coords() to
// get the window size in chars.
pub export fn resize(_: i32, _: i32, w: i32, h: i32) callconv(.c) void {
    var cw: i32 = undefined;
    var ch: i32 = undefined;
    eqnx.window_px_to_coords(w, h, &cw, &ch);
    ctx.cols = @intCast(cw);
    ctx.rows = @intCast(ch);
    eqnx.ask_for_redraw();
}

// Keypress event. A key has been pressed
pub export fn kp_event(sym: i32, mods: i32) callconv(.c) void {
    if (sym == eqnx.XKB_KEY_BackSpace) {
        _ = ctx.input.pop();
    } else if (sym == eqnx.XKB_KEY_Return) {
        select_and_run();
    } else if (sym == eqnx.XKB_KEY_Escape) {
        std.process.exit(0);
    } else if ((sym == 'j' and eqnx.mod_has_Control(mods)) or sym == eqnx.XKB_KEY_Down) {
        ctx.selected += 1;
    } else if ((sym == 'k' and eqnx.mod_has_Control(mods)) or sym == eqnx.XKB_KEY_Up) {
        ctx.selected -= 1;
    } else if (sym >= ' ' and sym < 127 and std.ascii.isAlphanumeric(@intCast(sym))) {
        var ch: u8 = @intCast(sym);
        if (ctx.ignorecase) ch = std.ascii.toLower(ch);
        ctx.input.append(ctx.allocator, ch) catch {};
    }
    eqnx.ask_for_redraw();
}

// Pointer event. A mouse event (movement, click, scroll) has happened.
pub export fn pointer_event(e: eqnx.Pointer_Event) callconv(.c) void {
    std.debug.print("Pointer event: {}\n", .{e.type});
}

// This function is called when the program request a new frame. You can request
// a new frame from other functions using ask_for_redraw().
pub export fn render() callconv(.c) void {
    eqnx.window_clear(self_window, eqnx.BACKGROUND, eqnx.BACKGROUND);
    update_screen() catch {};
}

// fn get_time() f64 {
//     var tp: std.os.linux.timespec = undefined;
//     _ = std.os.linux.clock_gettime(std.os.linux.CLOCK.REALTIME, &tp);
//     return @as(f64, @floatFromInt(tp.sec)) +
//         @as(f64, @floatFromInt(tp.nsec)) /
//             @as(f64, 1e9);
// }
//
// var start_time: f64 = undefined;
//
// inline fn debug(what: []const u8, args: anytype) void {
//     std.debug.print("debug: [{d:.6}s] ", .{get_time() - start_time});
//     std.debug.print(what, args);
// }

inline fn info(what: []const u8, args: anytype) void {
    std.debug.print("info: ", .{});
    std.debug.print(what, args);
}

// autocalculated things
const Ctx = struct {
    prompt: [*c]const u8 = "search: ", // default prompt
    prompt_color: u32 = eqnx.FOREGROUND,
    text_color: u32 = eqnx.FOREGROUND,
    bg_color: u32 = eqnx.BACKGROUND,
    selected_text_color: u32 = eqnx.MAGENTA,
    rows: usize = 0,
    cols: usize = 0,
    entry_list: List = .{},
    input: std.ArrayList(u8) = .empty,
    allocator: std.mem.Allocator,
    margin_top: i32 = 1,
    margin_left: i32 = 3,
    margin_entry: i32 = 4,
    selected: i32 = 0,
    selected_entry: Entry = .{},
    ignorecase: bool = true,
};

var ctx: Ctx = undefined;

const Entry = struct {
    efec_name: ?[]const u8 = null,
    real_name: ?[:0]const u8 = null,
    exec: ?[]const u8 = null,
    icon: ?[]const u8 = null,
    terminal: ?[]const u8 = null,
    gap: i32 = 0,

    pub fn match(e: *Entry, text: []const u8) bool {
        var i: usize = 0;
        var gap: i32 = 0;
        if (e.efec_name.?.len < text.len) return false;
        for (text) |ch| {
            for (i..e.efec_name.?.len) |j| {
                if (e.efec_name.?[j] == ch or ctx.ignorecase and
                    0 < ch and ch < 128 and
                    0 < e.efec_name.?[j] and e.efec_name.?[j] < 128 and
                    std.ascii.toLower(ch) == std.ascii.toLower(e.efec_name.?[j]))
                {
                    i = j + 1;
                    if (gap <= 0) {
                        gap -= 1;
                    } else {
                        gap = 0;
                    }
                    e.gap += gap;
                    break;
                }
                gap += 2;
            } else {
                return false;
            }
        }
        return true;
    }

    pub fn dup(e: Entry) !Entry {
        var e2: Entry = e;
        if (e.efec_name) |v| e2.efec_name = try ctx.allocator.dupe(u8, v);
        if (e.real_name) |v| e2.real_name = try ctx.allocator.dupeZ(u8, v);
        if (e.exec) |v| e2.exec = try ctx.allocator.dupe(u8, v);
        if (e.icon) |v| e2.icon = try ctx.allocator.dupe(u8, v);
        if (e.terminal) |v| e2.terminal = try ctx.allocator.dupe(u8, v);
        return e2;
    }

    pub fn destroy(e: Entry) void {
        if (e.efec_name) |v| ctx.allocator.free(v);
        if (e.real_name) |v| ctx.allocator.free(v);
        if (e.exec) |v| ctx.allocator.free(v);
        if (e.icon) |v| ctx.allocator.free(v);
        if (e.terminal) |v| ctx.allocator.free(v);
    }
};

fn entry_less_than(_: void, e1: Entry, e2: Entry) bool {
    return e1.gap < e2.gap;
}

inline fn extractValue(content: []const u8, comptime prefix: []const u8) ?[]const u8 {
    if (std.mem.startsWith(u8, content, prefix)) {
        if (content.len > prefix.len) {
            return std.mem.trim(u8, content[prefix.len..], &std.ascii.whitespace);
        }
    }
    return null;
}

pub fn exec_trim_from_args(ch: *[]u8, target: []const u8) void {
    if (std.mem.indexOf(u8, ch.*, target)) |index| {
        const remaining_start = index + target.len;
        const new_len = ch.*.len - target.len;
        std.mem.copyForwards(u8, ch.*[index..new_len], ch.*[remaining_start..]);
        ch.* = ch.*[0..new_len];
    }
}

fn parse_entry(name: []const u8, content: []const u8) !?Entry {
    var values: Entry = .{};
    var lines = std.mem.splitScalar(u8, content, '\n');

    while (lines.next()) |line| {
        const c = std.mem.trim(u8, line, &std.ascii.whitespace);

        if (extractValue(c, "Name=")) |value| {
            if (values.efec_name) |_| continue;
            values.efec_name = try ctx.allocator.dupe(u8, value);
            values.real_name = try ctx.allocator.dupeZ(u8, value);
            continue;
        }

        if (extractValue(c, "Exec=")) |value| {
            if (values.exec) |_| continue;
            var mut_value = @constCast(value);
            exec_trim_from_args(&mut_value, " %f");
            exec_trim_from_args(&mut_value, " %F");
            exec_trim_from_args(&mut_value, " %u");
            exec_trim_from_args(&mut_value, " %U");
            exec_trim_from_args(&mut_value, " %d");
            exec_trim_from_args(&mut_value, " %D");
            exec_trim_from_args(&mut_value, " %n");
            exec_trim_from_args(&mut_value, " %N");
            exec_trim_from_args(&mut_value, " %i");
            exec_trim_from_args(&mut_value, " %c");
            exec_trim_from_args(&mut_value, " %k");
            exec_trim_from_args(&mut_value, " %v");
            exec_trim_from_args(&mut_value, " %m");
            values.exec = try ctx.allocator.dupe(u8, mut_value);
            continue;
        }

        if (extractValue(c, "Icon=")) |value| {
            if (values.icon) |_| continue;
            values.icon = try ctx.allocator.dupe(u8, value);
            continue;
        }

        if (extractValue(c, "Terminal=")) |value| {
            if (values.terminal) |_| continue;
            values.terminal = try ctx.allocator.dupe(u8, value);
            continue;
        }

        if (extractValue(c, "NoDisplay=")) |value| {
            if (std.mem.eql(u8, value, "true")) {
                values.destroy();
                return null;
            }
            if (std.mem.eql(u8, value, "false")) {} else {
                return error.ValueIsNeitherTrueNorFalse;
            }
            continue;
        }
    }

    if (values.efec_name) |_| {
        return values;
    } else {
        info("Entry {s} has no name\n", .{name});
        values.destroy();
        return null;
    }
}

const List = struct {
    list: std.ArrayList(Entry) = .empty,

    pub fn get(l: List, i: usize) Entry {
        std.debug.assert(l.list.items.len > i);
        return l.list.items[i];
    }

    pub fn append(l: *List, e: Entry) !void {
        try l.list.append(ctx.allocator, e);
    }

    pub fn destroy(l: *List) void {
        for (l.list.items) |e| {
            e.destroy();
        }
        l.list.deinit(ctx.allocator);
    }

    pub fn filter(l: List, text: []const u8) !List {
        var lnew: List = .{};
        for (0..l.list.items.len) |i| {
            var e: Entry = l.get(i);
            if (e.match(text)) {
                try lnew.append(try e.dup());
            }
        }
        std.mem.sort(Entry, lnew.list.items, {}, comptime entry_less_than);
        return lnew;
    }
};

fn parse_desktop_dir(l: *List, path: []const u8) !void {
    const io = std.Io.Threaded.global_single_threaded.io();
    const cwd = std.Io.Dir.cwd();

    var dir = try cwd.openDir(io, path, .{ .iterate = true });
    defer dir.close(io);

    var iter = dir.iterateAssumeFirstIteration();

    while (try iter.next(io)) |entry| {
        if (!std.mem.endsWith(u8, entry.name, ".desktop")) continue;
        const content = dir.readFileAlloc(io, entry.name, ctx.allocator, .unlimited) catch |e| {
            info("Error reading {s}: {}\n", .{ entry.name, e });
            continue;
        };
        defer ctx.allocator.free(content);
        if (try parse_entry(entry.name, content)) |e| {
            try l.append(e);
        }
    }
}

fn update_screen() !void {
    var position: [2]i32 = .{ ctx.margin_left, ctx.margin_top };

    // this chunk is just for concat two cstr
    const p_slice = std.mem.span(ctx.prompt);
    // todo: the following two lines should be moved to something more performant
    const i_slice = try ctx.input.toOwnedSlice(ctx.allocator); // this empty the array
    try ctx.input.appendSlice(ctx.allocator, i_slice); // this refill the array
    defer ctx.allocator.free(i_slice);
    const parts = &[_][]const u8{ p_slice, i_slice };
    const prompt: []const u8 = try std.mem.concat(ctx.allocator, u8, parts);
    const prompt_z: [:0]const u8 = try ctx.allocator.dupeZ(u8, prompt);
    defer ctx.allocator.free(prompt);
    defer ctx.allocator.free(prompt_z);

    _ = self_window.window_puts(position[0], position[1], ctx.prompt_color, ctx.bg_color, @constCast(prompt_z));

    var filtered = try ctx.entry_list.filter(i_slice);
    defer filtered.destroy();

    if (filtered.list.items.len <= 0) return;

    // sanity oob check
    if (ctx.selected >= filtered.list.items.len) ctx.selected = 0;
    if (ctx.selected < 0) ctx.selected = @intCast(filtered.list.items.len - 1);

    ctx.selected_entry.destroy();
    ctx.selected_entry = try filtered.get(@intCast(ctx.selected)).dup();

    const window_size: i32 = @as(i32, @intCast(ctx.rows)) - ctx.margin_top - 2;
    const window_n: i32 = @divTrunc(ctx.selected, window_size);

    position[0] = ctx.margin_left + ctx.margin_entry;
    for (0.., filtered.list.items) |i, e| {
        // todo: improve this, there are a lot of wasted iterations
        if (i < window_n * window_size) continue;
        if (i >= (window_n + 1) * window_size) continue;
        position[1] += 1;

        if (i == ctx.selected) {
            _ = self_window.window_puts(position[0], position[1], ctx.selected_text_color, ctx.bg_color, @constCast(e.real_name.?));
        } else if (e.real_name) |name| {
            _ = self_window.window_puts(position[0], position[1], ctx.text_color, ctx.bg_color, @constCast(name));
        }
    }
}

fn _init() !void {
    ctx = .{
        .allocator = std.heap.c_allocator,
    };

    parse_desktop_dir(&ctx.entry_list, "/usr/share/applications/") catch {};
    parse_desktop_dir(&ctx.entry_list, "/home/hugo/.local/share/applications/") catch {};

    const entries = ctx.entry_list.list.items.len;
    if (entries <= 0) {
        info("No .desktop files\n", .{});
        _ = self_window.window_puts(0, 0, eqnx.RED, ctx.bg_color, @constCast("No .desktop files"));
        return;
    } else {
        info("Found {} .desktop files\n", .{entries});
    }
}

fn _fini() !void {
    ctx.entry_list.destroy();
    ctx.selected_entry.destroy();
    ctx.input.deinit(ctx.allocator);
}

fn select_and_run() void {
    const e: Entry = ctx.selected_entry;
    info("Selected: {s}\n", .{e.efec_name.?});
    info("     run: {s}\n", .{e.exec.?});

    const env_block: std.process.Environ.Block = .{
        .slice = environ[0..std.mem.len(environ) :null],
    };
    var threaded = std.Io.Threaded.init(ctx.allocator, .{
        .environ = .{ .block = env_block },
    });
    const io = threaded.io();
    _ = std.process.spawn(io, .{
        .argv = &.{ "sh", "-c", e.exec.? },
    }) catch |err| {
        std.debug.print("Error: {}\n", .{err});
    };

    std.process.exit(0);
}

// Main function - entry point.
pub export fn main(argc: usize, argv: [*c][*c]u8) callconv(.c) i32 {
    for (0..argc) |i| {
        std.debug.print("arg[{}] = {s}\n", .{ i, argv[i] });
    }

    _init() catch |e| {
        std.debug.print("init error: {}\n", .{e});
        _fini() catch {};
        return 1;
    };

    eqnx.mainloop();

    _fini() catch {};
    return 0;
}
