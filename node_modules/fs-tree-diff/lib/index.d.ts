// Type definitions for "fs-tree-diff"
// Definitions by: Adam Miller <https://github.com/amiller-gh>
import { Stats } from "fs";

declare namespace FSTree {
  export type ChangeType = "unlink" | "rmdir" | "mkdir" | "create" | "change";
  export type Patch = [ChangeType, string, FSTreeEntry];
  export type FSTreeDelegates = {
    [P in ChangeType]?: (inputPath: string, outputPath: string, relativePath: string) => void
  };

  export interface Entry {
    relativePath: string;
    mode: number;
    size: number;
    mtime: Date;
    isDirectory(): boolean;
  }

  export class FSTreeEntry implements Entry {
    relativePath: string;
    mode: number;
    size: number;
    mtime: Date;
    isDirectory(): boolean;
    isFile(): boolean;
    static fromStat(relativePath: string, stat: Stats): Entry;
  }

  export interface FSTreeOptions {
    entries: Entry[];
    sortAndExpand: boolean;
  }

  export type PartialFSTreeOptions = Partial<FSTreeOptions>;
}

declare class FSTree {
  constructor(options?: FSTree.PartialFSTreeOptions);
  calculatePatch(tree: FSTree, isEqual?: (a: FSTree.Entry, b: FSTree.Entry) => boolean): FSTree.Patch[];
  calculateAndApplyPatch(tree: FSTree, inputDir: string, outputDir: string, delegate?: FSTree.FSTreeDelegates): void;
  addEntries(entries: FSTree.Entry[], options?: FSTree.PartialFSTreeOptions): void;
  addPaths(paths: string[], options?: FSTree.PartialFSTreeOptions): void;
  forEach(cb: (entry: FSTree.Entry) => void, context?: any): void;
  static fromPaths(paths: string[]): FSTree;
  static fromEntries(entries: FSTree.Entry[]): FSTree;
  static applyPatch(inputDir: string, outputDir: string, patch: FSTree.Patch[]): void;
  static defaultIsEqual(a: FSTree.Entry, b: FSTree.Entry): boolean;
}

export = FSTree;
