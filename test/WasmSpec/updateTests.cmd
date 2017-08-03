::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------
@echo off
pushd %~dp0
if not exist "%1" (
    echo Please provide path to a chakra host, ch or jshost.
    exit /b 1
)

if exist "wasm-spec" (
    rd /q /s wasm-spec
)
git clone --depth 1 --branch master https://github.com/WebAssembly/spec.git wasm-spec
cd wasm-spec
git rev-parse HEAD > ..\testsuite.rev
cd ..
rem Exclude comments.wast because it is intended to test wast->wasm with uncommon characters
rem This is causing problems with our jenkins checks and it is not worth it to run in chakra
robocopy /e /mir wasm-spec\test testsuite /xf comments.wast
rd /q /s wasm-spec

rem regerate testsuite
cd convert-test-suite
call npm install --production
cd ..
node convert-test-suite --rebase %*
git add testsuite\*
git add baselines\*
echo Test updated and regenerated
popd