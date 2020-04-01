inc = include_directories('.', '../..')

executable('${edi_name}', [
  '${edi_name}_main.c',
  '${edi_name}_private.h',
  'gettext.h'
  ],
  include_directories: inc,
  dependencies: [ deps, deps_os ],
  gui_app : true,
  install : true)
