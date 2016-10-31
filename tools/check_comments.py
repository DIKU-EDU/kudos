#!/usr/bin/env python3

import os, os.path, re

__COMMENT = re.compile("/\*.*\*/", re.DOTALL)

class CommentError(Exception):
  def __init__(self, message):
    self.message = message

def _read_file(path):
  with open(path, "r") as fin:
    return fin.read()

def _check_file(path):
  code = _read_file(path)
  if __COMMENT.search(code):
    raise CommentError(
      "{} is using multiline comments. Only //-style comments are allowed.".format(path))

def _try_check_file(path):
  try:
    _check_file(path)
  except CommentError as e:
    print(e.message)

def check_comments():
  for (dirpath, _, filenames) in os.walk("."):
    for filename in filenames:
      if filename[-2:] in [".h", ".c"]:
        path = os.path.join(dirpath, filename)
        _try_check_file(path)

if __name__ == "__main__":
  check_comments()
