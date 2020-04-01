# Plan for Version 1.12

## Timing

The last feature release of ChakraCore occurred on 29th June 2018 (version 1.10) since then there have been numerous security update releases and 1 non-security bug fix release (1.11). There has however been significant feature work performed since that date all of which is unreleased.

We would therefore like our first release to be done fairly quickly - however there are also some half complete pieces of work to finish before that as well as a variety of bugs introduced that should be fixed before that.

We will aim to do this first release by June to allow time for confirming the exact status of the project, completing unfinished work, performing testing and fixing bugs.

## Actions prior to release

We will endeavour to perform the below work prior to issuing this first community release in June. If time permits additional work may be done - conversely if time is too short the release may be produced with some of these actions not taken - in either event what has and has not been done will be explained.

1. Review major changes between 1.11 and master to generate an accurate set of release notes.

1. Review current master against JavaScript standards to identify major gaps including assessing against the tc39 list of stage 3 and stage 4 proposals and also testing with test262. This action may identify further actions to be taken before release including fixing small deviations from the standards.

1. Review changes to WASM proposals since last update to ChakraCore's WASM implementation, look for proposals that have became part of the MVP, and update opcodes when necessary on supported in-flight proposals.

1. Review all open bug reports and assess impact, fix where practicable.

1. Complete partially complete work including:

    - Generator, async function and async-generator function enhancements and enabling of async-iteration and jitting of all of these types of functions.

    - Implementation of Top level await

    - RegExp ES6 properties and unicode mode

    - further items may be added upon completion of reviews details above

1. Have Google OSS-Fuzz recommence fuzzing of the master branch and triage the first reports from them.

1. Determine release method - where will releases be posted? Who will post them? How will it be clear that this release is NOT supported by microsoft?
