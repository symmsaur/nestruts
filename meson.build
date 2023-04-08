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
        'nestruts/audio_processing_unit.cpp',
        'nestruts/core6502.cpp',
        'nestruts/gfx.cpp',
        'nestruts/mem.cpp',
        'nestruts/ppu.cpp',
    ],
    dependencies : [
        fmt_dep,
        sdl2,
    ],
)

main = executable('nestruts',
    [
        'nestruts/nestruts.cpp',
    ],
    dependencies : [
        fmt_dep,
        sdl2,
    ],
    link_with : lib,
    )


struts_test = executable('struts_test', ['nestruts/test/cpu.cpp',],
    dependencies : [
        catch2_dep,
        fmt_dep,
    ],
    link_with : lib,
)

test('basic', struts_test)
