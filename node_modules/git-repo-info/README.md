## git-repo-info

Retrieves repo information without relying on the `git` command.

### Usage

```javascript
var getRepoInfo = require('git-repo-info');

var info = getRepoInfo();

info.branch               // current branch
info.sha                  // current sha
info.abbreviatedSha       // first 10 chars of the current sha
info.tag                  // tag for the current sha (or `null` if no tag exists)
info.lastTag              // tag for the closest tagged ancestor
                          //   (or `null` if no ancestor is tagged)
info.commitsSinceLastTag  // number of commits since the closest tagged ancestor
                          //   (`0` if this commit is tagged, or `Infinity` if no ancestor is tagged)
info.committer            // committer for the current sha
info.committerDate        // commit date for the current sha
info.author               // author for the current sha
info.authorDate           // authored date for the current sha
info.commitMessage        // commit message for the current sha
info.root                 // root directory for the Git repo or submodule
                          //   (if in a worktree, this is the directory containing the original copy)
info.commonGitDir         // directory containing Git metadata for this repo or submodule
                          //   (if in a worktree, this is the primary Git directory for the repo)
info.worktreeGitDir       // if in a worktree, the directory containing Git metadata specific to
                          //   this worktree; otherwise, this is the same as `commonGitDir`.
```

When called without any arguments, `git-repo-info` will automatically lookup upwards
into parent directories to find the first match with a `.git` folder.

If passed an argument, it will be assumed to be the path to the repo's `.git` folder
to inspect.
