project('minecraft_clone', 'cpp', default_options: 'default_library=static')

cmake = import('cmake')

raylib_opts = cmake.subproject_options()
raylib_opts.add_cmake_defines({
  'BUILD_EXAMPLES': 'OFF',
  # 'GLFW_BUILD_X11': 'OFF',
  # 'GLFW_BUILD_WAYLAND': 'ON',
  'BUILD_SHARED_LIBS': 'ON',
})

raylib_proj = cmake.subproject('raylib', options: raylib_opts)
raylib_dep = raylib_proj.dependency('raylib')

executable(
  'game',
  'src/main.cpp',
  dependencies: raylib_dep,
)
