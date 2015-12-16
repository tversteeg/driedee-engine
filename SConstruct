import os

env=Environment(CC='gcc')

sources=[Glob('src/l_*.c')]
libs=['ccore', 'X11', 'Xrandr', 'Xinerama', 'Xi', 'GL', 'GLU', 'GLEW', 'pthread', 'm', 'png']
libpaths=['/usr/lib', '/usr/local/lib', '.']

env.Append(CCFLAGS=['-DCC_USE_ALL'])

env.Append(CCFLAGS=['-g'])
env.Append(CCFLAGS=['-Wall'])
env.Append(CCFLAGS=['-Wextra'])
env.Append(CCFLAGS=['-O1'])
env.Append(CCFLAGS=['-ffast-math'])
env.Append(CCFLAGS=['-Iinclude/'])
#env.Append(CCFLAGS=['-coverage'])

staticLibrary=env.Library(target='lib/roguelib', source=sources, LIBS=libs, LIBPATH=libpaths)
env.Program(target='bin/game', source=[Glob('src/g_*.c')], LIBS=[staticLibrary, libs], LIBPATH=libpaths)
