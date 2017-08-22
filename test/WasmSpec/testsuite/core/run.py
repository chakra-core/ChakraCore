#!/usr/bin/env python

from __future__ import print_function
import argparse
import os
import os.path
import unittest
import subprocess
import glob
import sys


ownDir = os.path.dirname(os.path.abspath(sys.argv[0]))
inputDir = ownDir
outputDir = os.path.join(inputDir, "_output")

parser = argparse.ArgumentParser()
parser.add_argument("--wasm", metavar="<wasm-command>", default=os.path.join(os.getcwd(), "wasm"))
parser.add_argument("--js", metavar="<js-command>")
parser.add_argument("--out", metavar="<out-dir>", default=outputDir)
parser.add_argument("file", nargs='*')
arguments = parser.parse_args()
sys.argv = sys.argv[:1]

wasmCommand = arguments.wasm
jsCommand = arguments.js
outputDir = arguments.out
inputFiles = arguments.file if arguments.file else glob.glob(os.path.join(inputDir, "*.wast"))


class RunTests(unittest.TestCase):
  def _runCommand(self, command, logPath, expectedExitCode = 0):
    with open(logPath, 'w+') as out:
      exitCode = subprocess.call(command, shell=True, stdout=out, stderr=subprocess.STDOUT)
      self.assertEqual(expectedExitCode, exitCode, "failed with exit code %i (expected %i) for %s" % (exitCode, expectedExitCode, command))

  def _auxFile(self, path):
    if os.path.exists(path):
      os.remove(path)
    return path

  def _compareFile(self, expectFile, actualFile):
    if os.path.exists(expectFile):
      with open(expectFile) as expect:
        with open(actualFile) as actual:
          expectText = expect.read()
          actualText = actual.read()
          self.assertEqual(expectText, actualText)

  def _runTestFile(self, inputPath):
    dir, inputFile = os.path.split(inputPath)
    outputPath = os.path.join(outputDir, inputFile)

    # Run original file
    expectedExitCode = 1 if ".fail." in inputFile else 0
    logPath = self._auxFile(outputPath + ".log")
    self._runCommand(('%s "%s"') % (wasmCommand, inputPath), logPath, expectedExitCode)

    if expectedExitCode != 0:
      return

    # Convert to binary and validate again
    wasmPath = self._auxFile(outputPath + ".bin.wast")
    logPath = self._auxFile(wasmPath + ".log")
    self._runCommand(('%s -d "%s" -o "%s"') % (wasmCommand, inputPath, wasmPath), logPath)
    self._runCommand(('%s -d "%s"') % (wasmCommand, wasmPath), logPath)

    # Convert back to text and validate again
    wastPath = self._auxFile(wasmPath + ".wast")
    logPath = self._auxFile(wastPath + ".log")
    self._runCommand(('%s -d "%s" -o "%s"') % (wasmCommand, wasmPath, wastPath), logPath)
    self._runCommand(('%s -d "%s" ') % (wasmCommand, wastPath), logPath)

    # Convert back to binary once more and compare
    wasm2Path = self._auxFile(wastPath + ".bin.wast")
    logPath = self._auxFile(wasm2Path + ".log")
    self._runCommand(('%s -d "%s" -o "%s"') % (wasmCommand, wastPath, wasm2Path), logPath)
    self._runCommand(('%s -d "%s"') % (wasmCommand, wasm2Path), logPath)
    # TODO: The binary should stay the same, but OCaml's float-string conversions are inaccurate.
    # Once we upgrade to OCaml 4.03, use sprintf "%s" for printing floats.
    # self._compareFile(wasmPath, wasm2Path)

    # Convert to JavaScript
    jsPath = self._auxFile(outputPath.replace(".wast", ".js"))
    logPath = self._auxFile(jsPath + ".log")
    self._runCommand(('%s -d "%s" -o "%s"') % (wasmCommand, inputPath, jsPath), logPath)
    if jsCommand != None:
      self._runCommand(('%s "%s"') % (jsCommand, jsPath), logPath)


if __name__ == "__main__":
  if not os.path.exists(outputDir):
    os.makedirs(outputDir)
  for fileName in inputFiles:
    testName = 'test ' + os.path.basename(fileName)
    setattr(RunTests, testName, lambda self, file=fileName: self._runTestFile(file))
  unittest.main()
