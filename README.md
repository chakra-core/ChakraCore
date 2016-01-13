# ChakraCore with Time-Travel Debugging

This is a branch of [ChakraCore](https://github.com/Microsoft/ChakraCore) to support work on adding Time-Travel Debugging functionality to the Chakra JavaScript engine. The time-travel functionality adds reverse variations of the usual step forward operations provided by a debugger allowing a developer to rewind time to see the exact sequence of statements and program values leading to an error instead of going through the iterative process of re-setting breakpoints and reproducing the bug to narrow in on the source of the error.

The technique used to implement the time-travel functionality is described in an OOPSLA 2014 [paper](http://research.microsoft.com/en-us/um/people/marron/selectpubs/TimeTravelDbg.pdf) and is based on a record/replay with state snapshots design. The current code contains snapshot/restore code for the JavaScript program state, code for record/replay of events from the JsRT host, and new JsRT debugger API's that support time-travel debugging operations. Work is ongoing to complete the snapshot/inflate code and the addition of record/replay hooks to the JsRT API. 

A demo video of an earlier experimental prototype of the the idea running with Node.js can be seen in the OSS announcement [video](https://youtu.be/1bfDB3YPHFI). You can stay up-to-date on progress by following the [MSEdge developer blog](http://blogs.windows.com/msedgedev/) or [@ChakraCore](https://twitter.com/ChakraCore) on Twitter.

## Building TTD ChakraCore

You can build ChakraCore on Windows 7 SP1 or above with either Visual Studio 2013 or 2015 with C++ support installed.  Once you have Visual Studio installed:

* Clone ChakraCore through ```git clone https://github.com/Microsoft/ChakraCore.git``` and switch to the TimeTravelDebugging branch
* Open `Build\Chakra.Core.sln` in Visual Studio
* Build Solution (x86 with debug or test configurations)

More details in [Building ChakraCore](https://github.com/Microsoft/ChakraCore/wiki/Building-ChakraCore).

## Using TTD ChakraCore

At this point you can experiment with basic time-travel functionality in TTD ChakraCore by using the *ch.exe* binary. This app is a lightweight hosting of JsRT where you can run small applications.  After building, you can find this binary in: `Build\VcBuild\bin\[platform+output]`  (eg. `Build\VcBuild\bin\x86_debug`). Using the ch.exe application you can record the execution of a program execution using the command `ch.exe -TTRecord:[logDirectoryPath] yourSrc.js` (or use `-TTRecord:!default` for a temp default location). To time-travel debug this execution you can launch ch.exe with the command line flag `ch.exe -TTDebug:[logDirectoryPath]` (or `-TTDebug:!default` for the temp default location) which starts the time-travel execution and launches a simple command line debugger to experiemnt with. You can find more usage examples in the unit-test directory `test\TTBasic\`,  `test\TTExecuteBasic\`, and  `test\TTDebuggerBasic\`.

## Documentation

* [OOPSLA 2014  research paper](http://research.microsoft.com/en-us/um/people/marron/selectpubs/TimeTravelDbg.pdf)
* [ChakraCore Architecture](https://github.com/Microsoft/ChakraCore/wiki/Architecture-Overview)
* [Contribution guidelines](CONTRIBUTING.md)
* [Blogs, talks and other resources](https://github.com/Microsoft/ChakraCore/wiki/Resources)

## Contribute

Contributions to ChakraCore TTD (and ChakraCore) are welcome.  Here is how you can contribute:

* Submit pull requests to help complete TTD implementation or add new features
* Submit bugs to [ChakraCore](https://github.com/Microsoft/ChakraCore/issues) and help verify fixes
* Chat about [@ChakraCore](https://twitter.com/ChakraCore) on Twitter

Please refer to [Contribution guidelines](CONTRIBUTING.md) for more details.

As this work started as an academic research project in the [RiSE](http://research.microsoft.com/en-us/groups/rise/default.aspx) group at [Microsoft Research](http://research.microsoft.com/en-us/default.aspx) we are also excited to support continued research and experimentation based on TTD ChakraCore code.

## Roadmap
For details on our planned features and future direction please refer to our [roadmap](https://github.com/Microsoft/ChakraCore/wiki/Roadmap).

## Contact us
For questions about ChakraCore, please open an [issue](https://github.com/Microsoft/ChakraCore/issues/new) and prefix the issue title with [Question]. 
