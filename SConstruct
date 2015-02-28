import os

env=Environment(CC='gcc')

sources=[Glob('src/main.c')]
libs=['ccore', 'X11', 'Xrandr', 'Xinerama', 'Xi', 'GL', 'GLU', 'GLEW', 'pthread', 'm']
libpaths=['/usr/lib', '/usr/local/lib', '.']

env.Append(CCFLAGS=['-DCC_USE_ALL'])

env.Append(CCFLAGS=['-g'])
env.Append(CCFLAGS=['-Wall'])
env.Append(CCFLAGS=['-O3'])
env.Append(CCFLAGS=['-ffast-math'])

env.Program(target='bin/game', source=sources, LIBS=[libs], LIBPATH=libpaths)

sources=[Glob('src/editor.c')]
env.Program(target='bin/editor', source=sources, LIBS=[libs], LIBPATH=libpaths)
