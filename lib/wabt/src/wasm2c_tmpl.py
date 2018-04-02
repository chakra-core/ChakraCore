#!/usr/bin/env python
#
# Copyright 2018 WebAssembly Community Group participants
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from __future__ import print_function
import argparse
try:
  from cStringIO import StringIO
except ImportError:
  from io import StringIO
import os
import sys

def EscapeCString(s):
  out = ''
  for b in bytearray(s.encode('utf-8')):
    if b in (34, 92):
      # " or \
      out += '\\' + chr(b)
    elif b == 10:
      # newline
      out += '\\n'
    elif 32 <= b <= 127:
      # printable char
      out += chr(b)
    else:
      # non-printable; write as \xab
      out += '\\x%02x' % b

  return out


def main(args):
  arg_parser = argparse.ArgumentParser()
  arg_parser.add_argument('-o', '--output', metavar='PATH',
                          help='output file.')
  arg_parser.add_argument('file', help='input file.')
  options = arg_parser.parse_args(args)

  section_name = None
  output = StringIO()

  output.write('/* Generated from \'%s\' by wasm2c_tmpl.py, do not edit! */\n' %
               os.path.basename(options.file))

  with open(options.file) as f:
    for line in f.readlines():
      if line.startswith('%%'):
        if section_name is not None:
          output.write(';\n\n');
        section_name = line[2:-1]
        output.write('const char SECTION_NAME(%s)[] =\n' % section_name)
      else:
        output.write('"%s"\n' % EscapeCString(line))

  output.write(';\n');
  if options.output:
    with open(options.output, 'w') as outf:
      outf.write(output.getvalue())
  else:
    sys.stdout.write(output.getvalue())

  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
