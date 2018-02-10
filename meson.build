project(
  'edi', 'c',
  version : '0.7.0',
  default_options: [ 'c_std=gnu99', 'warning_level=2' ],
  meson_version : '>= 0.39.0')

add_global_arguments('-DHAVE_CONFIG_H=1', '-DHAVE_CONFIG=1', language: 'c')

config_h = configuration_data()
config_h.set_quoted('PACKAGE'          , meson.project_name())
config_h.set_quoted('PACKAGE_VERSION'  , meson.project_version())
config_h.set_quoted('PACKAGE_URL'      , 'https://www.enlightenment.org')
config_h.set_quoted('PACKAGE_TARNAME'  , meson.project_name())
config_h.set_quoted('PACKAGE_BUGREPORT', 'enlightenment-devel@lists.sourceforge.net')
config_h.set_quoted('PACKAGE_STRING'   , meson.project_name() + ' ' + meson.project_version())
config_h.set_quoted('PACKAGE_NAME'     , meson.project_name())
config_h.set_quoted('PACKAGE_BUILD_DIR', meson.build_root())
config_h.set_quoted('PACKAGE_BIN_DIR', join_paths(get_option('prefix'), get_option('bindir')))
config_h.set_quoted('PACKAGE_LIB_DIR', join_paths(get_option('prefix'), get_option('libdir')))
config_h.set_quoted('PACKAGE_DATA_DIR', join_paths(get_option('prefix'), get_option('datadir'), 'edi'))
config_h.set_quoted('PACKAGE_DOC_DIR', join_paths(get_option('prefix'), get_option('infodir'), 'edi'))

config_h.set_quoted('EFL_BETA_API_SUPPORT'     , '1')

elm = dependency('elementary', version : '>=1.20.6')
top_inc = include_directories('.')

cc = meson.get_compiler('c')



config_h.set_quoted('EFL_CFLAGS', run_command(find_program('pkg-config'), '--libs', '--cflags', 'elementary').stdout().strip())

intl = cc.find_library('intl', required : false)

bear = find_program('bear', required : false)
if get_option('bear') == true and bear.found()
  config_h.set_quoted('BEAR_COMMAND', 'bear')
else
  config_h.set_quoted('BEAR_COMMAND', '')
endif

opt_clang_header_dir = get_option('libclang-headerdir')
opt_clang_link_dir = get_option('libclang-libdir')

if build_machine.system() == 'darwin'
   if opt_clang_header_dir == ''
     opt_clang_header_dir = '/usr/local/opt/llvm/include'
   endif
   if opt_clang_link_dir == ''
     opt_clang_link_dir = '/usr/local/opt/llvm/lib'
   endif
endif

if get_option('libclang') == true

  #check for the header
  found = false
  if opt_clang_header_dir != ''
    found = cc.has_header('clang-c/Index.h', args : '-I'+opt_clang_header_dir)
  else
    found = cc.has_header('clang-c/Index.h')
  endif

  if found == false
    error('Clang header not found!')
  endif

  clang_inc = include_directories(opt_clang_header_dir)

  #check for the library to link
  if opt_clang_link_dir != ''
    clang = cc.find_library('clang', dirs : [opt_clang_link_dir])
  else
    clang = cc.find_library('clang')
  endif

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
