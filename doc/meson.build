sphinx_sources = [
        'conf.py',
        'index.rst']

man_pages = [
        'network-config-manager.8']

mandir8 = join_paths(get_option('mandir'), 'man8')

sphinx_build = find_program('sphinx-build-3', 'sphinx-build')

custom_target(
             'man',
              command : [sphinx_build,
                       '-b', 'man',
                        meson.current_source_dir(),
                        meson.current_build_dir()],
              input : sphinx_sources,
              output : man_pages,
              install : true,
              install_dir : mandir8)
