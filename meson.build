project('classicserver', 'c',
    default_options: [ 'warning_level=3' ]
)

cc = meson.get_compiler('c')
ccflags = []

source_files = [
    'src/blocks.c',
    'src/blocks.h',
    'src/buffer.c',
    'src/buffer.h',
    'src/client.c',
    'src/client.h',
    'src/cpe.c',
    'src/cpe.h',
    'src/main.c',
    'src/map.c',
    'src/map.h',
    'src/mapgen.c',
    'src/mapgen_classic.c',
    'src/mapgen_debug.c',
    'src/mapgen_flat.c',
    'src/mapgen_seantest.c',
    'src/mapsend.c',
    'src/perlin.c',
    'src/perlin.h',
    'src/rng.c',
    'src/rng.h',
    'src/server.c',
    'src/server.h'
]

deps = [
    dependency('threads', required: true),
    dependency('zlib', required: true)
]

if build_machine.system() == 'windows'
    deps += cc.find_library('ws2_32')
else
    deps += cc.find_library('m')
endif

if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
    ccflags += ['-ftree-vectorize']
endif


executable('classicserver', source_files, dependencies: deps, c_args: ccflags)
