tar = find_program('tar')

skeleton_names = ['eflproject', 'eflproject_python']

foreach skeleton_name : skeleton_names

  custom_target('skeleton ' + skeleton_name,
     command : [tar, 'zcf', '@OUTPUT@', '@INPUT@'],
     input : skeleton_name,
     output : skeleton_name + '.tar.gz',
     install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'edi', 'skeleton'),
     install : true,
  )

endforeach
