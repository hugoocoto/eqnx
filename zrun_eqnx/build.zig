const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const libzrun = b.addLibrary(.{
        .name = "zrun",
        .linkage = .dynamic,
        .version = .{ .major = 0, .minor = 0, .patch = 0 },
        .root_module = b.createModule(.{
            .root_source_file = b.path("zrun.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    libzrun.root_module.addIncludePath(b.path("../src/")); // plug_api.h
    libzrun.root_module.addIncludePath(b.path("../plugins_src/")); // theme.h
    libzrun.root_module.addSystemIncludePath(.{ .cwd_relative = "/usr/include/" }); // idk why this is not the default
    libzrun.root_module.link_libc = true;

    b.installArtifact(libzrun);
}
