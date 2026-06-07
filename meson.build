project('reaforge', 'cpp',
    version : '0.0.1',
    license : 'MIT',
    default_options : ['cpp_std=c++17', 'warning_level=2'])

cc = meson.get_compiler('cpp')

reaimgui = subproject('reaimgui', required : false)
have_reaimgui = reaimgui.found()
message('ReaImGui: ' + (have_reaimgui ? 'subproject found' : 'not available (panel will be a stub until vendored build is wired)'))

subdir('src/host')
