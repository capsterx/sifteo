import commands
from waflib.Task import Task
import tempfile


def configure(conf):
  import os
  conf.find_program('lua', var='LUA')
  conf.env.STIRLUA = conf.find_file('stirscan.lua', [conf.srcnode.abspath(), os.path.join(conf.srcnode.abspath(), "waf-utils")])
  conf.find_program('stir', var='STIR')

class stir(Task):
  ext_in = ['.lua']

  def scan(self):
    f = self.inputs[0].abspath()
    get = self.env.get_flat
    status, text = commands.getstatusoutput(
        "%s %s %s" % (get('LUA'), get('STIRLUA'), f)
        )
    nodes = []
    names = []
    for file in text.split('\n'):
      f = self.inputs[0].parent.find_resource(file)
      if f:
        nodes.append(f)
      else:
        names.append(file)
    return (nodes, names)

  def run(self):
    bld = self.inputs[0].__class__.ctx
    get = self.env.get_flat
    outputs = []
    for o in self.outputs:
      outputs.append("-o %s" % o.abspath())

    cmd = "cd %s; %s %s -v %s" % (
          self.inputs[0].parent.abspath(),
          get('STIR'),
          " ".join(outputs),
          self.inputs[0].abspath()
          )
    try:
      file = tempfile.SpooledTemporaryFile()
      ret = bld.exec_command(cmd, stdout=file, stderr=file)
      if ret != 0:
        file.seek(0)
        self.generator.bld.to_log("Stir failed: %s\n" % file.read())
    finally:
      file.close()
    return ret

from waflib.TaskGen import feature, before_method, extension

@feature('stir')
@before_method('process_source')
def stir_files(self):
  self.stir_files = tsk = self.create_task('stir')
  target = self.target
  targets = [
      self.path.find_or_declare("%s.cpp" % target), 
      self.path.find_or_declare("%s.h" % target), 
      self.path.find_or_declare("%s.html" % target)
      ]
  tsk.set_outputs(targets)
  tsk.set_inputs(self.path.find_or_declare(self.source))
  self.export_includes = [targets[0].parent.abspath()]

@extension('.lua')
def foo(self, f):
  pass
