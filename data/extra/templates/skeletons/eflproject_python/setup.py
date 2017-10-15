#!/usr/bin/env python
# encoding: utf-8

#  python setup.py check
#  python setup.py build
#  python setup.py run
#  python setup.py install
#  python setup.py clean --all


from distutils.core import setup, Command
from distutils.version import StrictVersion
from efl.utils.setup import build_extra, build_edc, build_fdo, build_i18n, uninstall
from efl import __version__ as efl_version


MIN_EFL = '1.18'
if StrictVersion(efl_version) < MIN_EFL:
    print('Your python-efl version is too old! Found: ' + efl_version)
    print('You need at least version ' + MIN_EFL)
    exit(1)


class run_in_tree(Command):
   description = 'Run the main() from the build folder (without install)'
   user_options = []

   def initialize_options(self):
      pass

   def finalize_options(self):
      pass

   def run(self):
        import sys, platform

        sys.path.insert(0, 'build/lib.%s-%s-%d.%d' % (
            platform.system().lower(), platform.machine(),
            sys.version_info[0], sys.version_info[1]))

        from ${Edi_Name}.main import main
        sys.exit(main())


setup(
    name = '${Edi_Name}',
    version = '0.0.1',
    description = 'Short description',
    long_description = 'A longer description of your project',
    author = '${Edi_User}',
    author_email = '${Edi_Email}',
    url = '${Edi_Url}',
    license = "3-Clause BSD",
    package_dir = {'${Edi_Name}': 'src'},
    packages = ['${Edi_Name}'],
    requires = ['efl'],
    scripts = ['bin/${Edi_Name}'],
    cmdclass = {
        'build': build_extra,
        'build_fdo': build_fdo,
        # 'build_edc': build_edc,
        # 'build_i18n': build_i18n,
        'run': run_in_tree,
        'uninstall': uninstall,
    },
    command_options = {
        'install': {'record': ('setup.py', 'installed_files.txt')}
    },
)

