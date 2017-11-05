packages = ['editor','language','mainview','screens',]

src = files([
  'edi_config.c',
  'edi_config.h',
  'edi_consolepanel.c',
  'edi_consolepanel.h',
  'edi_content_provider.c',
  'edi_content_provider.h',
  'edi_debugpanel.c',
  'edi_debugpanel.h',
  'edi_file.c',
  'edi_file.h',
  'edi_filepanel.c',
  'edi_filepanel.h',
  'edi_logpanel.c',
  'edi_logpanel.h',
  'edi_main.c',
  'edi_private.h',
  'edi_searchpanel.c',
  'edi_searchpanel.h',
])

bin_dir = include_directories('.')

foreach package : packages
   subdir(package)
endforeach

deps = [elm, edi_lib, intl]
incls = []

if get_option('libclang') == true
   deps += [clang]
   incls += [clang_inc]
endif

executable('edi', src,
  dependencies : deps,
  include_directories : incls,
  install : true
)

edi_scm_src = files([
  'edi_scm_main.c',
  'edi_scm_ui.c',
  'edi_scm_ui.h'
])

executable('edi_scm', edi_scm_src,
  dependencies : [elm, edi_lib, intl],
  install : true
)

executable('edi_build', 'edi_build_main.c',
  dependencies : [elm, edi_lib, intl],
  install : true
)
