project('nestruts', 'cpp',
    version : '0.1',
    default_options : ['warning_level=3',  'cpp_std=c++20'])

catch2_proj = subproject('catch2')
catch2_dep = dependency('catch2-with-main')

fmt_proj = subproject('fmt')
fmt_dep = dependency('fmt')
sdl2 = meson.get_compiler('cpp').find_library('SDL2')

lib = library(
    'struts_lib',
    [
        'nestruts/apu.cpp',
        'nestruts/audio.cpp',
        'nestruts/core6502.cpp',
        'nestruts/gfx.cpp',
        'nestruts/instruction_store.cpp',
        'nestruts/mem.cpp',
        'nestruts/ppu.cpp',
        'nestruts/rom.cpp',
    ],
    include_directories : [
        'nestruts',
    ],
    dependencies : [
        fmt_dep,
        sdl2,
    ],
)

lib_dep = declare_dependency(
    include_directories : [
        'nestruts'
    ],
    dependencies : [
        fmt_dep,
        sdl2,
    ],
    link_with : lib,
)

main = executable('nestruts',
    [
        'nestruts/nestruts.cpp',
    ],
    dependencies : [
        lib_dep,
    ],
    )


struts_test = executable('struts_test', ['nestruts/test/cpu.cpp',],
    dependencies : [
        catch2_dep,
        lib_dep,
    ],
)

test('basic', struts_test)
