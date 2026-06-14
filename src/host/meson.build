cc = meson.get_compiler('cpp')
cc_c = meson.get_compiler('c')

# Cross-platform: find python or py for the REAPER SDK header check below.
py = find_program('python', 'python3', 'py', required : false)

# std::thread on Linux needs pthread.
thread_dep = dependency('threads')

# Lua dependency: optional. REAPER provides Lua via its plugin API, so the
# extension can be built without an external Lua install. The Fase 0 in-DLL
# bridges that needed Lua were dead code post-pivot (see Engram #80 and the
# pivot in docs/documentacion/01-vision-and-pivot.md). When the bridges are
# removed (post-archive), this block can be deleted entirely.
lua_dep = dependency('lua', required : false)
if not lua_dep.found()
    lua_dep = declare_dependency()
endif

# QuickJS is Fase 0 dead code (in-DLL JS runtime, rejected by the 2026-06-07 pivot —
# REAPER is the runtime, see docs/documentacion/01-vision-and-pivot.md). The static_library
# target compiles quickjs.c with Linux-specific defines (_GNU_SOURCE, CONFIG_LINUX) and
# GCC-only flags (-fno-strict-aliasing), so it cannot build under MSVC. The C++ greenfield
# modules (http_server, artifact_writer, project_reader, refresh, main_thread_queue) don't
# use quickjs, so we can safely comment this out for the MVP build.
# quickjs_root = '..' / '..' / 'third_party' / 'quickjs-ng'
# quickjs_lib = static_library('quickjs',
#     quickjs_root / 'quickjs.c',
#     quickjs_root / 'quickjs-libc.c',
#     quickjs_root / 'libregexp.c',
#     quickjs_root / 'libunicode.c',
#     quickjs_root / 'dtoa.c',
#     include_directories : include_directories(quickjs_root),
#     c_args : ['-w', '-fno-strict-aliasing', '-D_GNU_SOURCE', '-DCONFIG_LINUX'],
#     gnu_symbol_visibility : 'hidden',
#     install : false)

reaper_sdk_root = '..' / '..' / 'third_party' / 'reaper-sdk' / 'sdk'
reaper_sdk_root_abs = meson.project_source_root() / 'third_party' / 'reaper-sdk' / 'sdk'
sdk_header = reaper_sdk_root_abs / 'reaper_plugin.h'
have_reaper_sdk = run_command('test', '-f', sdk_header, check : false).returncode() == 0
message('REAPER SDK headers: ' + (have_reaper_sdk ? 'found' : 'not found at ' + reaper_sdk_root))

# The Fase 0 reaper_core target depends on host.cpp/panel.cpp/context_menu.cpp/extension_loader.cpp
# which are the Fase 0 plugin lifecycle files. Post-pivot, those are DEAD CODE (see
# docs/documentacion/01-vision-and-pivot.md and the 2026-06-07 pivot). The MVP agentic
# only needs the greenfield HTTP server, project reader, artifact writer, and refresh
# hook. The Fase 0 plugin files were left in place as historical reference but are not
# compiled by this build target. A future PR will rewrite host.cpp for the new vision
# and re-enable this target.
#
# reaper_core = shared_library('reaper_reaforge_host',
#     'host.cpp',
#     'panel.cpp',
#     'context_menu.cpp',
#     'extension_loader.cpp',
#     'executor.cpp',
#     runtime_sources,
#     bridge_sources,
#     include_directories : common_inc,
#     dependencies : [lua_dep, thread_dep],
#     link_with : quickjs_lib,
#     gnu_symbol_visibility : 'default',
#     install : false)
#
# Disabled because Fase 0 dead code requires external Lua + QuickJS + JSFX in-DLL bridges.
# Re-enable once host.cpp is rewritten for the agentic MVP (PR 4-6 follow-up).
message('reaper_core: disabled (Fase 0 dead code, awaits host.cpp rewrite for MVP)')

# Greenfield PR 4 + PR 5 + PR 6 sources — pure C++17, no REAPER.
greenfield_sources = [
    'http_server.cpp',
    'artifact_writer.cpp',
    'project_reader.cpp',
    'refresh.cpp',
    'main_thread_queue.cpp',
]

