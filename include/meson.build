if not host_machine.system().startswith('windows')
	libfuse_headers = [ 'fuse.h', 'fuse_common.h', 'fuse_lowlevel.h', 'fuse_types.h',
			    'fuse_opt.h', 'cuse_lowlevel.h', 'fuse_log.h' ]
else
	libfuse_headers = [ 'fuse.h', 'fuse_common.h', 'fuse_lowlevel.h', 'fuse_types.h',
			    'fuse_opt.h', 'fuse_log.h' ]
endif

install_headers(libfuse_headers, subdir: 'fuse3')
