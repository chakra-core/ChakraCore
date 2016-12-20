::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

:: Regenerate all bytecode.
:: ch.exe is used to generate Intl bytecodes.
:: ch.exe (NoJIT variety) is used to generate NoJIT Intl bytecodes.
:: Each set of bytecode requires an x86_debug and x64_debug binary.
::
:: Thus we need to build the following:
:: [Core]   ch.exe        x64_debug
:: [Core]   ch.exe        x86_debug
:: [Core]   ch.exe        x64_debug (NoJIT)
:: [Core]   ch.exe        x86_debug (NoJIT)

setlocal
pushd %~dp0

:: ch.exe        x64_debug
:: ch.exe        x86_debug
call jenkins\buildone.cmd x64 debug
if %errorlevel% neq 0 (
  echo There was a build error for x64 debug. Stopping bytecode generation.
  exit /b 1
)
call jenkins\buildone.cmd x86 debug
if %errorlevel% neq 0 (
  echo There was a build error for x86 debug. Stopping bytecode generation.
  exit /b 1
)

pushd lib\Runtime\Library\InJavascript
call GenByteCode.cmd
if %errorlevel% neq 0 (
  echo There was an error when regenerating bytecode header.
  exit /b 1
)
popd

:: ch.exe        x64_debug (NoJIT)
:: ch.exe        x86_debug (NoJIT)
call jenkins\buildone.cmd x64 debug "/p:BuildJIT=false"
if %errorlevel% neq 0 (
  echo There was a build error for x64 debug NoJIT. Stopping bytecode generation.
  exit /b 1
)

call jenkins\buildone.cmd x86 debug "/p:BuildJIT=false"
if %errorlevel% neq 0 (
  echo There was a build error for x86 debug NoJIT. Stopping bytecode generation.
  exit /b 1
)

:: Generate Intl NoJIT Bytecodes using ch.exe (NoJIT)
pushd lib\Runtime\Library\InJavascript
call GenByteCode.cmd -nojit
if %errorlevel% neq 0 (
  echo There was an error when regenerating bytecode header for NoJIT.
  exit /b 1
)
popd

popd

endlocal
