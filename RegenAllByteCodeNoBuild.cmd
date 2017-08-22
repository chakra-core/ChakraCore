::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

:: WARNING: be careful when using this script as it assumes that
:: you already have bytecode-format-compatible builds for all required flavors.
:: This script helps speed things up when you are only making changes to scripts,
:: e.g. Intl.js, without making any changes to bytecode format, since rebuilding
:: every flavor of ChakraCore.dll when there are no relevant changes is a waste of time.
:: Please ensure that you use buddy builds to validate the results.

:: Regenerate all bytecode (without rebuilding each flavor of ch.exe)
:: ch.exe is used to generate Intl bytecodes.
:: ch.exe (NoJIT variety) is used to generate NoJIT Intl bytecodes.
:: Each set of bytecode requires an x86_debug and x64_debug binary.
::
:: Thus we need to already have compatible builds of the following:
:: [Core]   ch.exe        x64_debug
:: [Core]   ch.exe        x86_debug
:: [Core]   ch.exe        x64_debug (NoJIT)
:: [Core]   ch.exe        x86_debug (NoJIT)

@echo off
setlocal
  set _reporoot=%~dp0
  pushd %_reporoot%\lib\Runtime\Library\InJavascript
    call GenByteCode.cmd
    call GenByteCode.cmd -nojit
  popd
endlocal
