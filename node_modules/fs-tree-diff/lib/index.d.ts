// Type definitions for "fs-tree-diff"
// Definitions by: Adam Miller <https://github.com/amiller-gh>
import fs from "fs";

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
    static fromStat(relativePath: string, stat: fs.Stats): Entry;
  }

  export interface FSTreeOptions {
    entries: Entry[];
    sortAndExpand: boolean;
  }

  export type PartialFSTreeOptions = Partial<FSTreeOptions>;
}

declare class FSTree {
  constructor(options?: PartialFSTreeOptions): FSTree;
  calculatePatch(tree: FSTree, isEqual?: (a: FSTree, b: FSTree) => boolean): Patch[];
  calculateAndApplyPatch(tree: FSTree, inputDir: string, outputDir: string, delegate?: FSTreeDelegates): void;
  addEntries(entries: Entry[], options?: PartialFSTreeOptions): void;
  addPaths(paths: string[], options?: PartialFSTreeOptions): void;
  forEach(cb: (entry: FSEntry) => void, context?: any): void;
  static fromPaths(paths: string[]): FSTree;
  static fromEntries(entries: Entry[]): FSTree;
  static applyPatch(inputDir: string, outputDir: string, patch: Patch[]): void;
  static isEqual(a: FSTree, b: FSTree): boolean;
}

export = FSTree;