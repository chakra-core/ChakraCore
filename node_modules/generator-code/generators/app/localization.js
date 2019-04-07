/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
'use strict';


exports.askForLanguageId = (generator) => {
    if (generator.extensionConfig.type !== 'ext-localization') {
        return Promise.resolve();
    }
    generator.extensionConfig.isCustomization = true;
    generator.log("Enter the language identifier as used on transifex (e.g. bg, zh-Hant).");
    return generator.prompt({
        type: 'input',
        name: 'lpLanguageId',
        message: 'Language id:',
    }).then(answer => {
        generator.extensionConfig.lpLanguageId = answer.lpLanguageId;
        if (!generator.options['extensionName']) {
            generator.options['extensionName'] = "vscode-language-pack-" + answer.lpLanguageId;
        }
        return Promise.resolve();
    });
}

exports.askForLanguageName = (generator) => {
    if (generator.extensionConfig.type !== 'ext-localization') {
        return Promise.resolve();
    }
    generator.extensionConfig.isCustomization = true;
    generator.log("Enter the language name in English (e.g. 'Bulgarian', 'Dutch').");
    return generator.prompt({
        type: 'input',
        name: 'lpLanguageName',
        message: 'Language name:',
    }).then(answer => {
        generator.extensionConfig.lpLanguageName = answer.lpLanguageName;
        if (!generator.options['extensionDisplayName']) {
            generator.options['extensionDisplayName'] = answer.lpLanguageName + " Language Pack";
        }
        if (!generator.options['extensionDescription']) {
            generator.options['extensionDescription'] = "Language pack extension for " + answer.lpLanguageName;
        }
        return Promise.resolve();
    });
}

exports.askForLocalizedLanguageName = (generator) => {
    if (generator.extensionConfig.type !== 'ext-localization') {
        return Promise.resolve();
    }
    generator.extensionConfig.isCustomization = true;
    generator.log("Enter the language name in " + generator.extensionConfig.lpLanguageName);
    return generator.prompt({
        type: 'input',
        name: 'lpLocalizedLanguageName',
        message: 'Localized language name:',
    }).then(answer => {
        generator.extensionConfig.lpLocalizedLanguageName = answer.lpLocalizedLanguageName;
        return Promise.resolve();
    });
}

exports.writingLocalizationExtension = (generator) => {

    var context = generator.extensionConfig;

    generator.fs.copyTpl(generator.sourceRoot() + '/package.json', context.name + '/package.json', context);
    generator.fs.copyTpl(generator.sourceRoot() + '/vsc-extension-quickstart.md', context.name + '/vsc-extension-quickstart.md', context);
    generator.fs.copyTpl(generator.sourceRoot() + '/README.md', context.name + '/README.md', context);
    generator.fs.copyTpl(generator.sourceRoot() + '/CHANGELOG.md', context.name + '/CHANGELOG.md', context);
    generator.fs.copy(generator.sourceRoot() + '/vscodeignore', context.name + '/.vscodeignore');
    generator.fs.copy(generator.sourceRoot() + '/gitignore', context.name + '/.gitignore');
    generator.fs.copy(generator.sourceRoot() + '/gitattributes', context.name + '/.gitattributes');

    context.installDependencies = true;
}