project('jzip', 'c', version : '2.1', license : 'ModifiedBSD')

cc = meson.get_compiler('c')

link_args = []
compile_args = ['-DPOSIX', '-DHAVE_GETOPT', '-DHARD_COLORS']

lib_src = files([
  'control.c', 'extern.c', 'fileio.c', 'input.c', 'interpre.c',
  'license.c', 'math.c', 'memory.c', 'object.c', 'operand.c', 'osdepend.c',
  'property.c', 'quetzal.c', 'screen.c', 'text.c', 'variable.c'])

headers = files(['ztypes.h', 'jzip.h'])

if host_machine.system() == 'windows'
  lib_src += ['winio.c', 'conio.c']
  headers += files('conio2.h')
  compile_args += '-DLOUSY_RANDOM'
else
  lib_src += 'unixio.c'
  link_args += '-ltermcap'
endif

jzip_lib = static_library('jzip',
  lib_src,
  c_args : compile_args)

incdir = include_directories('.')

jzip_dep = declare_dependency(
  link_with : jzip_lib,
  link_args : link_args,
  include_directories : incdir)

jzip = executable('jzip',
  'jzip.c',
  c_args: compile_args,
  link_with : jzip_lib,
  link_args : link_args,
  include_directories : incdir)

ckifzs = executable('ckifzs',
  'ckifzs.c',
  c_args: compile_args,
  link_args : link_args,
  include_directories : incdir)

