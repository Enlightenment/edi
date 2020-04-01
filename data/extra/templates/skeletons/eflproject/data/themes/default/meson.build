edje_cc = find_program('edje_cc')

cmd = [ edje_cc,
	'-id', join_paths(meson.source_root(), 'data', 'themes', 'default', 'images'), 
	'-sd', join_paths(meson.source_root(), 'data', 'themes', 'default', 'sounds'),
	'@INPUT@', '@OUTPUT@'
      ]

custom_target('default.edj',
  input : 'default.edc',
  output : 'default.edj',
  command: cmd,
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), '${edi_name}', 'themes'),
  install: true
  )