# common_inc no longer references quickjs_root (Fase 0 dead code). The C++ greenfield
# modules only need the third_party headers (cpp-httplib, nlohmann/json).
common_inc = include_directories('.', '..' / '..' / 'third_party', '..' / '..' / 'third_party' / 'cpp-httplib')

# Windows sockets library (needed by cpp-httplib on Windows targets).
ws2_dep = cc.find_library('ws2_32', required : false)

# ---- MVP shared_library target (replaces the disabled Fase 0 reaper_core) ----
# Uses mvp_host.cpp + the 5 greenfield modules. Only builds when REAPER SDK
# headers are found (third_party/reaper-sdk/sdk/reaper_plugin.h).
# ---- MVP shared_library target (replaces the disabled Fase 0 reaper_core) ----
# Uses mvp_host.cpp + the 5 greenfield modules. Only builds when:
#   - REAPER SDK headers are found (have_reaper_sdk == true)
#   - Host machine is Windows (cross-compile via mingw-w64 or native MSVC)
# On Linux, reaper_plugin.h requires WDL/swell headers which aren't vendored.
if have_reaper_sdk and host_machine.system() == 'windows'
    reaper_inc = include_directories(reaper_sdk_root)
    ws2_dep = cc.find_library('ws2_32', required : false)
    reaper_reaforge_host = shared_library('reaper_reaforge_host',
        'mvp_host.cpp',
        greenfield_sources,
        include_directories : [common_inc, reaper_inc],
        dependencies : [thread_dep, ws2_dep],
        name_prefix : '',
        gnu_symbol_visibility : 'default',
        install : false)
    message('MVP: reaper_reaforge_host.dll target enabled (REAPER SDK found + Windows host)')
else
    message('MVP: reaper_reaforge_host.dll target DISABLED (requires REAPER SDK + Windows host)')
endif

# bench and host_main are also Fase 0 dead code, disabled for the same reason.
# bench = executable('reaforge_bench',
#     'bench.cpp',
#     'executor.cpp',
#     runtime_sources,
#     bridge_sources,
#     include_directories : common_inc,
#     dependencies : [lua_dep],
#     link_with : quickjs_lib,
#     install : false)

# host_main and init_smoke are also Fase 0 dead code, disabled.
# host_main = executable('reaforge_host_main',
#     'main.cpp',
#     'executor.cpp',
#     runtime_sources,
#     bridge_sources,
#     include_directories : common_inc,
#     dependencies : [lua_dep],
#     link_with : quickjs_lib,
#     install : false)
#
# test('init_smoke', host_main, suite : 'host')

# test_loader is Fase 0 dead code (extension_loader.cpp), disabled.
# test_loader = executable('test_loader',
#     'extension_loader.cpp',
#     'tests/test_loader.cpp',
#     include_directories : common_inc,
#     dependencies : [lua_dep],
#     install : false)
#
# test('loader', test_loader, suite : 'host')

# ---- PR 4 greenfield tests (pure C++17, no REAPER) ----

test_main_thread_queue = executable('test_main_thread_queue',
    'main_thread_queue.cpp',
    'tests/test_main_thread_queue.cpp',
    include_directories : common_inc,
    dependencies : [thread_dep, ws2_dep],
    install : false)

test('main_thread_queue', test_main_thread_queue, suite : 'host')

test_artifact_writer = executable('test_artifact_writer',
    greenfield_sources,
    'tests/test_artifact_writer.cpp',
    include_directories : common_inc,
    dependencies : [thread_dep, ws2_dep],
    install : false)

test('artifact_writer', test_artifact_writer, suite : 'host')

test_http_server = executable('test_http_server',
    greenfield_sources,
    'tests/test_http_server.cpp',
    include_directories : common_inc,
    dependencies : [thread_dep, ws2_dep],
    install : false)

test('http_server', test_http_server, suite : 'host')

# PR 5: project_reader unit test (cpp-unit; no REAPER bound).
test_project_reader = executable('test_project_reader',
    'project_reader.cpp',
    'tests/test_project_reader.cpp',
    include_directories : common_inc,
    dependencies : [thread_dep, ws2_dep],
    install : false)

test('project_reader', test_project_reader, suite : 'host')
