add_rules("mode.debug", "mode.release")


package("blocks-runtime", function()
    set_urls("https://github.com/mackyle/blocksruntime.git")
    add_versions("1.0.0", "9cc93ae2b58676c23fd02cf0c686fa15b7a3ff81")

--[[
Since there are only two files to build, a makefile didn't seem warranted. A special config.h file has been created to make the build work. Build the libBlocksRuntime.a library by running:

    ./buildlib

The gcc compiler will be used by default, but you can do CC=clang ./buildlib for example to use the clang compiler instead. Note that neither make nor cmake are needed (but ar and ranlib will be used but they can also be changed with the AR and RANLIB environment variables similarly to the way the compiler can be changed).
]]
    on_install(function (package)
        os.vrun("./buildlib")
        os.cp("libBlocksRuntime.a", package:installdir("lib"))
        os.cp("BlocksRuntime/Block.h", package:installdir("include"))
        os.cp("BlocksRuntime/Block_private.h", package:installdir("include"))
    end)
end)

--already built in macos
--fuck you if you try and run this on windows btw :)
if not is_plat "macosx" then
    add_requires("blocks-runtime")
    add_packages("blocks-runtime")
end

set_languages("gnulatest")

if is_mode "debug" then
    set_policy("build.sanitizer.address", true)
    set_policy("build.sanitizer.leak", true)
    set_policy("build.sanitizer.undefined", true)
end

target("text-editor", function()
    set_kind("binary")
    add_files("src/*.c|commands.test.c")
    add_cxflags {
        "-Wall",
        "-Wextra",
        "-Werror",
        
        "-fblocks",
        "-Wanon-enum-enum-conversion",
        "-Wassign-enum",
        "-Wenum-conversion",
        "-Wenum-enum-conversion",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wnull-dereference",
        "-Wnull-conversion",
        "-Wnullability-completeness",
        "-Wnullable-to-nonnull-conversion",
        "-Wno-missing-field-initializers",
    }
end)

target("text-editor-tests", function()
    set_kind("binary")
    add_files("src/*.c|commands.c|main.c") --commands.c is included by commands.test.c
    add_cxflags {
        "-Wall",
        "-Wextra",
        "-Werror",
        
        "-fblocks",
        "-Wanon-enum-enum-conversion",
        "-Wassign-enum",
        "-Wenum-conversion",
        "-Wenum-enum-conversion",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wnull-dereference",
        "-Wnull-conversion",
        "-Wnullability-completeness",
        "-Wnullable-to-nonnull-conversion",
        "-Wno-missing-field-initializers",
    }
end)
