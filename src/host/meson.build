project('reaforge_host', 'cpp',
    version : '0.0.1',
    license : 'MIT',
    default_options : ['cpp_std=c++17', 'warning_level=2'])

cc = meson.get_compiler('cpp')

lua_dep = dependency('lua5.4', required : false)
if not lua_dep.found()
    lua_dep = dependency('lua-5.4', required : false)
endif
if not lua_dep.found()
    lua_dep = cc.find_library('lua5.4', required : true)
endif

quickjs_root = meson.project_source_root() / '..' / '..' / 'third_party' / 'quickjs-ng'
quickjs_lib = static_library('quickjs',
    quickjs_root / 'quickjs.c',
    quickjs_root / 'quickjs-libc.c',
    quickjs_root / 'libregexp.c',
    quickjs_root / 'libunicode.c',
    quickjs_root / 'dtoa.c',
    include_directories : include_directories(quickjs_root),
    c_args : ['-w', '-fno-strict-aliasing'],
    gnu_symbol_visibility : 'hidden',
    install : false)

reaper_sdk_root = meson.project_source_root() / '..' / '..' / 'third_party' / 'reaper-sdk' / 'sdk'

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

common_inc = include_directories('.')

reaper_plugin_h = reaper_sdk_root / 'reaper_plugin.h'
have_reaper_sdk = reaper_plugin_h.exists()
message('REAPER SDK headers: ' + (have_reaper_sdk ? 'found' : 'not found'))

reaper_core = shared_library('reaper_reaforge_host',
    'host.cpp',
    'panel.cpp',
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
