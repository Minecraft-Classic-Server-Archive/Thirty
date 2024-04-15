project('classicserver', 'c',
    default_options: [ 'warning_level=3' ]
)

source_files = [
    'src/buffer.c',
    'src/buffer.h',
    'src/client.c',
    'src/client.h',
    'src/main.c',
    'src/map.c',
    'src/map.h',
    'src/mapsend.c',
    'src/server.c',
    'src/server.h'
]

deps = [
    dependency('threads', required: true),
    dependency('zlib', required: true)
]

executable('classicserver', source_files, dependencies: deps)
