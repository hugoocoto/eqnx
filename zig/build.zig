const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const libtemplate = b.addLibrary(.{
        .name = "template",
        .linkage = .dynamic,
        .version = .{ .major = 0, .minor = 0, .patch = 0 },
        .root_module = b.createModule(.{
            .root_source_file = b.path("template.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    libtemplate.root_module.addIncludePath(b.path("../src/")); // plug_api.h
    libtemplate.root_module.addIncludePath(b.path("../plugins_src/")); // theme.h
    libtemplate.root_module.addSystemIncludePath(.{ .cwd_relative = "/usr/include/" }); // idk why this is not the default

    b.installArtifact(libtemplate);
}
