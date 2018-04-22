# ChakraCore

[![Join the chat at https://gitter.im/Microsoft/ChakraCore](https://badges.gitter.im/Microsoft/ChakraCore.svg)](https://gitter.im/Microsoft/ChakraCore?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Licensed under the MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/Microsoft/ChakraCore/blob/master/LICENSE.txt)

ChakraCore is the core part of Chakra, the high-performance JavaScript engine that powers Microsoft Edge and Windows applications written in HTML/CSS/JS.  ChakraCore supports Just-in-time (JIT) compilation of JavaScript for x86/x64/ARM, garbage collection, and a wide range of the latest JavaScript features.  ChakraCore also supports the [JavaScript Runtime (JSRT) APIs](https://github.com/Microsoft/ChakraCore/wiki/JavaScript-Runtime-%28JSRT%29-Overview), which allows you to easily embed ChakraCore in your applications.

You can stay up-to-date on progress by following the [MSEdge developer blog](https://blogs.windows.com/msedgedev/).

## [Build Status](https://github.com/Microsoft/ChakraCore/wiki/Build-Status)

|                               | __Debug__ | __Test__ | __Release__ |
|:-----------------------------:|:---------:|:--------:|:-----------:|
| __Windows (x64)__             | [![x64debug][x64dbgicon]][x64dbglink] | [![x64test][x64testicon]][x64testlink] | [![x64release][x64relicon]][x64rellink] |
| __Windows (x86)__             | [![x86debug][x86dbgicon]][x86dbglink] | [![x86test][x86testicon]][x86testlink] | [![x86release][x86relicon]][x86rellink] |
| __Windows (ARM)__             | [![armdebug][armdbgicon]][armdbglink] | [![armtest][armtesticon]][armtestlink] | [![armrelease][armrelicon]][armrellink] |
| __Ubuntu 16.04 (x64)<sup>[a]</sup>__     | [![linux_a_debug][linux_a_dbgicon]][linux_a_dbglink] | [![linux_a_test][linux_a_testicon]][linux_a_testlink] | [![linux_a_release][linux_a_relicon]][linux_a_rellink] |
| __Ubuntu 16.04 (x64)<sup>[s]</sup>__     | [![linux_s_debug][linux_s_dbgicon]][linux_s_dbglink] | [![linux_s_test][linux_s_testicon]][linux_s_testlink] | [![linux_s_release][linux_s_relicon]][linux_s_rellink] |
| __Ubuntu 16.04 (x64)<sup>[s][n]</sup>__  | * | [![linux_sn_test][linux_sn_testicon]][linux_sn_testlink] | * |
| __OS X 10.9 (x64)<sup>[a]</sup>__        | [![osx_a_debug][osx_a_dbgicon]][osx_a_dbglink] | [![osx_a_test][osx_a_testicon]][osx_a_testlink] | [![osx_a_release][osx_a_relicon]][osx_a_rellink] |
| __OS X 10.9 (x64)<sup>[s][n]</sup>__     | * | [![osx_sn_test][osx_sn_testicon]][osx_sn_testlink] | * |

<sup>[a]</sup> Static | <sup>[s]</sup> Shared | <sup>[n]</sup> NoJIT | * Omitted

[x64dbgicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x64_debug/badge/icon
[x64dbglink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x64_debug/
[x64testicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x64_test/badge/icon
[x64testlink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x64_test/
[x64relicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x64_release/badge/icon
[x64rellink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x64_release/

[x86dbgicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x86_debug/badge/icon
[x86dbglink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x86_debug/
[x86testicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x86_test/badge/icon
[x86testlink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x86_test/
[x86relicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x86_release/badge/icon
[x86rellink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/x86_release/

[armdbgicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/arm_debug/badge/icon
[armdbglink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/arm_debug/
[armtesticon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/arm_test/badge/icon
[armtestlink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/arm_test/
[armrelicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/arm_release/badge/icon
[armrellink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/arm_release/

[linux_a_dbgicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_ubuntu_linux_debug/badge/icon
[linux_a_dbglink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_ubuntu_linux_debug/
[linux_a_testicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_ubuntu_linux_test/badge/icon
[linux_a_testlink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_ubuntu_linux_test/
[linux_a_relicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_ubuntu_linux_release/badge/icon
[linux_a_rellink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_ubuntu_linux_release/

[linux_s_dbgicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/shared_ubuntu_linux_debug/badge/icon
[linux_s_dbglink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/shared_ubuntu_linux_debug/
[linux_s_testicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/shared_ubuntu_linux_test/badge/icon
[linux_s_testlink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/shared_ubuntu_linux_test/
[linux_s_relicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/shared_ubuntu_linux_release/badge/icon
[linux_s_rellink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/shared_ubuntu_linux_release/

[linux_sn_dbgicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_ubuntu_linux_debug/badge/icon
[linux_sn_dbglink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_ubuntu_linux_debug/
[linux_sn_testicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_ubuntu_linux_test/badge/icon
[linux_sn_testlink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_ubuntu_linux_test/
[linux_sn_relicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_ubuntu_linux_release/badge/icon
[linux_sn_rellink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_ubuntu_linux_release/

[osx_a_dbgicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_osx_osx_debug/badge/icon
[osx_a_dbglink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_osx_osx_debug/
[osx_a_testicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_osx_osx_test/badge/icon
[osx_a_testlink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_osx_osx_test/
[osx_a_relicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_osx_osx_release/badge/icon
[osx_a_rellink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/static_osx_osx_release/

[osx_sn_dbgicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_osx_osx_debug/badge/icon
[osx_sn_dbglink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_osx_osx_debug/
[osx_sn_testicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_osx_osx_test/badge/icon
[osx_sn_testlink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_osx_osx_test/
[osx_sn_relicon]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_osx_osx_release/badge/icon
[osx_sn_rellink]: https://ci.dot.net/job/Microsoft_ChakraCore/job/master/job/_no_jit_shared_osx_osx_release/

Above is a table of our rolling build status. We run additional builds on a daily basis. See [Build Status](https://github.com/Microsoft/ChakraCore/wiki/Build-Status) for the status of all builds and additional details.

## Security

If you believe you have found a security issue in ChakraCore, please share it with us privately following the guidance at the Microsoft [Security TechCenter](https://technet.microsoft.com/en-us/security/ff852094). Reporting it via this channel helps minimize risk to projects built with ChakraCore.

## Documentation

* [ChakraCore Architecture](https://github.com/Microsoft/ChakraCore/wiki/Architecture-Overview)
* [Quickstart Embedding ChakraCore](https://github.com/Microsoft/ChakraCore/wiki/Embedding-ChakraCore)
* [JSRT Reference](https://github.com/Microsoft/ChakraCore/wiki/JavaScript-Runtime-%28JSRT%29-Reference)
* [Contribution guidelines](CONTRIBUTING.md)
* [Blogs, talks and other resources](https://github.com/Microsoft/ChakraCore/wiki/Resources)

## [Building ChakraCore](https://github.com/Microsoft/ChakraCore/wiki/Building-ChakraCore)

You can build ChakraCore on Windows 7 SP1 or above, and Windows Server 2008 R2 or above, with either Visual Studio 2015 or 2017 with C++ support installed.  Once you have Visual Studio installed:

* Clone ChakraCore through ```git clone https://github.com/Microsoft/ChakraCore.git```
* Open `Build\Chakra.Core.sln` in Visual Studio
* Build Solution

More details in [Building ChakraCore](https://github.com/Microsoft/ChakraCore/wiki/Building-ChakraCore).

Alternatively, see [Getting ChakraCore binaries](https://github.com/Microsoft/ChakraCore/wiki/Getting-ChakraCore-binaries) for pre-built ChakraCore binaries.

## Using ChakraCore

Once built, you have a few options for how you can use ChakraCore:

* The most basic is to test the engine is running correctly with the *ch.exe* binary.  This app is a lightweight hosting of JSRT that you can use to run small applications.  After building, you can find this binary in:
  * `Build\VcBuild\bin\${platform}_${configuration}`
  * (e.g. `Build\VcBuild\bin\x64_debug`)
* You can [embed ChakraCore](https://github.com/Microsoft/ChakraCore/wiki/Embedding-ChakraCore) in your applications - see [documentation](https://github.com/Microsoft/ChakraCore/wiki/Embedding-ChakraCore) and [samples](https://aka.ms/chakracoresamples).
* Finally, you can also use ChakraCore as the JavaScript engine in Node.  You can learn more by reading how to use [Chakra as Node's JS engine](https://github.com/Microsoft/node)

_A note about using ChakraCore_: ChakraCore is the foundational JavaScript engine, but it does not include the external APIs that make up the modern JavaScript development experience.  For example, DOM APIs like ```document.write()``` are additional APIs that are not available by default and would need to be provided.  For debugging, you may instead want to use ```print()```.

## [Contribute](CONTRIBUTING.md)

Contributions to ChakraCore are welcome.  Here is how you can contribute to ChakraCore:

* [Submit bugs](https://github.com/Microsoft/ChakraCore/issues) and help us verify fixes (please refer to [External Issues](https://github.com/Microsoft/ChakraCore/wiki/External-Issues) for anything external, such as Microsoft Edge or Node-ChakraCore issues)
* [Submit pull requests](https://github.com/Microsoft/ChakraCore/pulls) for bug fixes and features and discuss existing proposals
* Chat about [@ChakraCore](https://twitter.com/ChakraCore) on Twitter

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

Please refer to [Contribution Guidelines](CONTRIBUTING.md) for more details.

## [License](https://github.com/Microsoft/ChakraCore/blob/master/LICENSE.txt)

Code licensed under the [MIT License](https://github.com/Microsoft/ChakraCore/blob/master/LICENSE.txt).

## [Roadmap](https://github.com/Microsoft/ChakraCore/wiki/Roadmap)

For details on our planned features and future direction please refer to our [Roadmap](https://github.com/Microsoft/ChakraCore/wiki/Roadmap).

## Contact Us

If you have questions about ChakraCore, or you would like to reach out to us about an issue you're having or for development advice as you work on a ChakraCore issue, you can reach us as follows:

* Open an [issue](https://github.com/Microsoft/ChakraCore/issues/new) and prefix the issue title with [Question]. See [Question](https://github.com/Microsoft/ChakraCore/issues?q=label%3AQuestion) tag for already-opened questions.
* Discuss ChakraCore with the team and the community on our [Gitter Channel](https://gitter.im/Microsoft/ChakraCore).
* You can also start private messages with individual ChakraCore developers via Gitter.
