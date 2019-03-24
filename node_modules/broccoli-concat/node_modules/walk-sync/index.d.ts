// Type definitions for "walk-sync"
// Definitions by: Adam Miller <https://github.com/amiller-gh>
export = WalkSync;

declare function WalkSync(path: string, options?: WalkSync.WalkSyncOptions): string[];

declare namespace WalkSync {
  export type WalkSyncOptions = {
    directories?: boolean,
    globs?: string[],
    ignore?: string[],
  };

  export class WalkSyncEntry {
    relativePath: string;
    basePath: string;
    fullPath: string;
    mode: number;
    size: number;
    mtime: Date;
    isDirectory(): boolean;
    static fromStat(relativePath: string, stat: any): WalkSyncEntry;
  }

  export function entries(path: string, options?: WalkSyncOptions): WalkSyncEntry[];
}
