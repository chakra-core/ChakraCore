# Welcome to the <%= lpLanguageName %> language pack

## What's in the folder

* `package.json` - the manifest file, defining the name and description of the localization extension. It also contains the `localizations` contribution point that defines the language id:

  ```json
        "contributes": {
            "localization": [{
                "languageId": <%- JSON.stringify(lpLanguageId) %>,
                "languageName": <%- JSON.stringify(lpLanguageName) %>,
                "localizedLanguageName": <%- JSON.stringify(lpLocalizedLanguageName) %>
            }]
        }
  ```

* `translations` - the folder containing the translation strings

To create/update the `translations` folder with the latest strings from transifex, follow these steps:

* Get an API token from https://www.transifex.com/user/settings/api. The token needs to have access to the `vscode-editor`, `vscode-workbench` and `vscode-extensions` projects.
* Set the API token to the environment variable `TRANSIFEX_API_TOKEN`.
* Check out the `master` branch of the [VS Code repository](https://github.com/Microsoft/vscode).
  * Preferably, place the VSCode repo next to the language pack extension (so both have the same parent folder).
  * `cd vscode` and run `yarn` to initialize the VS Code repo. More information on how to build VS Code you can find [here](https://github.com/Microsoft/vscode/wiki/How-to-Contribute).
  * If the language pack extension is placed next to the VS Code repository: `npm run update-localization-extension <%- lpLanguageId %>`
  * Otherwise: `npm run update-localization-extension {path_to_lang_pack_ext}`
* This will download translation files to the folder `translations`
* `package.json` will be modified and add a `translations` property with paths to each extension's translations will be added.

  ```json
  "contributes": {
    "localizations": [
        {
            "languageId": <%- JSON.stringify(lpLanguageId) %>,
            "languageName": <%- JSON.stringify(lpLanguageName) %>,
            "localizedLanguageName": <%- JSON.stringify(lpLocalizedLanguageName) %>,
            "translations": [
                {
                    "id": "vscode",
                    "path": "./translations/main.i18n.json"
                },
                {
                    "id": "vscode.emmet",
                    "path": "./translations/extensions/emmet.i18n.json"
                },
                {
                    "id": "vscode.css",
                    "path": "./translations/extensions/css.i18n.json"
                },
                {
                    "id": "vscode.grunt",
                    "path": "./translations/extensions/grunt.i18n.json"
                },
                {
                    "id": "vscode.git",
                    "path": "./translations/extensions/git.i18n.json"
                },
                {
                    "id": "vscode.gulp",
                    "path": "./translations/extensions/gulp.i18n.json"
                },
                {
                    "id": "vscode.jake",
                    "path": "./translations/extensions/jake.i18n.json"
                },
                {
                    "id": "vscode.html",
                    "path": "./translations/extensions/html.i18n.json"
                },
                {
                    "id": "vscode.json",
                    "path": "./translations/extensions/json.i18n.json"
                },
                {
                    "id": "vscode.markdown",
                    "path": "./translations/extensions/markdown.i18n.json"
                },
                {
                    "id": "vscode.merge-conflict",
                    "path": "./translations/extensions/merge-conflict.i18n.json"
                },
                {
                    "id": "vscode.npm",
                    "path": "./translations/extensions/npm.i18n.json"
                },
                {
                    "id": "vscode.php",
                    "path": "./translations/extensions/php.i18n.json"
                },
                {
                    "id": "vscode.extension-editing",
                    "path": "./translations/extensions/extension-editing.i18n.json"
                },
                {
                    "id": "vscode.typescript",
                    "path": "./translations/extensions/typescript.i18n.json"
                },
                {
                    "id": "vscode.configuration-editing",
                    "path": "./translations/extensions/configuration-editing.i18n.json"
                },
                {
                    "id": "vscode.javascript",
                    "path": "./translations/extensions/javascript.i18n.json"
                },
                {
                    "id": "ms-vscode.node-debug2",
                    "path": "./translations/extensions/vscode-node-debug2.i18n.json"
                },
                {
                    "id": "msjsdiag.debugger-for-chrome",
                    "path": "./translations/extensions/vscode-chrome-debug.i18n.json"
                },
                {
                    "id": "ms-vscode.node-debug",
                    "path": "./translations/extensions/vscode-node-debug.i18n.json"
                }
            ]
        }
    ]
  }
  ```
