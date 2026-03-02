project('hyprsic', ['c', 'cpp'],
  version: '0.1.0',
  meson_version: '>= 0.60.0',
  default_options: [
    'cpp_std=c++20',
    'c_std=c11',
    'warning_level=2'
  ]
)

pkg = import('pkgconfig')

deps = [
  dependency('wayland-client'),
  dependency('gtk+-3.0'),
  dependency('gtk-layer-shell-0'),
  dependency('jsoncpp'),
  dependency('dbus-1'),
  dependency('libpulse'),
  dependency('sqlite3'), # Required by SQLiteCpp
]


sqlitecpp_proj = subproject('SQLiteCpp')
sqlitecpp_dep = sqlitecpp_proj.get_variable('sqlitecpp_dep')

cli11_proj = subproject('CLI11')
cli11_dep = cli11_proj.get_variable('cli11_dep')

deps += [sqlitecpp_dep, cli11_dep]


wayland_proto_sources = files(
  'src/wayland/ext-data-control-v1.c'
)

module_sources = files(
  'src/modules/workspaces/hyprland/manager.cpp',
  'src/modules/workspaces/hyprland/module.cpp',
  'src/modules/sysinfo/manager/battery.cpp',
  'src/modules/sysinfo/manager/disk.cpp',
  'src/modules/sysinfo/manager/memory.cpp',
  'src/modules/sysinfo/manager/network.cpp',
  'src/modules/sysinfo/manager/sys_load.cpp',
  'src/modules/sysinfo/manager/stats.cpp',
  'src/modules/sysinfo/module.cpp',
  'src/modules/pulseaudio/manager.cpp',
  'src/modules/pulseaudio/window.cpp',
  'src/modules/pulseaudio/module.cpp',
  'src/modules/mpris/manager.cpp',
  'src/modules/mpris/module.cpp',
  'src/modules/mpris/window.cpp',
  'src/modules/bluetooth/manager.cpp',
  'src/modules/bluetooth/module.cpp',
  'src/modules/bluetooth/window.cpp',
  'src/modules/notifications/manager.cpp',
  'src/modules/notifications/module.cpp',
  'src/modules/notifications/window.cpp',
  'src/modules/screensaver/manager.cpp',
  'src/modules/screensaver/module.cpp',
  'src/modules/statusnotifier/manager.cpp',
  'src/modules/statusnotifier/module.cpp',
  'src/modules/wifi/manager.cpp',
  'src/modules/clipboard/manager.cpp',
  'src/modules/brightness/manager.cpp',
  'src/modules/brightness/module.cpp',
  'src/modules/brightness/window.cpp'
)

database_sources = files('src/database/db_manager.cpp')

logging_sources = files('src/logging/manager.cpp')

util_sources = files(
  'src/utils/dbus_utils.cpp',
  'src/utils/helper_func.cpp'
)

debug_sources = files('src/debug/console_preview.cpp')

app_sources = files(
  'src/app/context.cpp',
  'src/app/window.cpp',
  'src/app/cmdline.cpp'
)

all_sources = [
  'src/app/main.cpp',
  'src/resources/store.cpp',
  wayland_proto_sources,
  module_sources,
  database_sources,
  logging_sources,
  util_sources,
  debug_sources,
  app_sources
]

inc = include_directories('src')

executable('hyprsic',
  all_sources,
  dependencies: deps,
  include_directories: inc,
  install: true
)