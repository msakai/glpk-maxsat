APPNAME = 'glpk-maxsat'
VERSION = '0.0.1'

srcdir = '.'
blddir = 'build'

def options(opt):
   opt.load('compiler_cxx')

def configure(conf):
   conf.load('compiler_cxx')
   conf.check(lib = ['glpk'], header = ['glpk'], mandatory = True, uselib_store='GLPK')

def build(bld):
   bld.program(
      source = 'glpsol-maxsat.cpp',
      target = 'glpsol-maxsat',
      use = 'GLPK')
