var eslintStandardConfig = require('eslint-config-standard');
var format = require('prettier-eslint');
var multimatch = require('multimatch');
var fsReadFile = require('fs-readfile-promise');
var fsWriteFile = require('fs-writefile-promise/lib/node7');

class PrettierEslintPlugin {
  constructor(
    {
      encoding = 'utf-8',
      extensions = ['.js', '.jsx'],
      ignore = ['**/node_modules/**'],
      // prettier-eslint API
      filePath,
      eslintConfig,
      prettierOptions,
      logLevel,
      eslintPath,
      prettierPath
    } = {}
  ) {
    // Encoding to use when reading / writing files
    this.encoding = encoding;
    // Only process files that match these patterns
    this.extensions = extensions;
    this.ignore = ignore;
    this.patterns = extensions
      .map(extension => `**/*${extension}`)
      .concat(ignore.map(ignore => `!${ignore}`));
    // Expose prettier-eslint API
    this.filePath = filePath;
    this.eslintConfig = eslintConfig || eslintStandardConfig;
    this.prettierOptions = prettierOptions;
    this.logLevel = logLevel;
    this.eslintPath = eslintPath;
    this.prettierPath = prettierPath;
  }

  apply(compiler) {
    compiler.plugin('emit', (compilation, callback) => {
      // Explore each chunk (build output):
      compilation.chunks.forEach(chunk => {
        // Explore each module within the chunk (built inputs):
        chunk.modules.forEach(module => {
          if (!module.fileDependencies) return;
          const matches = multimatch(module.fileDependencies, this.patterns);
          matches.forEach(this.processFilePath, this);
        });
      });
      callback();
    });
  }

  processFilePath(file) {
    return new Promise((resolve, reject) => {
      fsReadFile(file, { encoding: this.encoding })
        .then(buffer => buffer.toString())
        .catch(err => {
          reject(err.message);
        })
        .then(source => {
          const fmtOptions = {
            text: source,
            filePath: this.filePath,
            eslintConfig: this.eslintConfig,
            prettierOptions: this.prettierOptions,
            logLevel: this.logLevel,
            eslintPath: this.eslintPath,
            prettierPath: this.prettierPath
          };
          const formatted = format(fmtOptions);
          if (formatted !== source) {
            fsWriteFile(file, formatted, { encoding: this.encoding })
              .catch(err => reject(err.message))
              .then(() => {
                resolve('success!');
              });
          }
        })
        .catch(err => {
          reject(err.message);
        });
    });
  }
}

exports.PrettierEslintPlugin = PrettierEslintPlugin;
