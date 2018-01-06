tar = find_program('tar')
edje_cc = find_program('edje_cc')

skeleton_names = ['eflproject', 'eflproject_python']

foreach skeleton_name : skeleton_names

  custom_target('skeleton ' + skeleton_name,
     command : [tar, 'zcf', '@OUTPUT@', '-C', '../data/extra/templates/skeletons', skeleton_name],
     input : 'skeletons' + '/' + skeleton_name,
     output : skeleton_name + '.tar.gz',
     install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'edi', 'templates'),
     install : true,
  )

endforeach

cmd = [ edje_cc,
        '-id', join_paths(meson.source_root(), 'data' , 'extra', 'templates', 'images'),
        '@INPUT@', '@OUTPUT@'
      ]

template_dir = join_paths(get_option('datadir'), 'edi', 'templates')
custom_target('templates_meta',
    input   : 'templates.edc',
    output  : 'templates.edj',
    command : cmd,
    install_dir: template_dir,
    install: true,
)
out = join_paths(get_option('prefix'), template_dir, 'templates.edj')
meson.add_install_script('../../../scripts/world_read.sh', out)

