#
# (C) 2011-2013 KATO Takeshi
#

VERSION = '0.0.1.0'
APPNAME = 'uniqos'

top = '.'
out = 'build'

import os.path

def options(x):
	x.load('compiler_c')
	x.load('compiler_cxx')
	x.load('gcc gas')

	conf_opt = x.get_option_group('configure options')
	conf_opt.add_option('--compiler',
	    default = 'clang',
	    action  = 'store',
	    help    = 'compiler family (clang, llvm, gnu)')

	x.recurse('arch')


def configure(x):
	x.recurse('tools')

	if x.options.compiler == 'clang':
		x.find_program('clang', var='AS')
		x.find_program('clang', var='CC')
		x.find_program('clang++', var='CXX')
		x.env.append_unique('CXXFLAGS_KERNEL', '-std=c++11')
		x.env.append_unique('CXXFLAGS_KERNEL', '-Wc++11-compat')
		x.env.append_unique('CXXFLAGS_KERNEL', '-Wc++11-extensions')

	elif x.options.compiler == 'gnu':
		x.find_program('gcc', var='AS')
		x.find_program('gcc', var='CC')
		x.find_program('g++', var='CXX')
		x.env.append_unique('CXXFLAGS_KERNEL', '-std=c++0x')

	x.load('compiler_c')
	x.load('compiler_cxx')
	x.load('gcc gas')

	x.recurse('arch')
	x.recurse('drivers')
	x.recurse('external')


def build(x):
	x.env.append_value('DEFINES', 'IT_IS_UNIQUE')
	x.env.append_value('DEFINES_KERNEL', 'KERNEL')
	x.env.append_value('INCLUDES', '#include')
	x.env.append_value('INCLUDES', '#core/include')
	x.env.append_value('INCLUDES', '#arch/'+x.env.CONFIG_ARCH+'/include')

	x.recurse('arch')
	x.recurse('drivers')
	x.recurse('external')
	x.recurse('core/source')

