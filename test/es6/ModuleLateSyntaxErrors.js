//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let moduleId = 0;
function TestImportExportModule(exporting_code, importing_code) {
    moduleId++;

    let exporting_moduleName = `auto_export_${moduleId}`;
    WScript.RegisterModuleSource(exporting_moduleName, exporting_code);

    importing_code = importing_code.replace('$name', exporting_moduleName);

    let importing_moduleName = `auto_import_${moduleId}`;
    WScript.RegisterModuleSource(importing_moduleName, importing_code);

    try {
        WScript.LoadScriptFile(importing_moduleName, 'module');
    } catch(e) {
        print(e.stack);
    }
}

// import named binding from module which does not export a binding with that nme
TestImportExportModule('export default 1234;', 'import { fakebinding } from "$name"; ');
// import and rename a named binding from a module which does not export a binding with that name
TestImportExportModule('export default 1234;', 'import { fakebinding2 as localname } from "$name"; ');
// import default binding from a module which does not have a default export binding
TestImportExportModule('export let not_default = 1234;', 'import _default from "$name"; ');
// import statement located in an eval is syntax error
TestImportExportModule('export default "something";', `eval('import _default from "$name";');`);
// export statement located in eval is also syntax error
TestImportExportModule('', 'eval("export default function() {}");');
// export in eval is executed after parse-time of the module.
// export resolution is performed first so should throw when trying to resolve imports.
TestImportExportModule('eval("export default function() {}");', 'import _default from "$name";');
