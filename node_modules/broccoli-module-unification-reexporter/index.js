'use strict';

const Plugin = require('broccoli-plugin');
const fs = require('fs');
const path = require('path');
const walkSync = require('walk-sync');
const mkdirp = require('mkdirp');

module.exports = class ModuleUnificationReexporter extends Plugin {
  constructor(input, options) {
    super([input], {
      persistentOutput: true,
    });

    if (!options.namespace) {
      throw new Error('Must provide the `namespace` option to broccoli-module-unification-reexporter.');
    }

    this.namespace = options.namespace;

    this._hasRan = false;
  }

  _createReexport(inputPath, relativeOutputPath) {
    let extension = path.extname(inputPath);
    let importPath = inputPath.slice(0, -extension.length);
    let outputPath = path.join(this.outputPath, relativeOutputPath);

    try {
      fs.writeFileSync(outputPath, `export { default } from '${this.namespace}/src/${importPath}';`, 'utf-8');
    } catch (e) {
      if (e.code === 'ENOENT') {
        mkdirp.sync(path.dirname(outputPath));
        this._createReexport(inputPath, relativeOutputPath);
      } else {
        throw e;
      }
    }
  }

  build() {
    if (this._hasRan) { return; }

    let knownFiles = walkSync(this.inputPaths[0], {
      directories: false,
      globs: ['ui/components/*/{component,template}.*']
    });

    knownFiles.forEach((file) => {
      let componentRegex = /ui\/components\/([\w-]+)\/(component|template)/;
      let componentMatch = file.match(componentRegex);

      if(componentMatch) {
        let componentName = componentMatch[1];
        let type = componentMatch[2];

        let relativeOutputPath;
        if (type === 'component') {
          relativeOutputPath = `components/${componentName}.js`;

          if (componentName === 'main') {
            relativeOutputPath = `components/${this.namespace}.js`;
          }
        } else if (type === 'template') {
          relativeOutputPath = `templates/components/${componentName}.js`;

          if (componentName === 'main') {
            relativeOutputPath = `templates/components/${this.namespace}.js`;
          }
        }


        this._createReexport(file, relativeOutputPath);
      }
    });

    this._hasRan = true;
  }
}
