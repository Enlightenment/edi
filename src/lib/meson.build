src = files([
  'Edi.h',
  'edi.c',
  'edi_build_provider.c',
  'edi_build_provider.h',
  'edi_build_provider_cargo.c',
  'edi_build_provider_cmake.c',
  'edi_build_provider_make.c',
  'edi_build_provider_meson.c',
  'edi_build_provider_python.c',
  'edi_builder.c',
  'edi_builder.h',
  'edi_create.c',
  'edi_create.h',
  'edi_exe.c',
  'edi_exe.h',
  'edi_path.c',
  'edi_path.h',
  'edi_private.h',
  'edi_scm.c',
  'edi_scm.h',
  'md5.c',
  'md5.h',
])

edi_lib_lib = library('edi', src,
  dependencies : [elm],
  include_directories : top_inc,
  install : true
)

edi_lib = declare_dependency(
   link_with : edi_lib_lib,
   include_directories : [include_directories('./'), top_inc]
)
