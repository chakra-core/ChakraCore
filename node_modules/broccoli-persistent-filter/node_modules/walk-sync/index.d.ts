import { IMinimatch } from 'minimatch';
declare function walkSync(baseDir: string, inputOptions?: walkSync.Options | (string | IMinimatch)[]): string[];
export = walkSync;
declare namespace walkSync {
    function entries(baseDir: string, inputOptions?: Options | (string | IMinimatch)[]): Entry[];
    interface Options {
        includeBasePath?: boolean;
        globs?: (string | IMinimatch)[];
        ignore?: (string | IMinimatch)[];
        directories?: boolean;
    }
    class Entry {
        relativePath: string;
        basePath: string;
        mode: number;
        size: number;
        mtime: number;
        constructor(relativePath: string, basePath: string, mode: number, size: number, mtime: number);
        readonly fullPath: string;
        isDirectory(): boolean;
    }
}
