# master

# 1.1.8

* expose `canSymlink` boolean via `require('symlink-or-copy').canSymlink`

# 1.1.7

* seems like the last release had an issue

# 1.1.6

* add work-around for fininky Windows Subsystem Linux path + symlink issues

# 1.1.5

* only package files required for usage

# 1.1.4

* Mitigate race, sometimes the tempfile already exists, ensure it has been
  removed before trying to write it.

# 1.1.3

* [BUGFIX] Instruct Win32 to suspend path parsing by prefixing the path with a \\?\.

# 1.1.2

* [BUGFIX] fix typo, causing grief on windows

# 1.1.1

* [BUGFIX] use realpath before creating a junction to ensure we always point to
  the concrete source, not another junction.

# 1.1.0

* Use junctions when possible on Windows if symlinks are not available

# 1.0.1

* Use symlinks on Windows if possible

# 1.0.0

* Initial release
