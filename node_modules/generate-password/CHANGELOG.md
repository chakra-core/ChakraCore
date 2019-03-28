# 1.4.1 / 2018-10-28
Bug fix to `randomNumber()` function that reduced entropy, resulting in a bias towards letters, generally.

#### Notable Changes
- [`21a12d0fd`](https://github.com/brendanashworth/generate-password/commit/21a12d0fd47c1b8f63a310da052cedf29ba5c00d) - fixed randomNumber's frontwards bias (Starkteetje)

# 1.4.0 / 2018-1-20
Release that includes TypeScript definitions.

#### Notable Changes
- [`ef2ded619`](https://github.com/brendanashworth/generate-password/commit/ef2ded6195ef72ee364172d1ff2c4d107ffe2821) - typescript definition file (Carlos Gonzales)

# 1.3.0 / 2016-12-28
Release with two new features.

One can now pass `{ 'exclude': 'abc' }` to exclude various characters from password
generation. This can be used to blacklist certain symbols, remove alike characters,
etc by giving a string with all the characters to be removed.

The options parameter is now optional — it is now unnecessary to pass an empty object
when the defaults are desired.

#### Notable Changes
- [`38d4ae0b8`](https://github.com/brendanashworth/generate-password/commit/38d4ae0b8d27db7f3fef897db30143aedc530f1f) - add `exclude` option to restrict passwords (Michael Kimpton)
- [`d16c95369`](https://github.com/brendanashworth/generate-password/commit/d16c9536914df599751589f6721ec506cdfbd95c) - Accept generate() when called without the options parameter (Alexandre Perrin)

# 1.2.0 / 2016-9-25
Release with a new feature and various improvements.

`strict` is now an option that can be passed to password generation. When this is `true`,
each other option will be required — for example, if you generate a password with numbers,
lowercase letters, and uppercase letters, the password will have *at least one* number, one
lowercase letter, and one uppercase letter.

#### Notable Changes
- [`98f923c0c`](https://github.com/brendanashworth/generate-password/commit/98f923c0c9af4fd7f3010b3d40c85233de437eef) - fix strict password generation (Brendan Ashworth)
- [`c69e2ef6b`](https://github.com/brendanashworth/generate-password/commit/c69e2ef6bb876ba58e6b27bc1f460d6ff18cb877) - adds eslint (Brendan Ashworth)
- [`a798e846c`](https://github.com/brendanashworth/generate-password/commit/a798e846c70210f6c88cbc062d8f1d4f8b808f8b) - add code coverage (Brendan Ashworth)
- [`aa5e13edf`](https://github.com/brendanashworth/generate-password/commit/aa5e13edfee35852fb3a31414cbf2e8fa101e257) - Adds strict password generation (Algimantas Krasauskas)

# 1.1.1 / 2014-12-23
- Add `excludeSimilarCharacters` option
