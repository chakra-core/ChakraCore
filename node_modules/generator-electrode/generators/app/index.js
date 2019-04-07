"use strict";

/* eslint-disable arrow-parens */

const Generator = require("yeoman-generator");
const chalk = require("chalk");
const yosay = require("yosay");
const path = require("path");
const _ = require("lodash");
const extend = _.merge;
const parseAuthor = require("parse-author");
const githubUsername = require("github-username");
const Fs = require("fs");

const ExpressJS = "ExpressJS";
const HapiJS = "HapiJS";
const KoaJS = "KoaJS";
const pkg = require("../../package.json");

module.exports = class extends Generator {
  constructor(args, options) {
    super(args, options);

    this.isDemoApp = this.options.isDemoApp || false;

    if (!this.isDemoApp) {
      this.log(chalk.green("Yeoman Electrode App generator version"), pkg.version);
      this.log("Loaded from", chalk.magenta(path.dirname(require.resolve("../../package.json"))));
    }

    this.option("travis", {
      type: Boolean,
      required: false,
      defaults: true,
      desc: "Include travis config"
    });

    this.option("license", {
      type: Boolean,
      required: false,
      defaults: true,
      desc: "Include a license"
    });

    this.option("name", {
      type: String,
      required: false,
      desc: "Project name"
    });

    this.option("githubAccount", {
      type: String,
      required: false,
      desc: "GitHub username or organization"
    });

    this.option("projectRoot", {
      type: String,
      required: false,
      defaults: "server",
      desc: "Relative path to the project code root"
    });

    this.option("readme", {
      type: String,
      required: false,
      desc: "Content to insert in the README.md file"
    });

    this.option("cookiesModule", {
      type: String,
      required: false,
      default: "electrode-cookies",
      desc: "Universal cookies module name"
    });

    this.option("cookiesModuleSemver", {
      type: String,
      required: false,
      default: "^1.0.0",
      desc: "Universal cookies module semver"
    });

    this.option("uiConfigModule", {
      type: String,
      required: false,
      default: "electrode-ui-config",
      desc: "Universal UI config module name"
    });

    this.option("uiConfigModuleSemver", {
      type: String,
      required: false,
      default: "^1.1.2",
      desc: "Universal UI config module semver"
    });

    //Data should be passed to the generator using props
    this.props = this.options.props || {};
    //Flag to check if the OSS generator is being called as a subgenerator
    this.isExtended = this.props.isExtended || false;
  }

  initializing() {
    this.pkg = this.fs.readJSON(this.destinationPath("package.json"), {});

    if (this.pkg.keywords) {
      this.pkg.keywords = this.pkg.keywords.filter(x => x);
    }

    if (_.isObject(this.pkg.author)) {
      this.props.authorName = this.pkg.author.name;
      this.props.authorEmail = this.pkg.author.email;
      this.props.authorUrl = this.pkg.author.url;
      this.props.createDirectory = false;
      this.props.serverType = this.fs.exists(this.destinationPath("src/server/express-server.js"))
        ? ExpressJS
        : this.fs.exists(this.destinationPath("src/server/koa-server.js"))
        ? KoaJS
        : HapiJS;
      this.props.pwa = this.fs.exists(this.destinationPath("client/sw-registration.js"));
      this.props.autoSsr = this.fs.exists(this.destinationPath("server/plugins/autossr.js"));
      this.props.quoteType =
        _.get(pkg, "eslintConfig.rules.quotes", []).indexOf("single") >= 0 ? "'" : '"';
    } else if (_.isString(this.pkg.author)) {
      var info = parseAuthor(this.pkg.author);
      this.props.authorName = info.name;
      this.props.authorEmail = info.email;
      this.props.authorUrl = info.url;
    }
    // disable auto ssr (need to update to Hapi 17)
    this.props.autoSsr = false;
    // Pre set the default props from the information we have at this point
    this.props = extend(
      {
        name: this.pkg.name,
        description: this.pkg.description,
        version: this.pkg.version,
        homepage: this.pkg.homepage
      },
      this.props
    );
  }

  _cookiesModule() {
    if (this.config.get("serverType") === HapiJS) {
      return this.options.cookiesModule;
    }
    return undefined;
  }

  _isCwdEmpty() {
    return Fs.readdirSync(process.cwd()).length < 1;
  }

  _askFor() {
    if (this.pkg.name || this.options.name) {
      this.props.name = this.pkg.name || _.kebabCase(this.options.name);
    }

    var prompts = [
      {
        type: "input",
        name: "name",
        message: "Application Name",
        when: !this.props.name,
        default: path.basename(process.cwd())
      },
      {
        type: "input",
        name: "description",
        message: "Description",
        when: !this.props.description
      },
      {
        type: "input",
        name: "homepage",
        message: "Project homepage url",
        when: !this.props.homepage
      },
      {
        type: "list",
        name: "serverType",
        message: "Which framework for the server?",
        when: !this.props.serverType,
        choices: [HapiJS, ExpressJS, KoaJS],
        default: HapiJS
      },
      {
        type: "input",
        name: "authorName",
        message: "Author's Name",
        when: !this.props.authorName,
        default: this.user.git.name()
      },
      {
        type: "input",
        name: "authorEmail",
        message: "Author's Email",
        when: !this.props.authorEmail,
        default: this.user.git.email()
      },
      {
        type: "input",
        name: "authorUrl",
        message: "Author's Homepage",
        when: !this.props.authorUrl
      },
      {
        type: "input",
        name: "keywords",
        message: "Package keywords (comma to split)",
        when: _.isEmpty(this.pkg.keywords) && _.isEmpty(this.props.keywords),
        filter: function(words) {
          return words.split(/\s*,\s*/g).filter(x => x);
        }
      },
      {
        type: "confirm",
        name: "pwa",
        message: "Would you like to make a Progressive Web App?",
        when: this.props.pwa === undefined,
        default: false
      },
      // {
      //   type: "confirm",
      //   name: "autoSsr",
      //   message: "Support disabling server side rendering based on high load?",
      //   when: false,
      //   default: false
      // },
      {
        type: "list",
        name: "quoteType",
        message: "Use double quotes or single quotes?",
        choices: ['"', "'"],
        when: !this.props.quoteType,
        default: '"'
      },
      {
        type: "confirm",
        name: "createDirectory",
        message: "Would you like to create a new directory for your project?",
        when: this.props.createDirectory === undefined,
        default: this._isCwdEmpty() ? false : true
      },
      {
        type: "confirm",
        name: "flow",
        message: "Would you like to generate .flowconfig for flow usage?",
        when: this.props.flow === undefined,
        default: false
      }
    ];

    return this.prompt(prompts).then(props => {
      this.props = extend(this.props, props);

      if (this.props.createDirectory) {
        var newRoot = this.destinationPath() + "/" + _.kebabCase(_.deburr(this.props.name));
        this.destinationRoot(newRoot);
      }
      // Saving to storage after the correct destination root is set
      this.config.set("serverType", this.props.serverType);
    });
  }

  _askForGithubAccount() {
    if (this.props.githubAccount || this.options.githubAccount) {
      this.props.githubAccount = this.options.githubAccount;
      return Promise.resolve();
    }

    return githubUsername(this.props.authorEmail)
      .then(username => username, () => "")
      .then(username => {
        return this.prompt({
          name: "githubAccount",
          message: "GitHub username or organization",
          default: username
        }).then(prompt => {
          this.props.githubAccount = prompt.githubAccount;
        });
      });
  }

  prompting() {
    if (!this.isDemoApp) {
      this.log(yosay("Welcome to the phenomenal " + chalk.red("Electrode App") + " generator!"));
    }
    return this._askFor().then(this._askForGithubAccount.bind(this));
  }

  writing() {
    const isHapi = this.config.get("serverType") === HapiJS;
    const isExpress = this.config.get("serverType") === ExpressJS;
    const isPWA = this.props.pwa;
    const isAutoSSR = this.props.autoSsr;
    const isSingleQuote = this.props.quoteType === "'";
    const className = this.props.className;
    const packageName = this.props.packageName;
    const cookiesModule = this._cookiesModule();
    const cookiesModuleSemver = this.options.cookiesModuleSemver;
    const uiConfigModule = this.options.uiConfigModule;
    const uiConfigModuleSemver = this.options.uiConfigModuleSemver;

    let ignoreArray = [];
    if (isHapi) {
      ignoreArray.push("**/src/server/express-server.js");
      ignoreArray.push("**/src/server/koa-server.js");
    } else if (isExpress) {
      ignoreArray.push("**/src/server/koa-server.js");
    } else {
      ignoreArray.push("**/src/server/express-server.js");
    }

    // process default _package.js template
    const _pkgJs = "_package.js";

    this.fs.copyTpl(this.templatePath(_pkgJs), this.destinationPath(_pkgJs), {
      isHapi,
      isExpress,
      isPWA,
      isAutoSSR,
      isSingleQuote,
      projectName: this.props.name,
      cookiesModule,
      cookiesModuleSemver,
      uiConfigModule,
      uiConfigModuleSemver
    });

    const jsPkg = this.fs.read(this.destinationPath(_pkgJs)).toString();

    const defaultPkg = Function(`"use strict";` + jsPkg)();
    this.fs.delete(this.destinationPath(_pkgJs));

    // Re-read the content at this point because a composed generator might modify it.
    const currentPkg = this.fs.readJSON(this.destinationPath("package.json"), {});

    // delete certain empty fields in existing package.json
    ["name", "version", "description", "homepage", "main", "license"].forEach(x => {
      if (currentPkg.hasOwnProperty(x) && !currentPkg[x]) {
        delete currentPkg[x];
      }
    });

    const packageContents = {
      name: _.kebabCase(this.props.name),
      version: "0.0.1",
      description: this.props.description,
      homepage: this.props.homepage,
      author: {
        name: this.props.authorName,
        email: this.props.authorEmail,
        url: this.props.authorUrl
      },
      files: [this.options.projectRoot],
      main: path.posix.join("lib", this.options.projectRoot, "index.js"),
      keywords: []
    };

    if (this.isDemoApp) {
      packageContents.dependencies = {};
      packageContents.dependencies[this.props.packageName] =
        "../packages/" + this.props.packageName;
    }

    const updatePkg = _.defaultsDeep(currentPkg, packageContents);

    const pkg = extend({}, defaultPkg, updatePkg);

    // Combine the keywords
    if (this.props.keywords) {
      pkg.keywords = _.uniq(this.props.keywords.concat(pkg.keywords)).filter(x => x);
    }

    const sortDep = dep => {
      if (typeof pkg[dep] === "object") {
        pkg[dep] = _.pick(pkg[dep], Object.keys(pkg[dep]).sort());
      }
    };

    ["dependencies", "devDependencies", "peerDependencies", "optionalDependencies"].forEach(
      sortDep
    );

    // Let's extend package.json so we're not overwriting user previous fields
    this.fs.writeJSON(this.destinationPath("package.json"), pkg);

    const configSrcDir = this.props.serverType.toLowerCase();

    const rootConfigsToCopy = [["xclap.js"], [`config/${configSrcDir}`, "config"], ["test"]];

    if (this.props.flow) {
      rootConfigsToCopy.push([".flowconfig"]);
    }
    rootConfigsToCopy.push([".eslintrc.js"]);
    rootConfigsToCopy.forEach(f => {
      this.fs.copyTpl(
        this.templatePath(f[0]),
        this.destinationPath(f[1] || f[0]),
        { isSingleQuote },
        {},
        { globOptions: { dot: true } }
      );
    });

    if (this.isExtended) {
      this.fs.delete(this.destinationPath("config/default.js"));
    }

    //special handling for the server file
    this.fs.copyTpl(
      this.templatePath("src/server"),
      this.destinationPath("src/server"),
      { isHapi, isExpress },
      {},
      {
        globOptions: {
          dot: true,
          ignore: ignoreArray
        }
      }
    );

    this.fs.copyTpl(
      this.templatePath("src/client"),
      this.destinationPath("src/client"),
      { pwa: isPWA, cookiesModule, uiConfigModule },
      {}, // template options
      {
        // copy options
        globOptions: {
          dot: true,
          // Images are damaged by the template compiler
          ignore: ["**/client/images/**", !isPWA && "**/client/sw-registration.js"]
            .concat(
              this.isDemoApp && [
                "**/client/components/**",
                "**/client/actions/**",
                "**/client/reducers/**"
              ]
            )
            .filter(x => x)
        }
      }
    );

    this.fs.copy(this.templatePath("src/client/images"), this.destinationPath("src/client/images"));
  }

  default() {
    if (this.options.travis) {
      this.composeWith(require.resolve("generator-travis/generators/app"), {});
    }

    this.composeWith(require.resolve("../editorconfig"));

    if (!this.isDemoApp) {
      this.composeWith(
        require.resolve("../git"),
        _.pick(this.props, ["name", "githubAccount", "githubUrl"])
      );
    }

    if (this.options.license && !this.pkg.license) {
      this.composeWith(
        require.resolve("generator-license"),
        _.pick(this.props, ["name", "email", "website", "license"])
      );
    }

    if (!this.fs.exists(this.destinationPath("README.md"))) {
      this.composeWith(
        require.resolve("../readme"),
        _.pick(this.props, [
          "name",
          "description",
          "githubAccount",
          "authorName",
          "authorUrl",
          "content"
        ])
      );
    }

    const configProps = _.pick(this.props, ["name", "pwa", "serverType", "autoSsr"]);
    configProps._extended = this.isExtended;
    configProps.cookiesModule = this._cookiesModule();
    this.composeWith(require.resolve("../config"), configProps);

    if (!this.fs.exists(this.destinationPath("server/plugins/webapp"))) {
      this.composeWith(require.resolve("../webapp"), _.pick(this.props, ["pwa", "serverType"]));
    }
  }

  install() {
    if (!this.isExtended && !this.isDemoApp) {
      this.installDependencies({
        bower: false
      });
    }
  }

  end() {
    if (!this.isDemoApp) {
      const chdir = this.props.createDirectory
        ? "'cd " + _.kebabCase(_.deburr(this.props.name)) + "' then "
        : "";
      this.log(
        "\n" +
          chalk.green.underline("Your new Electrode application is ready!") +
          "\n" +
          "\nType " +
          chdir +
          "'clap dev' to start the server." +
          "\n"
      );
    }
  }
};
