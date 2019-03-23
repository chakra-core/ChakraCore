/**
 * Options supported by tree-sync. This is current a whitelist of options supported by walk-sync
 * and is passed verbatim as provided.
 */
export type TreeSyncOptions = {
  /**
   * An array of globs. Only files and directories that match at least one of the provided globs
   * will be returned.
   * If using this option, it's important to ensure that you've included parent directories of files
   * you add, otherwise you might run into an ENOENT error!
   */
  globs?: string[],

  /**
   * An array of globs. Files and directories that match at least one of the provided globs will be
   * pruned while searching.
   */
  ignore?: string[],
}

/**
 * A result from a patch operation.
 * The first item in the tuple is the operation that was run - can be one of "create", "change", "mkdir",
 * "unlink", or "rmdir".
 * The second item is the path that was affected, relative to the root input/output directory.
 */
export type TreeSyncResult = [string, string];

/**
 * A module for repeated efficient synchronizing two directories.
 * Use the `sync()` method to run a sync from the input path to the output path.
 * @see https://github.com/stefanpenner/tree-sync
 */
export default class TreeSync {
  /**
   * Initializes a new TreeSync instance. This instance is used to keep an output directory in sync with its
   * input directory. When using the same instance to keep two directories in sync over multiple sync operations,
   * TreeSync will figure out the differences between the two folders and only sync those changes specifically,
   * which in many cases will improve performance.
   *
   * @param inputPath    The originating path to sync contents from.
   * @param outputPath   The resulting path to sync contents to.
   * @param treeSyncOpts Options on how to run tree-sync. Use this to selectively choose or ignore directories
   *                     to sync.
   */
  constructor(inputPath: string, outputPath: string, treeSyncOpts?: TreeSyncOptions);

  /**
   * Syncs the input directory to the output directory. File I/O is done synchronously.
   */
  sync(): TreeSyncResult[];
}
