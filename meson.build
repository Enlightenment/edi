project(
  'edi', 'c',
  version : '0.0.1',
  default_options: [ 'c_std=gnu99', 'warning_level=2' ],
  meson_version : '>= 0.40.0')

add_global_arguments('-DHAVE_CONFIG_H=1', '-DHAVE_CONFIG=1', language: 'c')

config_h = configuration_data()
config_h.set_quoted('PACKAGE'          , meson.project_name())
config_h.set_quoted('PACKAGE_VERSION'  , meson.project_version())
config_h.set_quoted('PACKAGE_URL'      , 'https://www.enlightenment.org')
config_h.set_quoted('PACKAGE_TARNAME'  , meson.project_name())
config_h.set_quoted('PACKAGE_BUGREPORT', 'enlightenment-devel@lists.sourceforge.net')
config_h.set_quoted('PACKAGE_STRING'   , meson.project_name() + ' ' + meson.project_version())
config_h.set_quoted('PACKAGE_NAME'     , meson.project_name())
config_h.set_quoted('PACKAGE_BIN_DIR', join_paths(get_option('prefix'), get_option('bindir')))
config_h.set_quoted('PACKAGE_LIB_DIR', join_paths(get_option('prefix'), get_option('libdir')))
config_h.set_quoted('PACKAGE_DATA_DIR', join_paths(get_option('prefix'), get_option('datadir'), 'edi'))
config_h.set_quoted('PACKAGE_DOC_DIR', join_paths(get_option('prefix'), get_option('infodir'), 'edi'))

config_h.set_quoted('EFL_BETA_API_SUPPORT'     , '1')

elm = dependency('elementary')
top_inc = include_directories('.')

cc = meson.get_compiler('c')



config_h.set_quoted('EFL_CFLAGS', run_command(find_program('pkg-config'), '--libs', '--cflags', 'elementary').stdout().strip())

if get_option('bear') == true
  bear = find_program('bear')
  config_h.set_quoted('BEAR_COMMAND', 'bear')
else
  config_h.set_quoted('BEAR_COMMAND', '')
endif

if get_option('libclang') == true
  clang = cc.find_library('clang')

  clang_include_dir_command = run_command(find_program('scripts/clang_include_dir.sh'))
  clang_include_dir = clang_include_dir_command.stdout().strip()

  config_h.set_quoted('CLANG_INCLUDES', clang_include_dir)
  config_h.set('HAVE_LIBCLANG', '1')
endif

subdir('po')
subdir('src')
subdir('doc')
subdir('data')

install_data(['AUTHORS'],
  install_dir :  join_paths(get_option('prefix'), get_option('infodir'), 'edi')
)

configure_file(
  output : 'config.h',
  install : false,
  configuration: config_h
)
