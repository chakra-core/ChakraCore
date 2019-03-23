'use strict';

const fs = require('fs-extra');
const path = require('path');
const walkSync = require('walk-sync');
const stringUtil = require('ember-cli-string-utils');
const uniq = require('ember-cli-lodash-subset').uniq;
const SilentError = require('silent-error');
const sortPackageJson = require('sort-package-json');

let date = new Date();

const normalizeEntityName = require('ember-cli-normalize-entity-name');
const stringifyAndNormalize = require('../../lib/utilities/stringify-and-normalize');
const FileInfo = require('../../lib/models/file-info');

const replacers = {
  'package.json'(content) {
    return this.updatePackageJson(content);
  },
};

const description = 'The default blueprint for ember-cli addons.';
module.exports = {
  description,
  appBlueprintName: 'app',

  filesToRemove: [
    'tests/dummy/app/styles/.gitkeep',
    'tests/dummy/app/templates/.gitkeep',
    'tests/dummy/app/views/.gitkeep',
    'tests/dummy/public/.gitkeep',
    'Brocfile.js',
    'testem.json',
  ],

  updatePackageJson(content) {
    let contents = JSON.parse(content);

    contents.name = this.locals(this.options).addonName;
    contents.description = this.description;
    delete contents.private;
    contents.scripts = contents.scripts || {};
    contents.keywords = contents.keywords || [];
    contents.dependencies = contents.dependencies || {};
    contents.devDependencies = contents.devDependencies || {};

    // npm doesn't like it when we have something in both deps and devDeps
    // and dummy app still uses it when in deps
    contents.dependencies['ember-cli-babel'] = contents.devDependencies['ember-cli-babel'];
    delete contents.devDependencies['ember-cli-babel'];

    // 99% of addons don't need ember-data, make it opt-in instead
    delete contents.devDependencies['ember-data'];

    // 100% of addons don't need ember-cli-app-version, make it opt-in instead
    delete contents.devDependencies['ember-cli-app-version'];

    // addons should test _without_ jquery by default
    delete contents.devDependencies['@ember/jquery'];

    // ember-ajax depends on jquery, make it opt-in
    delete contents.devDependencies['ember-ajax'];

    if (contents.keywords.indexOf('ember-addon') === -1) {
      contents.keywords.push('ember-addon');
    }

    // add `ember-disable-prototype-extensions` to addons by default
    contents.devDependencies['ember-disable-prototype-extensions'] = '^1.1.3';

    // add `eslint-plugin-node` to addons by default
    contents.devDependencies['eslint-plugin-node'] = '^7.0.1';

    // add ember-try
    contents.devDependencies['ember-try'] = '^1.0.0';

    // add ember-source-channel-url
    contents.devDependencies['ember-source-channel-url'] = '^1.1.0';

    // add `ember-try` as `test:all` script in addons
    contents.scripts['test:all'] = 'ember try:each';

    contents['ember-addon'] = contents['ember-addon'] || {};
    contents['ember-addon'].configPath = 'tests/dummy/config';

    return stringifyAndNormalize(sortPackageJson(contents));
  },

  buildFileInfo(intoDir, templateVariables, file) {
    let mappedPath = this.mapFile(file, templateVariables);
    let options = {
      action: 'write',
      outputBasePath: path.normalize(intoDir),
      outputPath: path.join(intoDir, mappedPath),
      displayPath: path.normalize(mappedPath),
      inputPath: this.srcPath(file),
      templateVariables,
      ui: this.ui,
    };

    if (file in replacers) {
      options.replacer = replacers[file].bind(this);
    }

    return new FileInfo(options);
  },

  afterInstall() {
    let packagePath = path.join(this.path, 'files', 'package.json');
    let bowerPath = path.join(this.path, 'files', 'bower.json');

    [packagePath, bowerPath].forEach(filePath => {
      fs.removeSync(filePath);
    });
  },

  locals(options) {
    let entity = { name: 'dummy' };
    let rawName = entity.name;
    let name = stringUtil.dasherize(rawName);
    let namespace = stringUtil.classify(rawName);

    let addonEntity = options.entity;
    let addonRawName = addonEntity.name;
    let addonName = stringUtil.dasherize(addonRawName);
    let addonNamespace = stringUtil.classify(addonRawName);

    return {
      name,
      modulePrefix: name,
      namespace,
      addonName,
      addonNamespace,
      emberCLIVersion: require('../../package').version,
      year: date.getFullYear(),
      yarn: options.yarn,
      welcome: options.welcome,
      blueprint: 'addon',
    };
  },

  files() {
    let appFiles = this.lookupBlueprint(this.appBlueprintName).files();
    let addonFilesPath = this.filesPath(this.options);
    let addonFiles = walkSync(addonFilesPath);

    return uniq(appFiles.concat(addonFiles));
  },

  mapFile() {
    let result = this._super.mapFile.apply(this, arguments);
    return this.fileMapper(result);
  },

  fileMap: {
    '^app/.gitkeep': 'app/.gitkeep',
    '^app.*': 'tests/dummy/:path',
    '^config.*': 'tests/dummy/:path',
    '^public.*': 'tests/dummy/:path',

    '^addon-config/environment.js': 'config/environment.js',
    '^addon-config/ember-try.js': 'config/ember-try.js',

    '^npmignore': '.npmignore',
  },

  fileMapper(path) {
    for (let pattern in this.fileMap) {
      if ((new RegExp(pattern)).test(path)) {
        return this.fileMap[pattern].replace(':path', path);
      }
    }

    return path;
  },

  normalizeEntityName(entityName) {
    entityName = normalizeEntityName(entityName);

    if (this.project.isEmberCLIProject() && !this.project.isEmberCLIAddon()) {
      throw new SilentError('Generating an addon in an existing ember-cli project is not supported.');
    }

    return entityName;
  },

  srcPath(file) {
    let path = `${this.path}/files/${file}`;
    let superPath = `${this.lookupBlueprint(this.appBlueprintName).path}/files/${file}`;
    return fs.existsSync(path) ? path : superPath;
  },
};
