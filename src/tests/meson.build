src = files([
  'edi_suite.h',
  'edi_suite.c',
  'edi_test_content_provider.c',
  'edi_test_create.c',
  'edi_test_exe.c',
  'edi_test_language_provider.c',
  'edi_test_language_provider_c.c',
  'edi_test_path.c',
])

check = dependency('check')

deps = [elm, check, edi_lib]

if get_option('libclang') == true
   deps += [clang]
   incls += [clang_inc]
endif

exe = executable('edi_suite', src,
  dependencies : deps,
  include_directories : [include_directories('../../../src/bin'), include_directories('../../../src/lib'), top_inc],
  install : false
)
test('Edi Test Suite', exe)

