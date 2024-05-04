# Thirty, a ClassiCube (Minecraft Classic) server
# Copyright (C) 2024 Sean Baggaley
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

project('thirty', 'c',
    default_options: [
        'warning_level=3',
        'c_std=c99'
    ]
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
    'src/config.c',
    'src/config.h',
    'src/cpe.c',
    'src/heartbeat.c',
    'src/cpe.h',
    'src/main.c',
    'src/map.c',
    'src/map.h',
    'src/mapgen.c',
    'src/mapgen_classic.c',
    'src/mapgen_debug.c',
    'src/mapgen_flat.c',
    'src/mapgen_seantest.c',
    'src/mapsave.c',
    'src/mapsend.c',
    'src/namelist.c',
    'src/namelist.h',
    'src/nbt.c',
    'src/nbt.h',
    'src/perlin.c',
    'src/perlin.h',
    'src/rng.c',
    'src/rng.h',
    'src/server.c',
    'src/server.h',
    'src/util.c',
    'src/util.h',

    'lib/b64.c',
    'lib/b64.h',
    'lib/md5.c',
    'lib/md5.h',
    'lib/sha1.c',
    'lib/sha1.h',
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

libdir = include_directories('lib')

executable('thirty', source_files, dependencies: deps, include_directories: libdir, install: true)
install_data(sources: 'settings.ini', rename: 'thirty.ini', install_dir: 'etc')
