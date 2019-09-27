project('libfuse3', ['cpp', 'c'], version: '3.6.2',
        meson_version: '>= 0.42',
        default_options: [ 'buildtype=debugoptimized' ])


platform = host_machine.system()
if platform == 'darwin'
  error('libfuse does not support OS-X.\n' +
        'Take a look at http://osxfuse.github.io/ instead')
elif platform == 'cygwin'
  error('libfuse does not support Windows/Cygwin.\n' +
        'Take a look at http://www.secfs.net/winfsp/ instead')       
endif

#
# Feature detection
#
cfg = configuration_data()
cc = meson.get_compiler('c')

# Default includes when checking for presence of functions and
# struct members
include_default = '''
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
'''
args_default = [ '-D_GNU_SOURCE' ]

cfg.set_quoted('PACKAGE_VERSION', meson.project_version())

# Test for presence of some functions
test_funcs = [ 'fork', 'fstatat', 'openat', 'readlinkat', 'pipe2',
               'splice', 'vmsplice', 'posix_fallocate', 'fdatasync',
               'utimensat', 'copy_file_range', 'fallocate' ]
foreach func : test_funcs
    cfg.set('HAVE_' + func.to_upper(),
        cc.has_function(func, prefix: include_default, args: args_default))
endforeach
cfg.set('HAVE_SETXATTR', 
        cc.has_function('setxattr', prefix: '#include <sys/xattr.h>'))
cfg.set('HAVE_ICONV', 
        cc.has_function('iconv', prefix: '#include <iconv.h>'))

# Test if structs have specific member
if platform != 'windows'
    cfg.set('HAVE_STRUCT_STAT_ST_ATIM',
            cc.has_member('struct stat', 'st_atim',
                    prefix: include_default,
                    args: args_default))
    cfg.set('HAVE_STRUCT_STAT_ST_ATIMESPEC',
            cc.has_member('struct stat', 'st_atimespec',
                    prefix: include_default,
                    args: args_default))
else
    cfg.set('HAVE_STRUCT_STAT_ST_ATIM', true)
endif

# Write the test results into config.h (stored in build directory)
configure_file(output: 'config.h',
               configuration : cfg)

#
# Compiler configuration
#
if platform != 'windows'
    add_project_arguments('-D_REENTRANT', '-DHAVE_CONFIG_H', '-Wall', '-Wextra', '-Wno-sign-compare',
                    '-Wstrict-prototypes', '-Wmissing-declarations', '-Wwrite-strings',
                    '-fno-strict-aliasing', language: 'c')
    add_project_arguments('-D_REENTRANT', '-DHAVE_CONFIG_H', '-D_GNU_SOURCE',
                    '-Wall', '-Wextra', '-Wno-sign-compare', '-std=c++11',
                    '-Wmissing-declarations', '-Wwrite-strings',
                    '-fno-strict-aliasing', language: 'cpp')
else
    add_project_arguments('-D_REENTRANT', '-DHAVE_CONFIG_H',
                    '-D_CRT_SECURE_NO_WARNINGS', '-D_CRT_NONSTDC_NO_WARNINGS',
                    '-D_INO_T_DEFINED', '-D_DEV_T_DEFINED', '-D_OFF_T_DEFINED',
                    '-Wall',
                    '-wd4018', '-wd4100', '-wd4127', '-wd4146', '-wd4152', '-wd4200', '-wd4204',
                    '-wd4214', '-wd4221', '-wd4242', '-wd4244', '-wd4255', '-wd4267', '-wd4389',
                    '-wd4457', '-wd4464', '-wd4668', '-wd4701', '-wd4710', '-wd4774', '-wd4820',
                    '-wd5045',
                    language: 'c')
    add_project_arguments('-D_REENTRANT', '-DHAVE_CONFIG_H', '-D_GNU_SOURCE',
                    '-D_CRT_SECURE_NO_WARNINGS', '-D_CRT_NONSTDC_NO_WARNINGS',
                    '-D_INO_T_DEFINED', '-D_DEV_T_DEFINED', '-D_OFF_T_DEFINED',
                    '-Wall',
                    '-wd4018', '-wd4100', '-wd4127', '-wd4146', '-wd4152', '-wd4200', '-wd4204',
                    '-wd4214', '-wd4221', '-wd4242', '-wd4244', '-wd4255', '-wd4267', '-wd4389',
                    '-wd4457', '-wd4464', '-wd4668', '-wd4701', '-wd4710', '-wd4774', '-wd4820',
                    '-wd5045',
                    language: 'cpp')
endif

# Some (stupid) GCC versions warn about unused return values even when they are
# casted to void. This makes -Wunused-result pretty useless, since there is no
# way to suppress the warning when we really *want* to ignore the value.
code = '''
__attribute__((warn_unused_result)) int get_4() {
    return 4;
}
int main(void) {
    (void) get_4();
    return 0;
}'''
if platform != 'windows' and not cc.compiles(code, args: [ '-O0', '-Werror=unused-result' ])
     message('Compiler warns about unused result even when casting to void')
     add_project_arguments('-Wno-unused-result', language: 'c')
endif

# '.' will refer to current build directory, which contains config.h
if platform != 'windows'
    include_dirs = include_directories('include', 'lib', '.')
else
    include_dirs = include_directories('include', 'lib', 'lib/windows', '.')
endif

# Common dependencies
thread_dep = dependency('threads') 

#
# Read build files from sub-directories
#
if platform != 'windows'
  subdirs = [ 'lib', 'include', 'test' ]
else
  subdirs = [ 'lib', 'include' ]
endif
if get_option('utils') and not platform.endswith('bsd') and platform != 'dragonfly' and platform != 'windows'
  subdirs += [ 'util', 'doc' ]
endif

if get_option('examples')
  subdirs += 'example'
endif

foreach n : subdirs
    subdir(n)
endforeach
