top = '.'
out = 'build'

def options(opt):
  opt.load('sifteo', tooldir='waf-utils')

def configure(conf):
  conf.load('sifteo', tooldir='waf-utils')

def build(bld):
  bld.recurse('src')
