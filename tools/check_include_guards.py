#!/usr/bin/env python3

import os, os.path, re, subprocess

__GUARD = re.compile(
  "^.*?\n\s*#ifndef\s+([^\s]+)\s+#define\s+([^\s]+).*?#endif\s+$",
  re.DOTALL)

__ALL_GUARDS = {}

class IncludeGuardException(Exception):
  def __init__(self, message):
    self.message = message

def read_without_comments(path):
  command = ["gcc", "-fpreprocessed", "-dD", "-E", path]
  return subprocess.run(command,
    stdout=subprocess.PIPE,
    universal_newlines=True).stdout

def read_last_line(path):
  with open(path, "r") as f:
    return f.read().splitlines()[-1]

def check_prefix(prefix, name, path):
  if not name.startswith(prefix):
    raise IncludeGuardException(
      "{} include guard should begin with {}".format(path, prefix))

def join_path_parts(prefix, parts):
  parts = map(lambda c: re.sub("[^A-Za-z0-9_]", "_", c).upper(), parts)
  return prefix + "_".join(parts)

def check_postfix(prefix, name, path):
  parts = path.split(os.path.sep)[1:]

  goodname = join_path_parts(prefix, parts)
  if goodname != name:
    raise IncludeGuardException(
      "{} include guard should be named {}".format(path, goodname))

  if goodname in __ALL_GUARDS:
    raise IncludeGuardException(
      "There's an include guard conflict between {} and {}".format(
        path, __ALL_GUARDS[goodname]))
  __ALL_GUARDS[goodname] = path

def check_header(prefix, path):
  output = read_without_comments(path)

  m = __GUARD.search(output)
  if not m:
    raise IncludeGuardException(
      "{} doesn't seem to be protected by a proper include guard.".format(path))
  if m.group(1) != m.group(2):
    raise IncludeGuardException(
      "{} begins with a malformed include guard.".format(path))

  name = m.group(1)
  check_prefix(prefix, name, path)
  check_postfix(prefix, name, path)

  last_line = read_last_line(path)

  exp_last_line = "#endif // {}".format(name)

  if last_line != exp_last_line:
    raise IncludeGuardException(
      "{} should end with {}".format(path, exp_last_line))

def try_check_header(prefix, path):
  try:
    check_header(prefix, path)
  except IncludeGuardException as e:
    print(e.message)

def checkdir(dirname, prefix):
  for (dirpath, _, filenames) in os.walk(dirname):
    for filename in filenames:
      if filename.endswith(".h"):
        try_check_header(prefix, os.path.join(dirpath, filename))

def check_include_guards():
  checkdir("kudos", "KUDOS_")
  checkdir("userland", "KUDOS_USERLAND_")

if __name__ == "__main__":
  check_include_guards()
