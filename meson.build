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
    version: '0.1.0',
    default_options: [ 'c_std=gnu11', 'warning_level=3' ]
)

cc = meson.get_compiler('c')
ccflags = ['-ftree-vectorize']

version_h = configure_file(input: 'version.h.in', output: 'version.h', configuration: {
    'project_version': meson.project_version()
})

hg_identify = vcs_tag(command: ['hg', 'identify', '-T', '{ifeq(latesttag, "null", "", latesttag)}{ifeq(latesttagdistance, 0, "", "+{latesttagdistance}.{node|short}")}{if(topic, "-{topic}")}', '-r', '.'], input: 'hginfo.h.in', output: 'hginfo.h', replace_string: '@hg_changeset@')

source_files = [
    'src/blocks.c',
    'src/blocks.h',
    'src/buffer.c',
    'src/buffer.h',
    'src/client.c',
    'src/client.h',
    'src/commands.c',
    'src/commands.h',
    'src/config.c',
    'src/config.h',
    'src/cpe.c',
    'src/heartbeat.c',
    'src/log.c',
    'src/log.h',
    'src/cpe.h',
    'src/main.c',
    'src/map.c',
    'src/map.h',
    'src/mapgen.c',
    'src/mapgen_classic.c',
    'src/mapgen_debug.c',
    'src/mapgen_flat.c',
    'src/mapgen_seantest.c',
    'src/mapimage.c',
    'src/mapimage.h',
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
    'lib/stb_image_write.c',
    'lib/stb_image_write.h',

    version_h, hg_identify
]

readline = dependency('readline', required: get_option('use_readline'))

deps = [
    dependency('threads', required: true),
    dependency('zlib', required: true),
    dependency('libcurl', required: true),
    dependency('openmp'),
    readline,
]

if readline.found()
    add_project_arguments('-DUSE_READLINE', language: 'c')
endif

if build_machine.system() == 'windows'
    deps += cc.find_library('bcrypt')
    deps += cc.find_library('ws2_32')
    add_project_arguments('-D_USE_MATH_DEFINES', language: 'c')
else
    deps += cc.find_library('m')
endif

if cc.has_function('poll', prefix: '#include <poll.h>')
    add_project_arguments('-DUSE_POLL', language: 'c')
endif

if cc.has_function('uname', prefix: '#include <sys/utsname.h>')
    add_project_arguments('-DUSE_UNAME', language: 'c')
endif

libdir = include_directories('lib')

executable('thirty', source_files, dependencies: deps, include_directories: libdir, c_args: ccflags, install: true)
install_data(sources: 'settings.ini', rename: 'thirty.ini', install_dir: '/etc')
