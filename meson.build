project('network-config-manager', 'c',
        version : '0.7.5',
        license : 'Apache-2.0',
        default_options: [
                'c_std=gnu11',
                'prefix=/usr',
                'sysconfdir=/etc',
                'localstatedir=/var',
                'warning_level=2',
        ],
        meson_version : '>= 0.40')

so_version = '1'

conf = configuration_data()
conf.set_quoted('PROJECT_URL', 'https://github.com/vmware/network-config-manager')
conf.set('PROJECT_VERSION', meson.project_version().split('~')[0],
         description : 'Numerical project version (used where a simple number is expected)')
conf.set_quoted('PROJECT_VERSION_FULL', meson.project_version(), description : 'Full project version')

conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_STRING',  meson.project_name() + ' ' + meson.project_version())

substs = configuration_data()

substs.set('PACKAGE_URL',          'https://github.com/vmware/network-config-manager')
conf.set_quoted('PKGSYSCONFDIR',   get_option('sysconfdir'))

#####################################################################

cc = meson.get_compiler('c')

c_args = '''
        -Wextra
        -Werror=undef
        -Werror=format=2
        -Wformat-security
        -Wformat-nonliteral
        -Wlogical-op
        -Wmissing-include-dirs
        -Werror=old-style-definition
        -Werror=pointer-arith
        -Winit-self
        -Wdeclaration-after-statement
        -Wfloat-equal
        -Wsuggest-attribute=noreturn
        -Werror=missing-prototypes
        -Werror=implicit-function-declaration
        -Werror=missing-declarations
        -Werror=return-type
        -Werror=incompatible-pointer-types
        -Werror=shadow
        -Wstrict-prototypes
        -Wredundant-decls
        -Wmissing-noreturn
        -Wendif-labels
        -Wstrict-aliasing=2
        -Wwrite-strings
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-unused-result
        -Werror=overflow
        -Werror=sign-compare
        -Wdate-time
        -Wnested-externs
        -ffast-math
        -fno-common
        -fdiagnostics-show-option
        -fno-strict-aliasing
        -fvisibility=hidden
        -fstack-protector
        -fstack-protector-strong
        -fPIE
        --param=ssp-buffer-size=4
'''.split()

foreach arg : c_args
        if cc.has_argument(arg)
                add_project_arguments(arg, language : 'c')
        endif
endforeach

conf.set('_GNU_SOURCE', true)
conf.set('__SANE_USERSPACE_TYPES__', true)

conf.set('SIZEOF_PID_T', cc.sizeof('pid_t', prefix : '#include <sys/types.h>'))
conf.set('SIZEOF_UID_T', cc.sizeof('uid_t', prefix : '#include <sys/types.h>'))
conf.set('SIZEOF_GID_T', cc.sizeof('gid_t', prefix : '#include <sys/types.h>'))
conf.set('SIZEOF_DEV_T', cc.sizeof('dev_t', prefix : '#include <sys/types.h>'))
conf.set('SIZEOF_INO_T', cc.sizeof('ino_t', prefix : '#include <sys/types.h>'))
conf.set('SIZEOF_TIME_T', cc.sizeof('time_t', prefix : '#include <sys/time.h>'))
conf.set('SIZEOF_RLIM_T', cc.sizeof('rlim_t', prefix : '#include <sys/resource.h>'))

if cc.has_function('getrandom', prefix : '''#include <sys/random.h>''')
        conf.set10('USE_SYS_RANDOM_H', true)
        conf.set10('HAVE_GETRANDOM', true)
else
        have = cc.has_function('getrandom', prefix : '''#include <linux/random.h>''')
        conf.set10('USE_SYS_RANDOM_H', false)
        conf.set10('HAVE_GETRANDOM', have)
endif

############################################################
want_nftables = get_option('nftables')
if want_nftables
   conf.set10('HAVE_NFTABLES', get_option('nftables'))
else
   conf.set10('HAVE_NFTABLES', false)
endif

##############################################################
want_docs = get_option('docs')
if want_docs
   subdir('doc')
endif

##############################################################
config_h = configure_file(
        output : 'config.h',
        configuration : conf)
add_project_arguments('-include', 'config.h', language : 'c')

subdir('include')
subdir('src')
subdir('units')
subdir('conf')

############################################################

libsystemd = dependency('libsystemd',
                     version : '>= 245')

glib_dep = dependency('glib-2.0')
gobj_dep = dependency('gobject-2.0')
gio_dep = dependency('gio-2.0')
yaml_dep = dependency('yaml-0.1')
libmnl_dep = dependency('libmnl')
libjson_dep = dependency('json-c')

if want_nftables
   libnftnl_dep = dependency('libnftnl')
   libnft_dep = dependency('libnftables')
endif

includes = include_directories('src/ctl',
                               'src/dbus',
                               'src/dns',
                               'src/dracut',
                               'src/json',
                               'src/lib-network/netlink',
                               'src/lib-network/networkd',
                               'src/manager',
                               'src/nftables',
                               'src/share',
                               'src/udev',
                               'src/yaml',
                               'src/lib-network/mnl',
                               'include',
                               '.')

if want_nftables
   network_config_managerlib = library('network_config_manager',
                        network_config_manager_sources,
                        include_directories: includes,
                        dependencies : [
                        libsystemd, glib_dep, gio_dep, gobj_dep, yaml_dep, libmnl_dep, libjson_dep, libnftnl_dep, libnft_dep],
                        install: true,
                        version: meson.project_version(),
                        soversion: so_version)
else
  network_config_managerlib = library('network_config_manager',
                        network_config_manager_sources,
                        include_directories: includes,
                        dependencies : [
                        libsystemd, glib_dep, gio_dep, gobj_dep, yaml_dep, libmnl_dep, libjson_dep],
                        install: true,
                        version: meson.project_version(),
                        soversion: so_version)
endif

if want_nftables
   network_config_manager = executable(
                              'nmctl',
                              [network_config_manager_sources, 'src/manager/network-manager-ctl.c'],
                              include_directories : includes,
                              dependencies : [
                              libsystemd, glib_dep, gio_dep, gobj_dep, yaml_dep, libmnl_dep, libnftnl_dep, libnft_dep, libjson_dep],
                              install : true)
else
    network_config_manager = executable(
                              'nmctl',
                              [network_config_manager_sources, 'src/manager/network-manager-ctl.c'],
                              include_directories : includes,
                              dependencies : [
                              libsystemd, glib_dep, gio_dep, gobj_dep, yaml_dep, libmnl_dep, libjson_dep],
                              install : true)
endif

pkg = import('pkgconfig')
pkg.generate(libraries : network_config_managerlib,
              version : meson.project_version(),
              name : 'libnetwork_config_manager',
              filebase : 'network-config-manager',
              description : 'A library for setting the network configuration manager files.')

want_tests = get_option('tests')

if want_tests
cmocka_dep = dependency('cmocka')

network_config_manager_tests = executable(
                              'nmctl-tests',
                              [network_config_manager_sources,
                              'tests/cmocka/basic.c',
                              'tests/cmocka/shared.c',
                              'tests/cmocka/shared.h',
                              'tests/cmocka/set-network.c',
                              'tests/cmocka/set-network.h',
                              'tests/cmocka/vami-set-network-port.c',
                              ],
                              include_directories : includes,
                              dependencies : [
                              libsystemd, cmocka_dep, glib_dep, gio_dep, gobj_dep, yaml_dep, libmnl_dep, libnftnl_dep, libnft_dep, libjson_dep],
                              install : true)
endif
