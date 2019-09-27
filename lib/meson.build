if not host_machine.system().startswith('windows')
   libfuse_sources = ['fuse.c', 'fuse_i.h', 'fuse_loop.c', 'fuse_loop_mt.c',
                     'fuse_lowlevel.c', 'fuse_misc.h', 'fuse_opt.c',
                     'fuse_signals.c', 'buffer.c', 'cuse_lowlevel.c',
                     'helper.c', 'modules/subdir.c', 'mount_util.c',
                     'fuse_log.c' ]
else
   libfuse_sources = ['fuse.c', 'fuse_i.h', 'fuse_loop.c', 'fuse_loop_mt.c',
                     'fuse_lowlevel.c', 'fuse_misc.h', 'fuse_opt.c',
                     'fuse_signals.c', 'buffer.c',
                     'helper.c', 'modules/subdir.c',
                     'fuse_log.c',
                     'windows/libfuse_windows_internal.c', 'windows/winfuse.c' ]
endif

if host_machine.system().startswith('linux')
   libfuse_sources += [ 'mount.c' ]
elif not host_machine.system().startswith('windows')
   libfuse_sources += [ 'mount_bsd.c' ]
endif

deps = [ thread_dep ]
if cfg.get('HAVE_ICONV')
   libfuse_sources += [ 'modules/iconv.c' ]
   libiconv = cc.find_library('iconv', required: false)
   if libiconv.found()
      deps += [ libiconv ]
   endif
endif

libdl = cc.find_library('dl', required: false)
if libdl.found()
   deps += [ libdl ]
endif

if host_machine.system().startswith('netbsd')
   deps += [ cc.find_library('perfuse'),
             cc.find_library('puffs') ]
elif not host_machine.system().startswith('windows')
   # Required for clock_gettime before glibc 2.17
   deps += cc.find_library('rt')
endif

fusermount_path = join_paths(get_option('prefix'), get_option('bindir'))
if not host_machine.system().startswith('windows')
   libfuse = library('fuse3', libfuse_sources, version: meson.project_version(),
                     soversion: '3', include_directories: include_dirs,
                     dependencies: deps, install: true,
                     link_depends: 'fuse_versionscript',
                     c_args: [ '-DFUSE_USE_VERSION=34',
                              '-DFUSERMOUNT_DIR="@0@"'.format(fusermount_path) ],
                     link_args: ['-Wl,--version-script,' + meson.current_source_dir()
                                 + '/fuse_versionscript' ])
else
   libfuse = library('fuse3', libfuse_sources, version: meson.project_version(),
                     soversion: '3', include_directories: include_dirs,
                     dependencies: deps, install: true,
                     link_depends: 'fuse_versionscript',
                     c_args: [ '-DFUSE_USE_VERSION=34' ],
                     vs_module_defs: 'windows/fuse.def')
endif

pkg = import('pkgconfig')
pkg.generate(libraries: [ libfuse, '-lpthread' ],
             libraries_private: '-ldl',
             version: meson.project_version(),
             name: 'fuse3',
             description: 'Filesystem in Userspace',
             subdirs: 'fuse3')

libfuse_dep = declare_dependency(include_directories: include_dirs,
                                 link_with: libfuse, dependencies: deps)
