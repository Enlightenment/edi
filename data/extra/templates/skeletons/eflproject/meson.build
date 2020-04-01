##### Project
project('asdas', 'c',
	version      : '0.0.1',
	meson_version : '>= 0.40.0')

efl_version = '>= 1.22.0'

cfg = configuration_data()

host_os = host_machine.system()

deps_os = declare_dependency(link_args : [] )
if host_os == 'freebsd'
  deps_os = declare_dependency(link_args : ['-I/usr/local/include', '-lintl'])
endif

deps = dependency('elementary', version: efl_version)

##### dir locations
dir_prefix = get_option('prefix')
dir_bin    = join_paths(dir_prefix, get_option('bindir'))
dir_lib    = join_paths(dir_prefix, get_option('libdir'))
dir_data   = join_paths(dir_prefix, get_option('datadir'))
dir_locale = join_paths(dir_prefix, get_option('localedir'))

subdir('data')
subdir('po')
subdir('src')

##### config.h
cfg.set('HAVE_CONFIG_H', 1)
cfg.set_quoted('PACKAGE', 'asdas')
cfg.set_quoted('PACKAGE_VERSION', meson.project_version())
cfg.set_quoted('PACKAGE_DATA_DIR', join_paths(dir_data, 'asdas'))
configure_file(output: 'config.h', configuration: cfg)

