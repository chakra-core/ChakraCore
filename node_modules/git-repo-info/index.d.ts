declare function gitRepoInfo(): gitRepoInfo.GitRepoInfo;

declare namespace gitRepoInfo {
  export interface GitRepoInfo {
    /** The current branch */
    branch: string;
    /** SHA of the current commit */
    sha: string;
    /** The first 10 chars of the current SHA */
    abbreviatedSha: string;
    /** The tag for the current SHA (or `null` if no tag exists) */
    tag: string | null;
    /** Tag for the closest tagged ancestor (or `null` if no ancestor is tagged) */
    lastTag: string | null;
    /** The committer of the current SHA */
    committer: string;
    /** The commit date of the current SHA */
    committerDate: string;
    /** The author for the current SHA */
    author: string;
    /** The authored date for the current SHA */
    authorDate: string;
    /** The commit message for the current SHA */
    commitMessage: string;
    /**
     * The root directory for the Git repo or submodule.
     * If in a worktree, this is the directory containing the original copy, not the worktree.
     */
    root: string;
    /**
    * The directory containing Git metadata for this repo or submodule.
    * If in a worktree, this is the primary Git directory for the repo, not the worktree-specific one.
    */
    commonGitDir: string;
    /**
     * If in a worktree, the directory containing Git metadata specific to this worktree.
     * Otherwise, this is the same as `commonGitDir`.
     */
    worktreeGitDir: string;
  }
}

export = gitRepoInfo;
