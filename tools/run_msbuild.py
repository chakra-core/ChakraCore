#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

import glob
import json
import os
import pipes
import platform
import re
import shutil
import stat
import subprocess
import sys

# add build to path
BUILD_TOOLS_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), "../../../build"))
sys.path.append(BUILD_TOOLS_DIR)
import find_depot_tools

find_depot_tools.add_depot_tools_to_path()
import vswhere


# On Windows, run msbuild.exe on sln and copy lib, dll, pdb files to appropriate locations, 
def main(sln, outdir, target_gen_dir, *flags):

  # On non-Windows, that's all we can do.
  if sys.platform != 'win32':
    print "builds on non-windows are NYI!!"
    return 1

  status_code, vsInstallRoot = vswhere.get_vs_path()
  if status_code != 0:
    print "Visual Studio not found!!"
    return 1

  msbuildPath = os.path.join(vsInstallRoot["path"], r"MSBuild\Current\Bin\MSBuild.exe")
  if not os.path.exists(msbuildPath):
    msbuildPath = os.path.join(vsInstallRoot["path"], r"MSBuild\15.0\Bin\MSBuild.exe")
  args = [msbuildPath, os.path.normpath(sln)] + list(flags)

  print "== BUILDING CHAKRACORE:"
  print " ".join(args)

  popen = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  out, _ = popen.communicate() 

  if popen.returncode != 0:
      print out
      return popen.returncode
  
  # Now copy the lib  target_gen_dir and  dll/pdb  to the build root 
  # 2 levels above  ( above gen\third_party\chakracore )
  shutil.copy(os.path.join(outdir, "ChakraCore.lib"), target_gen_dir)
  shutil.copy(os.path.join(outdir, "ChakraCore.dll"), os.path.join(target_gen_dir, "../../.."))
  shutil.copy(os.path.join(outdir, "ChakraCore.pdb"), os.path.join(target_gen_dir, "../../.."))

  return 0

if __name__ == '__main__':
  sys.exit(main(*sys.argv[1:]))
