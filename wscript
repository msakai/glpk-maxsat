from waflib import Options

APPNAME = 'glpk-maxsat'
VERSION = '0.0.1'

srcdir = '.'
blddir = 'build'

def options(opt):
   opt.load('compiler_cxx')
   opt.add_option('--static', dest='staticlink', default=False, action='store_true', help='statically link')

def configure(conf):
   conf.load('compiler_cxx')
   if Options.options.staticlink:
      # https://code.google.com/p/waf/wiki/FAQ
      # For gcc, set conf.env.SHLIB_MARKER = '-Wl,-Bstatic' to link all libraries in static mode,
      # and add '-static' to the linkflags to make a fully static binary.
      conf.env.SHLIB_MARKER = '-Wl,-Bstatic'
      conf.env.append_value('LINKFLAGS', "-static")
      #conf.check(lib = ['glpk', 'z', 'gmp', 'ltdl', 'dl'], header = ['glpk'], mandatory = True, uselib_store='GLPK')
      conf.check(lib = ['glpk', 'gmp'], header = ['glpk'], mandatory = True, uselib_store='GLPK')
   else:
      conf.check(lib = ['glpk'], header = ['glpk'], mandatory = True, uselib_store='GLPK')

def build(bld):
   bld.program(
      source = 'glpsol-maxsat.cpp',
      target = 'glpsol-maxsat',
      use = 'GLPK')
