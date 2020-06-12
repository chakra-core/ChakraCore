# ChakraCore

[![Join the chat at https://gitter.im/Microsoft/ChakraCore](https://badges.gitter.im/Microsoft/ChakraCore.svg)](https://gitter.im/Microsoft/ChakraCore?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Discord Chat](https://img.shields.io/discord/695166668967510077?label=Discord&logo=Discord)](https://discord.gg/3e49Ptz)
[![Licensed under the MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/Microsoft/ChakraCore/blob/master/LICENSE.txt)
[![PR's Welcome](https://img.shields.io/badge/PRs%20-welcome-brightgreen.svg)](#contribute)

ChakraCore is the core part of Chakra, the high-performance JavaScript engine that powers Windows applications written in HTML/CSS/JS and used to power Microsoft Edge.  ChakraCore supports Just-in-time (JIT) compilation of JavaScript for x86/x64/ARM, garbage collection, and a wide range of the latest JavaScript features.  ChakraCore also supports the [JavaScript Runtime (JSRT) APIs](https://github.com/Microsoft/ChakraCore/wiki/JavaScript-Runtime-%28JSRT%29-Overview), which allows you to easily embed ChakraCore in your applications.

## Future of ChakraCore

As you may have heard Microsoft Edge no longer uses Chakra. Microsoft will continue to provide security updates for Chakracore 1.11 until 9th March 2021 but do not intend to support it after that.

However ChakraCore is planned to continue as a community project targeted primarily at embedded use cases. We hope to produce future releases with new features and enhancements to support such use cases. We also would like to invite any interested parties to be involved in this project. For further details please see the following draft planning documents:
[Overall plan](https://github.com/chakra-core/org/blob/master/ChakraCore%20Future%20Plan.md)
[Version 1.12 plan](https://github.com/chakra-core/org/blob/master/Release%201.12%20plan.md)

Also see discussion in issue [#6384](https://github.com/microsoft/ChakraCore/issues/6384)

If you'd like to contact the community team please either open an issue or join the discord chat linked above.

## Build Status

|                               | __Debug__ | __Test__ | __Release__ |
|:-----------------------------:|:---------:|:--------:|:-----------:|
| __Windows 10 (x64)__             | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20x64_debug)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20x64_test)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20x64_release)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) |
| __Windows 10 (x86)__             | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20x86_debug)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20x86_test)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20x86_release)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) |
| __Windows 10 (ARM)__             | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20arm_debug)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20arm_test)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20arm_release)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) |
| __Windows 10 (ARM64)__           | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20arm64_debug)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20arm64_test)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Windows%2010%20-%20daily?branchName=master&jobName=Build%5Cscripts%5C*.ps1&configuration=Build%5Cscripts%5C*.ps1%20arm64_release)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=50&branchName=master) |
| __Ubuntu 16.04 (x64)<sup>[a]</sup>__     | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Linux%20(Ubuntu%2016.04)%20-%20daily?branchName=master&jobName=static%20debug)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=51&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Linux%20(Ubuntu%2016.04)%20-%20daily?branchName=master&jobName=static%20test)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=51&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Linux%20(Ubuntu%2016.04)%20-%20daily?branchName=master&jobName=static%20release)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=51&branchName=master) |
| __Ubuntu 16.04 (x64)<sup>[s]</sup>__     | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Linux%20(Ubuntu%2016.04)%20-%20daily?branchName=master&jobName=shared%20debug)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=51&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Linux%20(Ubuntu%2016.04)%20-%20daily?branchName=master&jobName=shared%20test)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=51&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Linux%20(Ubuntu%2016.04)%20-%20daily?branchName=master&jobName=shared%20release)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=51&branchName=master) |
| __Ubuntu 16.04 (x64)<sup>[s][n]</sup>__  | * | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/Linux%20(Ubuntu%2016.04)%20-%20daily?branchName=master&jobName=no%20jit%20shared%20test%20)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=51&branchName=master) | * |
| __macOS 10.13 (x64)<sup>[a]</sup>__        | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/macOS%20-%20daily?branchName=master&jobName=static%20debug)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=52&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/macOS%20-%20daily?branchName=master&jobName=static%20test)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=52&branchName=master) | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/macOS%20-%20daily?branchName=master&jobName=static%20release)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=52&branchName=master) |
| __macOS 10.13 (x64)<sup>[s][n]</sup>__     | * | [![Build Status](https://chakrateam.visualstudio.com/Push_Build_Runner/_apis/build/status/daily/macOS%20-%20daily?branchName=master&jobName=no%20jit%20shared%20test%20)](https://chakrateam.visualstudio.com/Push_Build_Runner/_build/latest?definitionId=52&branchName=master) | * |

<sup>[a]</sup> Static | <sup>[s]</sup> Shared | <sup>[n]</sup> NoJIT | * Omitted

Above is a table of our rolling build status. We run additional builds on a daily basis. See [Build Status](https://github.com/Microsoft/ChakraCore/wiki/Build-Status) for the status of all builds and additional details.

## Security

If you believe you have found a security issue in ChakraCore, please share it with us privately following the guidance at the Microsoft [Security TechCenter](https://technet.microsoft.com/en-us/security/ff852094). Reporting it via this channel helps minimize risk to projects built with ChakraCore.

## Documentation

* [ChakraCore Architecture](https://github.com/Microsoft/ChakraCore/wiki/Architecture-Overview)
* [Quickstart Embedding ChakraCore](https://github.com/Microsoft/ChakraCore/wiki/Embedding-ChakraCore)
* [JSRT Reference](https://github.com/Microsoft/ChakraCore/wiki/JavaScript-Runtime-%28JSRT%29-Reference)
* [Contribution guidelines](CONTRIBUTING.md)
* [Blogs, talks and other resources](https://github.com/Microsoft/ChakraCore/wiki/Resources)

## Building ChakraCore

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

Alternatively, if you are using the [vcpkg](https://github.com/Microsoft/vcpkg/) dependency manager you can download and install ChakraCore with CMake integration in a single command:
* vcpkg install chakracore

## Contribute

Contributions to ChakraCore are welcome.  Here is how you can contribute to ChakraCore:

* [Submit bugs](https://github.com/Microsoft/ChakraCore/issues) and help us verify fixes (please refer to [External Issues](https://github.com/Microsoft/ChakraCore/wiki/External-Issues) for anything external, such as Microsoft Edge or Node-ChakraCore issues)
* [Submit pull requests](https://github.com/Microsoft/ChakraCore/pulls) for bug fixes and features and discuss existing proposals
* Chat about [@ChakraCore](https://twitter.com/ChakraCore) on Twitter

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

Please refer to [Contribution Guidelines](CONTRIBUTING.md) for more details.

## License

Code licensed under the [MIT License](https://github.com/Microsoft/ChakraCore/blob/master/LICENSE.txt).

## Roadmap

For details on our planned features and future direction please refer to our [Roadmap](https://github.com/Microsoft/ChakraCore/wiki/Roadmap).

## Contact Us

If you have questions about ChakraCore, or you would like to reach out to us about an issue you're having or for development advice as you work on a ChakraCore issue, you can reach us as follows:

* Open an [issue](https://github.com/Microsoft/ChakraCore/issues/new) and prefix the issue title with [Question]. See [Question](https://github.com/Microsoft/ChakraCore/issues?q=label%3AQuestion) tag for already-opened questions.
* Discuss ChakraCore with the team and the community on our [Gitter Channel](https://gitter.im/Microsoft/ChakraCore).
* You can also start private messages with individual ChakraCore developers via Gitter.
