from waflib.TaskGen import feature, before_method, extension
from waflib.Task import Task

def configure(conf):
  conf.find_program('slinky', var='SLINKY')

class slinky(Task):
  def run(self):
    bld = self.inputs[0].__class__.ctx
    get = self.env.get_flat
    inputs = []
    for i in self.inputs:
      inputs.append("%s" % i.abspath())
    cmd = "%s -o %s %s -g" % (
          get('SLINKY'),
          self.outputs[0].abspath(),
          " ".join(inputs)
          )
    return bld.exec_command(cmd)


    print "Running %s" % self.inputs[0]
@feature('slinky')
@before_method('process_source')
def slinky_files(self):
  self.stir_files = mytsk = self.create_task('slinky')
  mytsk.set_outputs(self.path.find_or_declare(self.target))
  names = self.to_list(getattr(self, 'use', []))
  use_not = self.tmp_use_not = set([])
  self.tmp_use_seen = []
  use_prec = self.tmp_use_prec = {}


  for x in names:
    self.use_rec(x)
  for x in use_not:
    if x in use_prec:
      del use_prec[x]

  # topological sort
  out = []
  tmp = []
  for x in self.tmp_use_seen:
    for k in use_prec.values():
      if x in k:
        break
    else:
      tmp.append(x)

  while tmp:
    e = tmp.pop()
    out.append(e)
    try:
      nlst = use_prec[e]
    except KeyError:
      pass
    else:
      del use_prec[e]
      for x in nlst:
        for y in use_prec:
          if x in use_prec[y]:
            break
        else:
          tmp.append(x)
  if use_prec:
    raise Errors.WafError('Cycle detected in the use processing %r' % use_prec)
  out.reverse()

  for x in out:
    y = self.bld.get_tgen_by_name(x)
    var = y.tmp_use_var
    if y.tmp_use_objects:
      for tsk in getattr(y, 'compiled_tasks', []):
        for x in tsk.outputs:
          mytsk.inputs.append(x)
