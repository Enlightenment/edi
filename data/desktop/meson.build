desktop_config = configuration_data()
desktop_config.set('VERSION', meson.project_version())
desktop_config.set('PACKAGE_NAME', meson.project_name())

configure_file(
  input : 'edi.desktop.in',
  output : 'edi.desktop',
  install_dir : join_paths(get_option('prefix'), 'share/applications/'),
  configuration : desktop_config
)

install_data(['edi.png'],
             install_dir: join_paths(get_option('prefix'), 'share/icons/hicolor/256x256/apps')
)
