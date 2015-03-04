import os

env=Environment(CC='gcc')

sources=[Glob('src/l_*.c')]
libs=['ccore', 'X11', 'Xrandr', 'Xinerama', 'Xi', 'GL', 'GLU', 'GLEW', 'pthread', 'm']
libpaths=['/usr/lib', '/usr/local/lib', '.']

env.Append(CCFLAGS=['-DCC_USE_ALL'])

env.Append(CCFLAGS=['-g'])
env.Append(CCFLAGS=['-Wall'])
env.Append(CCFLAGS=['-O3'])
env.Append(CCFLAGS=['-ffast-math'])
env.Append(CCFLAGS=['-Iinclude/'])

staticLibrary=env.Library(target='lib/roguelib', source=sources, LIBS=libs, LIBPATH=libpaths)
env.Program(target='bin/editor', source=['src/e_main.c'], LIBS=[staticLibrary, libs], LIBPATH=libpaths)
env.Program(target='bin/game', source=['src/g_main.c'], LIBS=[staticLibrary, libs], LIBPATH=libpaths)
