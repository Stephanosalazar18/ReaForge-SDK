cc = meson.get_compiler('cpp')
cc_c = meson.get_compiler('c')

lua_inc = include_directories('/home/stephano/.local/include')
lua_lib = cc.find_library('lua', dirs : '/home/stephano/.local/lib')
lua_dep = declare_dependency(include_directories : lua_inc, dependencies : lua_lib)

quickjs_root = '..' / '..' / 'third_party' / 'quickjs-ng'
quickjs_lib = static_library('quickjs',
    quickjs_root / 'quickjs.c',
    quickjs_root / 'quickjs-libc.c',
    quickjs_root / 'libregexp.c',
    quickjs_root / 'libunicode.c',
    quickjs_root / 'dtoa.c',
    include_directories : include_directories(quickjs_root),
    c_args : ['-w', '-fno-strict-aliasing', '-D_GNU_SOURCE', '-DCONFIG_LINUX'],
    gnu_symbol_visibility : 'hidden',
    install : false)

reaper_sdk_root = '..' / '..' / 'third_party' / 'reaper-sdk' / 'sdk'

bridge_sources = [
    'bridge/bridge.cpp',
    'bridge/bridge_lua.cpp',
    'bridge/bridge_quickjs.cpp',
    'bridge/bridge_jsfx.cpp',
]

runtime_sources = [
    'runtime/lua_runtime.cpp',
    'runtime/quickjs_runtime.cpp',
    'runtime/jsfx_runtime.cpp',
]

common_inc = include_directories('.', quickjs_root)

have_reaper_sdk = run_command('test', '-f', reaper_sdk_root / 'reaper_plugin.h', check : false).returncode() == 0
message('REAPER SDK headers: ' + (have_reaper_sdk ? 'found' : 'not found at ' + reaper_sdk_root))

reaper_core = shared_library('reaper_reaforge_host',
    'host.cpp',
    'panel.cpp',
    'context_menu.cpp',
    'extension_loader.cpp',
    'executor.cpp',
    runtime_sources,
    bridge_sources,
    include_directories : common_inc,
    dependencies : [lua_dep],
    link_with : quickjs_lib,
    gnu_symbol_visibility : 'default',
    install : false)

bench = executable('reaforge_bench',
    'bench.cpp',
    'executor.cpp',
    runtime_sources,
    bridge_sources,
    include_directories : common_inc,
    dependencies : [lua_dep],
    link_with : quickjs_lib,
    install : false)

host_main = executable('reaforge_host_main',
    'main.cpp',
    'executor.cpp',
    runtime_sources,
    bridge_sources,
    include_directories : common_inc,
    dependencies : [lua_dep],
    link_with : quickjs_lib,
    install : false)

test('init_smoke', host_main, suite : 'host')

test_loader = executable('test_loader',
    'extension_loader.cpp',
    'tests/test_loader.cpp',
    include_directories : common_inc,
    dependencies : [lua_dep],
    install : false)

test('loader', test_loader, suite : 'host')
