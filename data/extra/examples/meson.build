tar = find_program('tar')
edje_cc = find_program('edje_cc')

cmd = [ edje_cc,
        '-id', join_paths(meson.source_root(), 'data' , 'extra', 'examples', 'images'),
        '@INPUT@', '@OUTPUT@'
      ]

examples_dir = join_paths(get_option('datadir'), 'edi', 'examples')
custom_target('examples_meta',
    input   : 'examples.edc',
    output  : 'examples.edj',
    command : cmd,
    install_dir: examples_dir,
    install: true,
)

out = join_paths(get_option('prefix'), examples_dir, 'examples.edj')
meson.add_install_script('../../../scripts/world_read.sh', out)

