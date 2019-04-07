/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
'use strict';

let Generator = require('yeoman-generator');
let yosay = require('yosay');

let path = require('path');
let validator = require('./validator');
let snippetConverter = require('./snippetConverter');
let themeConverter = require('./themeConverter');
let grammarConverter = require('./grammarConverter');
let env = require('./env');
let childProcess = require('child_process');
let chalk = require('chalk');
let sanitize = require("sanitize-filename");
let localization = require('./localization');

module.exports = class extends Generator {

    constructor(args, opts) {
        super(args, opts);
        this.option('extensionType', { type: String });
        this.option('extensionName', { type: String });
        this.option('extensionDescription', { type: String });
        this.option('extensionDisplayName', { type: String });

        this.option('extensionParam', { type: String });
        this.option('extensionParam2', { type: String });

        this.extensionConfig = Object.create(null);
        this.extensionConfig.installDependencies = false;
    }

    initializing() {

        // Welcome
        this.log(yosay('Welcome to the Visual Studio Code Extension generator!'));

        // evaluateEngineVersion
        let extensionConfig = this.extensionConfig;
        return env.getLatestVSCodeVersion().then(version => { extensionConfig.vsCodeEngine = version; });
    }

    prompting() {
        let generator = this;
        let prompts = {
            // Ask for extension type
            askForType: () => {
                let extensionType = generator.options['extensionType'];
                if (extensionType) {
                    let extensionTypes = ['colortheme', 'language', 'snippets', 'command-ts', 'command-js', 'extensionpack'];
                    if (extensionTypes.indexOf(extensionType) !== -1) {
                        generator.extensionConfig.type = 'ext-' + extensionType;
                    } else {
                        generator.log("Invalid extension type: " + extensionType + '. Possible types are :' + extensionTypes.join(', '));
                    }
                    return Promise.resolve();
                }

                return generator.prompt({
                    type: 'list',
                    name: 'type',
                    message: 'What type of extension do you want to create?',
                    choices: [{
                        name: 'New Extension (TypeScript)',
                        value: 'ext-command-ts'
                    },
                    {
                        name: 'New Extension (JavaScript)',
                        value: 'ext-command-js'
                    },
                    {
                        name: 'New Color Theme',
                        value: 'ext-colortheme'
                    },
                    {
                        name: 'New Language Support',
                        value: 'ext-language'
                    },
                    {
                        name: 'New Code Snippets',
                        value: 'ext-snippets'
                    },
                    {
                        name: 'New Keymap',
                        value: 'ext-keymap'
                    },
                    {
                        name: 'New Extension Pack',
                        value: 'ext-extensionpack'
                    },
                    {
                        name: 'New Language Pack (Localization)',
                        value: 'ext-localization'
                    }
                    ]
                }).then(typeAnswer => {
                    generator.extensionConfig.type = typeAnswer.type;
                });
            },

            askForThemeInfo: () => {
                if (generator.extensionConfig.type !== 'ext-colortheme') {
                    return Promise.resolve();
                }
                generator.extensionConfig.isCustomization = true;
                return generator.prompt({
                    type: 'list',
                    name: 'themeImportType',
                    message: 'Do you want to import or convert an existing TextMate color theme?',
                    choices: [
                        {
                            name: 'No, start fresh',
                            value: 'new'
                        },
                        {
                            name: 'Yes, import an existing theme but keep it as tmTheme file.',
                            value: 'import-keep'
                        },
                        {
                            name: 'Yes, import an existing theme and inline it in the Visual Studio Code color theme file.',
                            value: 'import-inline'
                        }
                    ]
                }).then(answer => {
                    let inline = true;
                    let type = answer.themeImportType;
                    if (type === 'import-keep' || type === 'import-inline') {
                        generator.log("Enter the location (URL (http, https) or file name) of the tmTheme file, e.g., http://www.monokai.nl/blog/wp-content/asdev/Monokai.tmTheme.");
                        return generator.prompt({
                            type: 'input',
                            name: 'themeURL',
                            message: 'URL or file name to import:'
                        }).then(urlAnswer => {
                            return themeConverter.convertTheme(urlAnswer.themeURL, generator.extensionConfig, type === 'import-inline', generator);
                        });
                    } else {
                        return themeConverter.convertTheme(null, generator.extensionConfig, false, generator);
                    }
                });
            },

            askForLanguageInfo: () => {
                if (generator.extensionConfig.type !== 'ext-language') {
                    return Promise.resolve();
                }

                generator.extensionConfig.isCustomization = true;
                generator.log("Enter the URL (http, https) or the file path of the tmLanguage grammar or press ENTER to start with a new grammar.");
                return generator.prompt({
                    type: 'input',
                    name: 'tmLanguageURL',
                    message: 'URL or file to import, or none for new:',
                }).then(urlAnswer => {
                    return grammarConverter.convertGrammar(urlAnswer.tmLanguageURL, generator.extensionConfig);
                });
            },

            askForSnippetsInfo: () => {
                if (generator.extensionConfig.type !== 'ext-snippets') {
                    return Promise.resolve();
                }

                generator.extensionConfig.isCustomization = true;
                let extensionParam = generator.options['extensionParam'];

                if (extensionParam) {
                    let count = snippetConverter.processSnippetFolder(extensionParam, generator);
                    if (count <= 0) {
                        generator.log('')
                    }
                    return Promise.resolve();
                }
                generator.log("Folder location that contains Text Mate (.tmSnippet) and Sublime snippets (.sublime-snippet) or press ENTER to start with a new snippet file.");

                let snippetPrompt = () => {
                    return generator.prompt({
                        type: 'input',
                        name: 'snippetPath',
                        message: 'Folder name for import or none for new:'
                    }).then(snippetAnswer => {
                        let count = 0;
                        let snippetPath = snippetAnswer.snippetPath;

                        if (typeof snippetPath === 'string' && snippetPath.length > 0) {
                            snippetConverter.processSnippetFolder(snippetPath, generator);
                        } else {
                            generator.extensionConfig.snippets = {};
                            generator.extensionConfig.languageId = null;
                        }

                        if (count < 0) {
                            return snippetPrompt();
                        }
                    });
                };
                return snippetPrompt();
            },


            askForLocalizationLanguageId: () => {
                return localization.askForLanguageId(generator);
            },

            askForLocalizationLanguageName: () => {
                return localization.askForLanguageName(generator);
            },

            askForLocalizedLocalizationLanguageName: () => {
                return localization.askForLocalizedLanguageName(generator);
            },

            askForExtensionPackInfo: () => {
                if (generator.extensionConfig.type !== 'ext-extensionpack') {
                    return Promise.resolve();
                }

                generator.extensionConfig.isCustomization = true;
                const defaultExtensionList = ['publisher.extensionName'];

                const getExtensionList = () =>
                    new Promise((resolve, reject) => {
                        childProcess.exec(
                            'code --list-extensions',
                            (error, stdout, stderr) => {
                                if (error) {
                                    generator.env.error(error);
                                } else {
                                    let out = stdout.trim();
                                    if (out.length > 0) {
                                        generator.extensionConfig.extensionList = out.split(/\s/);
                                    }
                                }
                                resolve();
                            }
                        );
                    });

                const extensionParam = generator.options['extensionParam'];
                if (extensionParam) {
                    switch (extensionParam.toString().trim().toLowerCase()) {
                        case 'n':
                            generator.extensionConfig.extensionList = defaultExtensionList;
                            return Promise.resolve();
                        case 'y':
                            return getExtensionList();
                    }
                }

                return generator.prompt({
                    type: 'confirm',
                    name: 'addExtensions',
                    message: 'Add the currently installed extensions to the extension pack?',
                    default: true
                }).then(addExtensionsAnswer => {
                    generator.extensionConfig.extensionList = defaultExtensionList;
                    if (addExtensionsAnswer.addExtensions) {
                        return getExtensionList();
                    }
                });
            },

            // Ask for extension display name ("displayName" in package.json)
            askForExtensionDisplayName: () => {
                let extensionDisplayName = generator.options['extensionDisplayName'];
                if (extensionDisplayName) {
                    generator.extensionConfig.displayName = extensionDisplayName;
                    return Promise.resolve();
                }

                return generator.prompt({
                    type: 'input',
                    name: 'displayName',
                    message: 'What\'s the name of your extension?',
                    default: generator.extensionConfig.displayName
                }).then(displayNameAnswer => {
                    generator.extensionConfig.displayName = displayNameAnswer.displayName;
                });
            },

            // Ask for extension id ("name" in package.json)
            askForExtensionId: () => {
                let extensionName = generator.options['extensionName'];
                if (extensionName) {
                    generator.extensionConfig.name = extensionName;
                    return Promise.resolve();
                }
                let def = generator.extensionConfig.name;
                if (!def && generator.extensionConfig.displayName) {
                    def = generator.extensionConfig.displayName.toLowerCase().replace(/[^a-z0-9]/g, '-');
                }
                if (!def) {
                    def == '';
                }

                return generator.prompt({
                    type: 'input',
                    name: 'name',
                    message: 'What\'s the identifier of your extension?',
                    default: def,
                    validate: validator.validateExtensionId
                }).then(nameAnswer => {
                    generator.extensionConfig.name = nameAnswer.name;
                });
            },

            // Ask for extension description
            askForExtensionDescription: () => {
                let extensionDescription = generator.options['extensionDescription'];
                if (extensionDescription) {
                    generator.extensionConfig.description = extensionDescription;
                    return Promise.resolve();
                }

                return generator.prompt({
                    type: 'input',
                    name: 'description',
                    message: 'What\'s the description of your extension?'
                }).then(descriptionAnswer => {
                    generator.extensionConfig.description = descriptionAnswer.description;
                });
            },

            askForJavaScriptInfo: () => {
                if (generator.extensionConfig.type !== 'ext-command-js') {
                    return Promise.resolve();
                }
                generator.extensionConfig.checkJavaScript = false;
                return generator.prompt({
                    type: 'confirm',
                    name: 'checkJavaScript',
                    message: 'Enable JavaScript type checking in \'jsconfig.json\'?',
                    default: false
                }).then(strictJavaScriptAnswer => {
                    generator.extensionConfig.checkJavaScript = strictJavaScriptAnswer.checkJavaScript;
                });
            },

            askForGit: () => {
                if (['ext-command-ts', 'ext-command-js'].indexOf(generator.extensionConfig.type) === -1) {
                    return Promise.resolve();
                }

                return generator.prompt({
                    type: 'confirm',
                    name: 'gitInit',
                    message: 'Initialize a git repository?',
                    default: true
                }).then(gitAnswer => {
                    generator.extensionConfig.gitInit = gitAnswer.gitInit;
                });
            },

            askForThemeName: () => {
                if (generator.extensionConfig.type !== 'ext-colortheme') {
                    return Promise.resolve();
                }

                return generator.prompt({
                    type: 'input',
                    name: 'themeName',
                    message: 'What\'s the name of your theme shown to the user?',
                    default: generator.extensionConfig.themeName,
                    validate: validator.validateNonEmpty
                }).then(nameAnswer => {
                    generator.extensionConfig.themeName = nameAnswer.themeName;
                });
            },

            askForBaseTheme: () => {
                if (generator.extensionConfig.type !== 'ext-colortheme') {
                    return Promise.resolve();
                }

                return generator.prompt({
                    type: 'list',
                    name: 'themeBase',
                    message: 'Select a base theme:',
                    choices: [{
                        name: "Dark",
                        value: "vs-dark"
                    },
                    {
                        name: "Light",
                        value: "vs"
                    },
                    {
                        name: "High Contrast",
                        value: "hc-black"
                    }
                    ]
                }).then(themeBase => {
                    generator.extensionConfig.themeBase = themeBase.themeBase;
                });
            },

            askForLanguageId: () => {
                if (generator.extensionConfig.type !== 'ext-language') {
                    return Promise.resolve();
                }

                generator.log('Enter the id of the language. The id is an identifier and is single, lower-case name such as \'php\', \'javascript\'');
                return generator.prompt({
                    type: 'input',
                    name: 'languageId',
                    message: 'Language id:',
                    default: generator.extensionConfig.languageId,
                }).then(idAnswer => {
                    generator.extensionConfig.languageId = idAnswer.languageId;
                });
            },

            askForLanguageName: () => {
                if (generator.extensionConfig.type !== 'ext-language') {
                    return Promise.resolve();
                }

                generator.log('Enter the name of the language. The name will be shown in the VS Code editor mode selector.');
                return generator.prompt({
                    type: 'input',
                    name: 'languageName',
                    message: 'Language name:',
                    default: generator.extensionConfig.languageName,
                }).then(nameAnswer => {
                    generator.extensionConfig.languageName = nameAnswer.languageName;
                });
            },

            askForLanguageExtensions: () => {
                if (generator.extensionConfig.type !== 'ext-language') {
                    return Promise.resolve();
                }

                generator.log('Enter the file extensions of the language. Use commas to separate multiple entries (e.g. .ruby, .rb)');
                return generator.prompt({
                    type: 'input',
                    name: 'languageExtensions',
                    message: 'File extensions:',
                    default: generator.extensionConfig.languageExtensions.join(', '),
                }).then(extAnswer => {
                    generator.extensionConfig.languageExtensions = extAnswer.languageExtensions.split(',').map(e => { return e.trim(); });
                });
            },

            askForLanguageScopeName: () => {
                if (generator.extensionConfig.type !== 'ext-language') {
                    return Promise.resolve();
                }
                generator.log('Enter the root scope name of the grammar (e.g. source.ruby)');
                return generator.prompt({
                    type: 'input',
                    name: 'languageScopeName',
                    message: 'Scope names:',
                    default: generator.extensionConfig.languageScopeName,
                }).then(extAnswer => {
                    generator.extensionConfig.languageScopeName = extAnswer.languageScopeName;
                });
            },

            askForSnippetLanguage: () => {
                if (generator.extensionConfig.type !== 'ext-snippets') {
                    return Promise.resolve();
                }
                let extensionParam2 = generator.options['extensionParam2'];

                if (extensionParam2) {
                    generator.extensionConfig.languageId = extensionParam2;
                    return Promise.resolve();
                }

                generator.log('Enter the language for which the snippets should appear. The id is an identifier and is single, lower-case name such as \'php\', \'javascript\'');
                return generator.prompt({
                    type: 'input',
                    name: 'languageId',
                    message: 'Language id:',
                    default: generator.extensionConfig.languageId
                }).then(idAnswer => {
                    generator.extensionConfig.languageId = idAnswer.languageId;
                });
            },

            askForPackageManager: () => {
                if (['ext-command-ts', 'ext-command-js', 'ext-localization'].indexOf(generator.extensionConfig.type) === -1) {
                    return Promise.resolve();
                }
                generator.extensionConfig.pkgManager = 'npm';
                return generator.prompt({
                    type: 'list',
                    name: 'pkgManager',
                    message: 'Which package manager to use?',
                    choices: [
                        {
                            name: 'npm',
                            value: 'npm'
                        },
                        {
                            name: 'yarn',
                            value: 'yarn'
                        }
                    ]
                }).then(pckgManagerAnswer => {
                    generator.extensionConfig.pkgManager = pckgManagerAnswer.pkgManager;
                });
            },
        };

        // run all prompts in sequence. Results can be ignored.
        let result = Promise.resolve();
        for (let taskName in prompts) {
            let prompt = prompts[taskName];
            result = result.then(_ => {
                return new Promise((s, r) => {
                    setTimeout(_ => prompt().then(s, r), 0); // set timeout is required, otherwise node hangs
                });
            })
        }
        return result;
    }
    // Write files
    writing() {
        this.sourceRoot(path.join(__dirname, './templates/' + this.extensionConfig.type));

        switch (this.extensionConfig.type) {
            case 'ext-colortheme':
                this._writingColorTheme();
                break;
            case 'ext-language':
                this._writingLanguage();
                break;
            case 'ext-snippets':
                this._writingSnippets();
                break;
            case 'ext-keymap':
                this._writingKeymaps();
                break;
            case 'ext-command-ts':
                this._writingCommandTs();
                break;
            case 'ext-command-js':
                this._writingCommandJs();
                break;
            case 'ext-extensionpack':
                this._writingExtensionPack();
                break;
            case 'ext-localization':
                localization.writingLocalizationExtension(this);
                break;
            default:
                //unknown project type
                break;
        }
    }

    // Write Color Theme Extension
    _writingExtensionPack() {
        let context = this.extensionConfig;

        this.fs.copy(this.sourceRoot() + '/vscode', context.name + '/.vscode');
        this.fs.copyTpl(this.sourceRoot() + '/package.json', context.name + '/package.json', context);
        this.fs.copyTpl(this.sourceRoot() + '/vsc-extension-quickstart.md', context.name + '/vsc-extension-quickstart.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/README.md', context.name + '/README.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/CHANGELOG.md', context.name + '/CHANGELOG.md', context);
        this.fs.copy(this.sourceRoot() + '/vscodeignore', context.name + '/.vscodeignore');
        if (this.extensionConfig.gitInit) {
            this.fs.copy(this.sourceRoot() + '/gitignore', context.name + '/.gitignore');
            this.fs.copy(this.sourceRoot() + '/gitattributes', context.name + '/.gitattributes');
        }
    }

    // Write Color Theme Extension
    _writingColorTheme() {

        let context = this.extensionConfig;
        if (context.tmThemeFileName) {
            this.fs.copyTpl(this.sourceRoot() + '/themes/theme.tmTheme', context.name + '/themes/' + context.tmThemeFileName, context);
        }
        context.themeFileName = sanitize(context.themeName + '-color-theme.json');
        if (context.themeContent) {
            context.themeContent.name = context.themeName;
            this.fs.copyTpl(this.sourceRoot() + '/themes/color-theme.json', context.name + '/themes/' + context.themeFileName, context);
        } else {
            if (context.themeBase === 'vs') {
                this.fs.copyTpl(this.sourceRoot() + '/themes/new-light-color-theme.json', context.name + '/themes/' + context.themeFileName, context);
            } else if (context.themeBase === 'hc') {
                this.fs.copyTpl(this.sourceRoot() + '/themes/new-hc-color-theme.json', context.name + '/themes/' + context.themeFileName, context);
            } else {
                this.fs.copyTpl(this.sourceRoot() + '/themes/new-dark-color-theme.json', context.name + '/themes/' + context.themeFileName, context);
            }
        }

        this.fs.copy(this.sourceRoot() + '/vscode', context.name + '/.vscode');
        this.fs.copyTpl(this.sourceRoot() + '/package.json', context.name + '/package.json', context);
        this.fs.copyTpl(this.sourceRoot() + '/vsc-extension-quickstart.md', context.name + '/vsc-extension-quickstart.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/README.md', context.name + '/README.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/CHANGELOG.md', context.name + '/CHANGELOG.md', context);
        this.fs.copy(this.sourceRoot() + '/vscodeignore', context.name + '/.vscodeignore');
        if (this.extensionConfig.gitInit) {
            this.fs.copy(this.sourceRoot() + '/gitignore', context.name + '/.gitignore');
            this.fs.copy(this.sourceRoot() + '/gitattributes', context.name + '/.gitattributes');
        }
    }

    // Write Language Extension
    _writingLanguage() {
        let context = this.extensionConfig;
        if (!context.languageContent) {
            context.languageFileName = sanitize(context.languageId + '.tmLanguage.json');

            this.fs.copyTpl(this.sourceRoot() + '/syntaxes/new.tmLanguage.json', context.name + '/syntaxes/' + context.languageFileName, context);
        } else {
            this.fs.copyTpl(this.sourceRoot() + '/syntaxes/language.tmLanguage', context.name + '/syntaxes/' + sanitize(context.languageFileName), context);
        }

        this.fs.copy(this.sourceRoot() + '/vscode', context.name + '/.vscode');
        this.fs.copyTpl(this.sourceRoot() + '/package.json', context.name + '/package.json', context);
        this.fs.copyTpl(this.sourceRoot() + '/README.md', context.name + '/README.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/CHANGELOG.md', context.name + '/CHANGELOG.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/vsc-extension-quickstart.md', context.name + '/vsc-extension-quickstart.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/language-configuration.json', context.name + '/language-configuration.json', context);
        this.fs.copy(this.sourceRoot() + '/vscodeignore', context.name + '/.vscodeignore');
        if (this.extensionConfig.gitInit) {
            this.fs.copy(this.sourceRoot() + '/gitignore', context.name + '/.gitignore');
            this.fs.copy(this.sourceRoot() + '/gitattributes', context.name + '/.gitattributes');
        }
    }

    // Write Snippets Extension
    _writingSnippets() {
        let context = this.extensionConfig;

        this.fs.copy(this.sourceRoot() + '/vscode', context.name + '/.vscode');
        this.fs.copyTpl(this.sourceRoot() + '/package.json', context.name + '/package.json', context);
        this.fs.copyTpl(this.sourceRoot() + '/vsc-extension-quickstart.md', context.name + '/vsc-extension-quickstart.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/README.md', context.name + '/README.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/CHANGELOG.md', context.name + '/CHANGELOG.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/snippets/snippets.json', context.name + '/snippets/snippets.json', context);
        this.fs.copy(this.sourceRoot() + '/vscodeignore', context.name + '/.vscodeignore');
        if (this.extensionConfig.gitInit) {
            this.fs.copy(this.sourceRoot() + '/gitignore', context.name + '/.gitignore');
            this.fs.copy(this.sourceRoot() + '/gitattributes', context.name + '/.gitattributes');
        }
    }

    // Write Snippets Extension
    _writingKeymaps() {
        let context = this.extensionConfig;

        this.fs.copy(this.sourceRoot() + '/vscode', context.name + '/.vscode');
        this.fs.copyTpl(this.sourceRoot() + '/package.json', context.name + '/package.json', context);
        this.fs.copyTpl(this.sourceRoot() + '/vsc-extension-quickstart.md', context.name + '/vsc-extension-quickstart.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/README.md', context.name + '/README.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/CHANGELOG.md', context.name + '/CHANGELOG.md', context);
        this.fs.copy(this.sourceRoot() + '/vscodeignore', context.name + '/.vscodeignore');
        if (this.extensionConfig.gitInit) {
            this.fs.copy(this.sourceRoot() + '/gitignore', context.name + '/.gitignore');
            this.fs.copy(this.sourceRoot() + '/gitattributes', context.name + '/.gitattributes');
        }
    }

    // Write Command Extension (TypeScript)
    _writingCommandTs() {
        let context = this.extensionConfig;

        this.fs.copy(this.sourceRoot() + '/vscode', context.name + '/.vscode');
        this.fs.copy(this.sourceRoot() + '/src/test', context.name + '/src/test');

        this.fs.copy(this.sourceRoot() + '/vscodeignore', context.name + '/.vscodeignore');
        if (this.extensionConfig.gitInit) {
            this.fs.copy(this.sourceRoot() + '/gitignore', context.name + '/.gitignore');
        }
        this.fs.copyTpl(this.sourceRoot() + '/README.md', context.name + '/README.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/CHANGELOG.md', context.name + '/CHANGELOG.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/vsc-extension-quickstart.md', context.name + '/vsc-extension-quickstart.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/tsconfig.json', context.name + '/tsconfig.json', context);

        this.fs.copyTpl(this.sourceRoot() + '/src/extension.ts', context.name + '/src/extension.ts', context);
        this.fs.copyTpl(this.sourceRoot() + '/package.json', context.name + '/package.json', context);

        this.fs.copy(this.sourceRoot() + '/tslint.json', context.name + '/tslint.json');

        this.extensionConfig.installDependencies = true;
    }

    // Write Command Extension (JavaScript)
    _writingCommandJs() {
        let context = this.extensionConfig;

        this.fs.copy(this.sourceRoot() + '/vscode', context.name + '/.vscode');
        this.fs.copy(this.sourceRoot() + '/test', context.name + '/test');

        this.fs.copy(this.sourceRoot() + '/vscodeignore', context.name + '/.vscodeignore');

        if (this.extensionConfig.gitInit) {
            this.fs.copy(this.sourceRoot() + '/gitignore', context.name + '/.gitignore');
        }

        this.fs.copyTpl(this.sourceRoot() + '/README.md', context.name + '/README.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/CHANGELOG.md', context.name + '/CHANGELOG.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/vsc-extension-quickstart.md', context.name + '/vsc-extension-quickstart.md', context);
        this.fs.copyTpl(this.sourceRoot() + '/jsconfig.json', context.name + '/jsconfig.json', context);

        this.fs.copyTpl(this.sourceRoot() + '/extension.js', context.name + '/extension.js', context);
        this.fs.copyTpl(this.sourceRoot() + '/package.json', context.name + '/package.json', context);
        this.fs.copyTpl(this.sourceRoot() + '/.eslintrc.json', context.name + '/.eslintrc.json', context);

        this.extensionConfig.installDependencies = true;
    }

    // Installation
    install() {
        process.chdir(this.extensionConfig.name);

        if (this.extensionConfig.installDependencies) {
            this.installDependencies({
                yarn: this.extensionConfig.pkgManager === 'yarn',
                npm: this.extensionConfig.pkgManager === 'npm',
                bower: false
            });
        }
    }

    // End
    end() {

        // Git init
        if (this.extensionConfig.gitInit) {
            this.spawnCommand('git', ['init', '--quiet']);
        }

        this.log('');
        this.log('Your extension ' + this.extensionConfig.name + ' has been created!');
        this.log('');
        this.log('To start editing with Visual Studio Code, use the following commands:');
        this.log('');
        this.log('     cd ' + this.extensionConfig.name);
        this.log('     code .');
        this.log('');
        this.log('Open vsc-extension-quickstart.md inside the new extension for further instructions');
        this.log('on how to modify, test and publish your extension.');
        this.log('');

        if (this.extensionConfig.type === 'ext-extensionpack') {
            this.log(chalk.default.yellow('Please review the "extensionPack" in the "package.json" before publishing the extension pack.'));
            this.log('');
        }

        this.log('For more information, also visit http://code.visualstudio.com and follow us @code.');
        this.log('\r\n');
    }
}
