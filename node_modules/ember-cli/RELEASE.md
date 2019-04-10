# Release Process

Although not very tricky, it is quite easy to deploy something that doesn't
quite work as expected. The following steps help you navigate through some of
the release gotchas and will hopefully result in a successful release.

---

## Preparation

### Communication

- Ensure that homu isn't presently processing any PRs.
- Post a note in [#dev-ember-cli](https://discordapp.com/channels/480462759797063690/480501885837770763) letting us know you're doing a release.

> I'm starting an Ember CLI release. Please hold off merging PRs and pushing new code!

### Environment

Make sure that you're running the most recent stable `node` and bundled `npm`.

```sh
node --version
npm --version
```

## Branching

If you're planning to release a stable/bugfix version _and_ a beta, make sure to release the beta _after_ the stable version.

```sh
# Fetch changes from GitHub
git fetch origin
```

Once you're done following these instructions make sure that you push your `master`, `beta`, and `release` branches back to GitHub.


### Promoting beta to stable

Follow these steps if you're releasing a new minor or major version (e.g. from `v2.5.0` to `v2.6.0`):

```sh
# Switch to "release" branch and reset it to "origin/beta"
git checkout -B release --track origin/beta

# Merge any unmerged changes from "origin/release" back in
git merge origin/release

# ... do the stable release ...

# Switch to "beta" branch and reset it to "origin/beta"
git checkout -B beta --track origin/beta

# Merge the new stable release into the "beta" branch
git merge vX.Y.0
```

### Stable bugfix release

Follow these steps if you're releasing a bugfix for a stable version (e.g. from `v2.5.0` to `v2.5.1`)

```sh
# Switch to "release" branch and reset it to "origin/release"
git checkout -B release --track origin/release

# ... do the stable release ...

# Switch to "beta" branch and reset it to "origin/beta"
git checkout -B beta --track origin/beta

# Merge the new stable release into the "beta" branch
git merge vX.Y.Z
```


### Promoting canary to beta

Follow these steps if you're releasing a beta version after a new minor/major release (e.g. `v2.7.0-beta.1`)

```sh
# Switch to "beta" branch and reset it to "origin/master"
git checkout -B beta --track origin/master

# Merge any unmerged changes from "origin/beta" back in
git merge origin/beta

# ... do the beta release ...

# Switch to "master" branch and reset it to "origin/master"
git checkout -B master --track origin/master

# Merge the new beta release into the "master" branch
git merge vX.Y.0-beta.1

# Push back upstream.
git push origin
```


### Incremental beta release

Follow these steps if you're releasing a beta version following another beta (e.g. `v2.7.0-beta.N` with `N != 1`)

```sh
# Switch to "beta" branch and reset it to "origin/beta"
git checkout -B beta --track origin/beta

# ... do the beta release ...

# Switch to "master" branch and reset it to "origin/master"
git checkout -B master --track origin/master

# Merge the new beta release into the "master" branch
git merge vX.Y.0-beta.N
```

## Release

### Setup

* Update Ember and Ember Data versions.
  * `blueprints/app/files/package.json`
  * if you're releasing a new minor or major version:
    * `tests/fixtures/addon/npm/package.json`
    * `tests/fixtures/addon/yarn/package.json`
    * `tests/fixtures/app/npm/package.json`
    * `tests/fixtures/app/yarn/package.json`
* generate changelog
  * if on master branch
    * run `./dev/changelog`
  * if this is a beta
    * run `./dev/changelog beta`
  * if this is a release
    * run `./dev/changelog release`
* prepend changelog output to `CHANGELOG.md`
* edit changelog output to be as user-friendly as possible (drop [INTERNAL] changes, non-code changes, etc.)
* replace any "ember-cli" user references in the changelog to whomever made the change
* bump `package.json` version
* don't commit these changes until later
* run `./dev/prepare-release`
* the `du` command should give you ballpark 190K as of `3.8.0`

### Test

* `cd to/someplace/to/test/`
* ensure `ember version` is the newly packaged version

```shell
# ensure new project generation works
ember new --skip-npm my-cool-test-project
cd my-cool-test-project

# link your local ember-cli
npm link ember-cli

# install other deps
npm install

# test the server
ember serve
```

* test other things like generators and live-reload
* generate an http mock `ember g http-mock my-http-mock`
* test upgrades of other apps
* if releasing using Windows, check that it works on a Linux VM
  * we are checking if any Windows line endings snuck in, because they won't work on Unix
* if releasing using Unix, you are set, Windows can read your line endings

### Update Artifacts

* if normal release
  * run `./dev/add-to-output-repos.sh`
* if incremental beta release
  * run `./dev/add-to-output-repos.sh beta`
* if promoting canary to beta
  * run `./dev/add-to-output-repos.sh beta fork`
* copy the [`ember new` diff] and [`ember addon` diff] lines from the previous
  release changelog and paste into the current, then update the url with the
  newer tags

### Publish

If everything went well, publish. Please note, we must have an extremely low
tolerance for quirks and failures. We do not want our users to endure any extra
pain.

1. go back to ember-cli directory
* `git add` the modified `package.json` and `CHANGELOG.md`
* Commit the changes `git commit -m "Release vx.y.z"` and push `git push`
* `git tag "vx.y.z"`
* `git push origin <vx.y.z>`
* publish to npm
  * if normal release
    * `npm publish ./ember-cli-<version>.tgz`
  * if beta release
    * `npm publish ./ember-cli-<version>.tgz --tag beta`


### Test Again

Test published version

1. `npm uninstall -g ember-cli`
* `npm cache clear`
* install
  * if normal release
    * `npm install -g ember-cli`
  * if beta release
    * `npm install -g ember-cli@beta`
* ensure version is as expected `ember version`
* ensure new project generates
* ensure old project upgrades nicely


## Promote

Announce release!

### Create a new release on GitHub

* [Draft a new release.](https://github.com/ember-cli/ember-cli/releases/new)
  * enter the new version number as the tag prefixed with `v` e.g. (`v0.1.12`)
  * Make sure to include the links for diffs between the versions.
  * for release title choose a great name, no pressure
  * in the description paste the following upgrade instructions

```
## Setup

`npm install -g ember-cli@NEW_VERSION_NUMBER` -- Install new global ember-cli

## Project Update

1. `rm -rf node_modules dist tmp` -- Delete temporary development folders.
2. `npm install -g ember-cli-update` -- Install Ember CLI update tool globally.
3. Run `ember-cli-update --to NEW_VERSION_NUMBER` -- This will update your app or addon to the latest ember-cli release. You will probably encounter merge conflicts that you should resolve in your normal git workflow.
4. Run `ember-cli-update --run-codemods` -- This will let you pick codemods to run against your project, to ensure you are using the latest patterns and platform features.

## Changelog

INSERT_CHANGELOG_HERE
```

  * Fill in the version number for `NEW_VERSION_NUMBER`
  * Replace `INSERT_CHANGELOG_HERE` with the new content from `CHANGELOG.md`
  * Attach the `ember-cli-<version>.tgz` from above
  * Check the "Pre-release" checkbox for beta releases.
  * Publish the release.

### Twitter

> Ember CLI vX.Y.Z "Release name goes here." released!
https://github.com/ember-cli/ember-cli/releases/tag/vX.Y.Z
\#emberjs

### Discord

Grab a link to your tweet and post in:
* [#news-and-announcements](https://discordapp.com/channels/480462759797063690/480499624663056390)
* [#dev-ember-cli](https://discordapp.com/channels/480462759797063690/480501885837770763)
* [#ember-cli](https://discordapp.com/channels/480462759797063690/486548111221719040)


## Troubleshooting

* if a few mins after release you notice an issue, you can unpublish
  * `npm unpublish ember-cli@<version>` (`npm unpublish` is write-only, that is you can unpublish but cannot push `ember-cli` with the same version, you have to bump `version` in `package.json`)
* if it is completely broken, feel free to unpublish a few hours later or the next morning, even if you don't have time to immediately rerelease
