edje_cc = find_program('edje_cc')

cmd = [ edje_cc,
        '-id', join_paths(meson.source_root(), 'data' , 'themes'),
        '@INPUT@', '@OUTPUT@'
      ]

theme_names = ['black', 'white', 'solarized']

foreach theme_name : theme_names
  custom_target('theme ' + theme_name,
     input : theme_name + '.edc',
     output : theme_name + '.edj',
     command: cmd,
     install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'edi', 'themes'),
     install: true,
  )

   out = join_paths(get_option('prefix'), get_option('datadir'), 'edi', 'themes', theme_name + '.edj')
   meson.add_install_script('../../scripts/world_read.sh', out)
endforeach


