# ChakraCore Project Future

## Future Aim

Future releases of ChakraCore are planned by a community team of volunteers (please see history section at the end of this document for how this has come about). We will aim to:

1. Prioritise ease of use for potential embedders of ChakraCore - these are intended as our primary users. We will aim to reflect this priority in any design decisions including by prioritising API stability and providing detailed and user friendly documentation.

1. Work towards standards compliance - we recognise that ChakraCore is significantly behind in some areas. We will work towards standards compliance including seeking to fix existing deviations and implement new standards as they arise. Any deviation from standards should be considered a bug.

1. Improve performance - we will aim to to improve performance when we can.

1. Invite community involvement - this project is being continued by volunteers and input will be appreciated from any other volunteers with relevant skills and availability. We will aim to make the project easy to get involved in - if as a potential contributor you see any obvious barriers to entry please raise an issue to discuss them.

1. Simplify the codebase and test suite - when time permits and where practicable we will endeavour to perform housekeeping actions such as removing dead code, appropriate refactoring, combining related tests and removing unnecessary layering. We will also aim to introduce a consistent style guide.

## Core Community Team

The core team currently planning to work on this are:

- Richard Lawrence (@rhuanjl) - team coordinator (by virtue of having the most free time)
- Kevin Smith (@zenparsing)
- Sergey Rubanov (@chicoxyzzy) - JS standards compliance
- Petr Penzin (@ppenzin) - WASM
- Alex Syrotenko (@Fly-Style)

## Development process

This community development will proceed on the ChakraCore master branch. At a future time this repository may be transferred out of the microsoft organisation within Github - though it will remain here to begin with.

We will periodically (time scale TBD) publish a release plan document listing planned enhancements and proposed time scale for the next release. Major decisions such as these release plans will require approval from all members of the Core Community Team.

We will also aim to triage and address bug reports as received, depending on the scale of these issues they may be fixed immediately or left until a future release.

Pull requests will be invited from anyone who wishes to contribute in addition to the Core Community Team. Any pull request will be required to pass all CI checks and be reviewed by a member of the Core Community Team (different to its author) prior to being merged. Once the review is cleared and the reviwer and author are happy with the work any member of the Core Community Team may merge the PR.

Additionally any bug fix PR must include a test case that it has fixed; and any new feature PR must include tests for that feature.

Further documentation and guidance will eventually be created on PR best practises including code style and how to author tests - in its absence please ask if unsure.

## Continuous Integration (CI)

We will continue to use the existing check in CI for the time being. However we will aim to migrate this to a platform managed by the community team before March 2021 - we will seek Microsoft's assistance with this migration.

We will additionally ask Google's project OSS-Fuzz to provide continuous fuzzing for our work.

## Security Issues

Until March 2021 we will liaise with microsoft on any security issues in case they relate to ChakraCore 1.11 and Chakra.dll - the exact method for this is to be determined. For the time being please report any ChakraCore security concerns to microsoft via the microsoft security tech centre in the first instance.

## Licensing and Copyright

The project will continue to be licensed under the MIT license. However the license and copyright statements will be updated to say "Copyright (c) Microsoft Corporation and ChakraCore Community contributors".

## History

Historically ChakraCore was the core part of the javascript engine used in Microsoft Edge and shipped as Chakra.dll with windows.

Microsoft Edge now uses Chromium including v8 and hence microsoft is discontinuing their support of Chakra and by extension ChakraCore. They intend to provide security fixes to the version 1.11 release of until 09/03/2021. But no other future development.

However rather than discarding ChakraCore Microsoft have asked if a community team would like to continue it as an open source project.
