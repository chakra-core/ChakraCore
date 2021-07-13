# ChakraCore

[![Discord Chat](https://img.shields.io/discord/695166668967510077?label=Discord&logo=Discord)](https://discord.gg/vT5J7adt)
[![Licensed under the MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/Microsoft/ChakraCore/blob/master/LICENSE.txt)
[![PR's Welcome](https://img.shields.io/badge/PRs%20-welcome-brightgreen.svg)](#contribute)

ChakraCore is a Javascript engine with a C API you can use to add support for Javascript to any C or C compatible project. It can be compiled for x64 processors on Linux macOS and Windows. And x86 and ARM for windows only. It is a future goal to support x86 and ARM processors on Linux and ARM on macOS. 

## Future of ChakraCore

As you may have heard Microsoft Edge no longer uses Chakra. Microsoft will continue to provide security updates for ChakraCore 1.11 until 9th March 2021 but do not intend to support it after that.

ChakraCore is planned to continue as a community project targeted primarily at embedded use cases. We hope to produce future releases with new features and enhancements to support such use cases. We also would like to invite any interested parties to be involved in this project. For further details please see the following draft planning documents:
[Overall plan](https://github.com/chakra-core/org/blob/master/ChakraCore%20Future%20Plan.md)
[Version 1.12 plan](https://github.com/chakra-core/org/blob/master/Release%201.12%20plan.md)

Also see discussion in issue [#6384](https://github.com/microsoft/ChakraCore/issues/6384)

If you'd like to contact the community team please either open an issue or join the discord chat linked above.

## Security

If you believe you have found a security issue in ChakraCore 1.11, please share it with Microsoft privately following the guidance at the Microsoft [Security TechCenter](https://technet.microsoft.com/en-us/security/ff852094). Reporting it via this channel helps minimize risk to projects built with ChakraCore.

If you find a security issue in the Master branch of Chakracore but not in 1.11 please join our discord server and private message one of the Core team members.

## Documentation

* [ChakraCore Architecture](https://github.com/chakra-core/ChakraCore/wiki/Architecture-Overview)
* [Quickstart Embedding ChakraCore](https://github.com/chakra-core/ChakraCore/wiki/Embedding-ChakraCore)
* [API Reference](https://github.com/chakra-core/ChakraCore/wiki/JavaScript-Runtime-%28JSRT%29-Reference)
* [Contribution guidelines](CONTRIBUTING.md)
* [Blogs, talks and other resources](https://github.com/chakra-core/ChakraCore/wiki/Resources)

## Building ChakraCore

You can build ChakraCore on Windows 7 SP1 or above, and Windows Server 2008 R2 or above, with either Visual Studio 2015 or 2017 with C++ support installed.  Once you have Visual Studio installed:

* Clone ChakraCore through ```git clone https://github.com/Microsoft/ChakraCore.git```
* Open `Build\Chakra.Core.sln` in Visual Studio
* Build Solution

On macOS you can build ChakraCore with the xcode command line tools and `cmake`.
On Linux you can build ChakraCore with `cmake` and `ninja`.

More details in [Building ChakraCore](https://github.com/chakra-core/ChakraCore/wiki/Building-ChakraCore).

Alternatively, see [Getting ChakraCore binaries](https://github.com/Microsoft/ChakraCore/wiki/Getting-ChakraCore-binaries) for pre-built ChakraCore binaries.

## Using ChakraCore

Once built, you have a few options for how you can use ChakraCore:

* The most basic is to test the engine is running correctly with the application *ch.exe* (ch on linux or macOS).  This app is a lightweight host of ChakraCore that you can use to run small applications.  After building, you can find this binary in:
  * Windows: `Build\VcBuild\bin\${platform}_${configuration}` (e.g. `Build\VcBuild\bin\x64_debug`)
  * Mac/Linux: `buildFolder/config/ch` (e.g. `out/Release/ch`)
* You can [embed ChakraCore](https://github.com/chakra-core/ChakraCore/wiki/Embedding-ChakraCore) in your applications - see [documentation](https://github.com/chakra-core/ChakraCore/wiki/Embedding-ChakraCore) and [samples](https://aka.ms/chakracoresamples).

_A note about using ChakraCore_: ChakraCore is a JavaScript engine, it does not include the external APIs that are provided by a Web Browser or nodejs.  For example, DOM APIs like ```document.write()``` are additional APIs that are not provided by ChakraCore, when embedding ChakraCore in an application you will need to implement your own input and output APIs.  For debugging, in `ch` you can use ```print()``` to put text to the terminal.

Alternatively, if you are using the [vcpkg](https://github.com/Microsoft/vcpkg/) dependency manager you can download and install ChakraCore with CMake integration in a single command:
* vcpkg install chakracore

## Contribute

Contributions to ChakraCore are welcome.  Here is how you can contribute to ChakraCore:

* [Submit bugs](https://github.com/chakra-core/ChakraCore/issues) and help us verify fixes.
* [Submit pull requests](https://github.com/chakra-core/ChakraCore/pulls) for bug fixes and features and discuss existing proposals

Please refer to [Contribution Guidelines](CONTRIBUTING.md) for more details.

## License

Code licensed under the [MIT License](https://github.com/chakra-core/ChakraCore/blob/master/LICENSE.txt).

## Contact Us

If you have questions about ChakraCore, or you would like to reach out to us about an issue you're having or for development advice as you work on a ChakraCore issue, you can reach us as follows:

* Open an [issue](https://github.com/chakra-core/ChakraCore/issues/new) and prefix the issue title with [Question]. See [Question](https://github.com/chakra-core/ChakraCore/issues?q=label%3AQuestion) tag for already-opened questions.
* Discuss ChakraCore with the team and the community via the Discord link above
