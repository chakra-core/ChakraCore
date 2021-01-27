# Contributing Code

ChakraCore accepts bug fix pull requests. For a bug fix PR to be accepted, it must first have a tracking issue that has been marked approved. Your PR should link to the bug you are fixing. If you've submitted a PR for a bug, please post a comment in the bug to avoid duplication of effort.

ChakraCore also accepts new feature pull requests. For a feature-level PR to be accepted, it first needs to have design discussion. Design discussion can take one of two forms a) a feature request in the issue tracker that has been marked as approved or b) the PR must be accompanied by a full design spec and this spec is later approved in the open design discussion. Features are evaluated against their complexity, impact on other features, roadmap alignment, and maintainability.

These two blogs posts on contributing code to open source projects are a good reference: [Open Source Contribution Etiquette](http://tirania.org/blog/archive/2010/Dec-31.html) by Miguel de Icaza and [Don't "Push" Your Pull Requests](https://www.igvita.com/2011/12/19/dont-push-your-pull-requests/) by Ilya Grigorik.

## Security

If you believe you have found a security issue in ChakraCore 1.11, please share it with Microsoft privately following the guidance at the Microsoft [Security TechCenter](https://technet.microsoft.com/en-us/security/ff852094). Reporting it via this channel helps minimize risk to projects built with ChakraCore.

If you find a security issue in the Master branch of ChakraCore but not in 1.11 please join our discord server and private message one of the Core team members.

## Legal

You will need to complete a Contribution Agreement before your pull request can be accepted. This agreement testifies that you are granting us permission to use the source code you are submitting, and that this work is being submitted under appropriate license that we can use it.

You can read the agreement here: [Contribution Agreement](ContributionAgreement.md)

## Housekeeping

Your pull request should:

* Include a description of what your change intends to do
* Be a child commit of a reasonably recent commit in the master branch
* Pass all unit tests
* Have a clear commit message
* Include adequate tests
  * At least one test should fail in the absence of your non-test code changes. If your PR does not match this criteria, please specify why
  * Tests should include reasonable permutations of the target fix/change
  * Include baseline changes with your change

Submissions that have met these requirements will be will reviewed by a core contributor. Submissions must meet functional and performance expectations, including meeting requirements in scenarios for which the team doesn’t yet have open source tests. This means you may be asked to fix and resubmit your pull request against a new open test case if it fails one of these tests.

ChakraCore is an organically grown codebase. The consistency of style reflects this. For the most part, the team follows these [coding conventions](https://github.com/chakra-core/ChakraCore/wiki/Coding-Convention). Contributors should also follow them when making submissions. Otherwise, follow the general coding conventions adhered to in the code surrounding your changes. Pull requests that reformat the code will not be accepted.

## Running the tests

The unit tests can be run offline with following these steps:

### a) Windows

* Build `Chakra.Core.sln` for the version of ChakraCore you wish to test e.g. x64 Debug.
  * Specifically, running tests requires that `rl.exe`, `ch.exe`, and `ChakraCore.dll` be built.
* Call `test\runtests.cmd` and specify the build config

e.g.  `test\runtests.cmd -x64debug`

For full coverage, please run unit tests against debug and test for both x86 and x64:
* `test\runtests.cmd -x64debug`
* `test\runtests.cmd -x64test`
* `test\runtests.cmd -x86debug`
* `test\runtests.cmd -x86test`

`runtests.cmd` can take more switches that are useful for running a subset of tests.  Read the script file for more information.
`runtests.cmd` looks for the build output in the default build output folder `Build\VcBuild\bin`. If the build output path is changed from this default then use the `-bindir` switch to specify that path.

### b) Linux or macOS

Build the version of ChakraCore you wish to test - either a Debug or Test (RelWithDebugInfo) build. You will need the ChakraCore library and the `ch` application built.

If building with `cmake` you can then use the `make check` or `ninja check` command to run the test suite.
Alternatively you can directly run `test/runtests.py` you'll need to specify `-t` (Test build) or `-d` (Debug build).

`runtests.py` can take more switches that are useful for running a subset of tests.  Read the script file for more information.
`runtests.py` looks for the build output in the default build output folder `out/test/ch` or `out/debug/ch`. If you've used a different path then use `--binary=path` to specify it

## Issue Labels

 - [`help wanted`](https://github.com/chakra-core/ChakraCore/labels/help%20wanted): these issues are specifically well suited for outside contributors.
 - [`good first issue`](https://github.com/chakra-core/ChakraCore/labels/good%20first%20issue): these issues are small and appropriate for people who wish to familiarize themselves with GitHub pull requests and/or ChakraCore's contributor guidelines, build process, and running tests.  We're here to help you get started in open source.

You are welcome to work on issues that are not tagged with these labels. However, issues without these labels may be fairly complex, therefore please discuss with a core team member via comments on the issue before attempting to solve it.

For all issues you choose to work on please communicate on the issue that you are claiming it to avoid duplicated work.

To learn more about our GitHub labels see the [Label Glossary](https://github.com/Microsoft/ChakraCore/wiki/Label-Glossary) on our wiki page.
