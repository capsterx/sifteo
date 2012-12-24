from waflib.Configure import conf
from waflib import Options
from waflib import Utils

@conf
def sifteo(bld, *k, **kw):
  target_assets = []
  objs = []
  for asset in Utils.to_list(kw.get('assets', [])):
    tgt = "%s" % bld.path.find_resource(asset).change_ext('.gen')
    target_assets.append(tgt)
    objs.append("%s.o" % tgt)
    bld(features='stir', source=asset, target=tgt)
    bld(features='cxx', source='%s.cpp' % tgt, target='%s.o' % tgt, use=tgt)
 
  for source in Utils.to_list(kw.get('source', [])):
    tgt = "%s" % bld.path.find_resource(source).change_ext('.o')
    bld(features='cxx', source=source, target=tgt, use=target_assets)
    objs.append(tgt)
  bld(features='slinky', use=objs, target=kw['target'])
  
def options(opt):
  opt.load('compiler_cxx')
  opt.add_option('--sifteo-dir', action='store', default=False, help='built a fully static plank exec', dest='sifteo_dir')

def configure(conf):
  import os
  sifteo_dir = getattr(Options.options, 'sifteo_dir', False)
  if sifteo_dir:
    sifteo_dir = os.path.expanduser(os.path.expandvars(sifteo_dir))
    os.environ['PATH'] += "%s%s/bin" % (os.pathsep, sifteo_dir)
    if not os.environ.get('CXX'):
      os.environ['CXX'] = os.path.join(sifteo_dir, "bin", 'arm-clang')
  else:
    if not os.environ.get('CXX'):
      os.environ['CXX'] = 'arm-clang'
  conf.load('stir slinky sifteo', tooldir=os.path.dirname(os.path.realpath(__file__)))
  conf.load('compiler_cxx')
  sifteo_include = os.path.join("%s" % conf.top_dir, "include")
  if sifteo_dir:
    sifteo_include = os.path.join(sifteo_dir, "include")

  conf.env.CXXFLAGS = [
     "-fno-exceptions",
     "-fno-threadsafe-statics",
     "-fno-rtti",
     "-fno-stack-protector",
     "-I%s" % sifteo_include,
     "-I.",
     "-emit-llvm",
     "-ffreestanding",
     "-nostdinc",
     "-msoft-float",
     "-Wall",
     "-Werror",
     "-Wno-unused",
     "-Wno-gnu",
     "-Wno-c++11-extensions",
     "-Wunused-result",
     "-MD",
     "-MP",
     "-g"
     ]
