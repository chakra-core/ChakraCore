import Cache = require('./cache');
import CacheGroup = require('./cache-group');
declare function _getRealFilePath(realFilePathCache: Cache, filePath: string): string | null;
declare function _getRealDirectoryPath(realDirectoryPathCache: Cache, directoryPath: string): string | null;
declare function _findPackagePath(realFilePathCache: Cache, name: string, dir: string): string | null;
declare function resolvePackagePath(caches: CacheGroup, name?: string, dir?: string): string | null;
declare namespace resolvePackagePath {
    var _findPackagePath: typeof _findPackagePath;
    var _getRealFilePath: typeof _getRealFilePath;
    var _getRealDirectoryPath: typeof _getRealDirectoryPath;
}
export = resolvePackagePath;
