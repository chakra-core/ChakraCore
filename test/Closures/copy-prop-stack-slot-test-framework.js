//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

(function (window) {
    ///////////////////////////////////////////////////////////////////////////////
    // Utils object for helper functions
    ///////////////////////////////////////////////////////////////////////////////
    var Utils = {};

    //Doc Mode
    Utils.IE7STANDARDSMODE = 7;
    Utils.IE8STANDARDSMODE = 8;
    Utils.IE9STANDARDSMODE = 9;
    Utils.IE10STANDARDSMODE = 10;
    Utils.IE11STANDARDSMODE = 11;
    Utils.IE12STANDARDSMODE = 12;

    //IE Architectures 
    Utils.X86MODE = "x86";
    Utils.X64MODE = "x64";
    Utils.ARMMODE = "arm";

    Utils.getHOSTMode = function () {
        var documentMode = this.IE11STANDARDSMODE; // Making default to be IE11 standards mode
        //See if document.documentMode is defined
        try {
            if ((document) && (document.documentMode)) {
                documentMode = document.documentMode;
            }
        } catch (ex) {
            //We are not running in IE, need to put logic of versioning for other hosts later
        }
        if (documentMode < 8)
            return this.IE7STANDARDSMODE;
        else if (documentMode == 8)
            return this.IE8STANDARDSMODE;
        else if (documentMode == 9)
            return this.IE9STANDARDSMODE;
        else if (documentMode == 10)
            return this.IE10STANDARDSMODE;
        else if (documentMode == 11 && typeof Math.hypot === "undefined") //hack for now as we run version:6 switch to emulate Doc mode 12
            return this.IE11STANDARDSMODE;
        else
            return this.IE12STANDARDSMODE;
    }

    var activeXInfo = {
        "WScript.Shell": {
            "clsid": "72C24DD5-D70A-438b-8A42-98424B88AFB8",
            "type": "application/x-shellscript"
        },
        "Loader42.Logger": {
            "clsid": "7CDFA963-4058-45AE-9061-D34614D83472",
            "type": "application/vns.ms-loader42loggerax"
        },
        "Scripting.FileSystemObject": {
            "clsid": "0D43FE01-F093-11cf-8940-00A0C9054228",
            "type": "application/x-filesystemobject"
        },
        "MutatorsHost.MutatorsActiveX": {
            "clsid": "ffafec7c-214c-4aab-b929-caa646777649",
            "type": "application/x-mutatorsactivex"
        },
        "apgloballib.apglobal": {
            "clsid": "CBFCDDC2-CDB1-3270-B9C5-FBE31729B041",
            "type": "application/x-apgloballib"
        },
        "Logger.Status": {
            "clsid": "712B2344-7A71-4242-B21C-AA728108EDD8",
            "type": "application/x-loggerstatus"
        },
        "WTTLogger.Logger": {
            "clsid": "FFC4E997-D97B-45DF-AD22-44C4B2DEDD2B",
            "type": "application/x-wttlogger"
        }
    };

    function createActiveXObject(progId) {
        var activeX;
        try {
            activeX = new ActiveXObject(progId);
        }
        catch (ex) {
            // Can't create ActiveXObject element in Edge mode, append Object element instead
            activeX = document.createElement("object");
            activeX.setAttribute('id', progId);
            activeX.setAttribute('type', activeXInfo[progId]["type"]);
            document.body.appendChild(activeX);
        }
        return activeX;
    }

    Utils.IEMatchesArch = function () {
        if (this.getProcessorArchitecture() != this.X64MODE) {
            return true;
        }
        if (this.runWow64()) {
            return this.getIEArchitecture() == this.X86MODE;
        }
        else {
            return this.getIEArchitecture() == this.X64MODE;
        }
    }

    Utils.getProcessorArchitecture = function () {
        try {
            var GetEnvVariable = function (variable) {
                var wscriptShell = createActiveXObject("WScript.Shell");
                var wshEnvironment = wscriptShell.Environment("System");
                return wshEnvironment(variable);
            }
            var result = GetEnvVariable("PROCESSOR_ARCHITECTURE");
            if (result != null) {
                result = result.toUpperCase();
            }
            if (result === "AMD64") {
                return this.X64MODE;
            }
            else if (result === "X86") {
                return this.X86MODE;
            }
            else if (result === "ARM") {
                return this.ARMMODE;
            }
        }
        catch (e) {
            throw new Error("[getProcessorArchitecture] Failed to retrieve environment variable. " + e.Message);
        }
    }
    
    Utils.getIEArchitecture = function () {
        if (typeof navigator === 'undefined') {
            return this.X64MODE;
        }
        
        //get the user agent and split the strings and identify t he IE mode that we are running 
        var mode = navigator.userAgent;
        var arr_mode = mode.toLowerCase().split(";");

        for (var i = 0; i < arr_mode.length; i++) {
            if (arr_mode[i] === " wow64") {
                return this.X86MODE;
            }
            else if (arr_mode[i] === " x64") {
                return this.X64MODE;
            }
            else if (arr_mode[i] === " arm") {
                for (var j = i + 1; j < arr_mode.length; j++) {
                    if (arr_mode[j] === " virtual)")
                        return X86MODE;
                }
                return this.ARMMODE;
            }
        }
        //default to x86
        return this.X86MODE;
    }

    Utils.runWow64 = function () {
        try {
            var GetEnvVariable = function (variable) {
                var wscriptShell = createActiveXObject("WScript.Shell");
                var wshEnvironment = wscriptShell.Environment("Process");
                return wshEnvironment(variable);
            }
            var result = GetEnvVariable("NightlyWow64");
            if (result === "1") {
                return true;
            }
            else {
                return false;
            }
        }
        catch (e) {
            throw new Error("[runWow64] Failed to retrieve environment variable. " + e.Message);
        }
    }
    
    //Get the host of script engine
    Utils.WWAHOST = "WWA";
    Utils.IEHOST = "Internet Explorer";
    Utils.getHOSTType = function () {
        //navigator.appName will return the host name, e.g. "WWAHost/1.0" or "Microsoft Internet Explorer" 
        if (typeof navigator != 'undefined' && navigator.appName.indexOf(this.WWAHOST) >= 0) {
            return this.WWAHOST;
        }
        return this.IEHOST;
    }

    //Get localized error message
    Utils.getLocalizedError = function (ID, _default, substitution) {
        if (_default) {
            return _default;
        }
        if (substitution == undefined) {
            var localizedString = apGlobalObj.apGetLocalizedString(ID);
        } else {
            var localizedString = apGlobalObj.apGetLocalizedStringWithSubstitution(ID, substitution);
        }
        return localizedString;
    }

    // read "MaxInterpretCount" environment variable to know how many time to run test suite
    Utils.getMaxInterpretCount = function () {
        try {
            var GetEnvVariable = function (variable) {
                var wscriptShell = createActiveXObject("WScript.Shell");
                var wshEnvironment = wscriptShell.Environment("Process");
                return wshEnvironment(variable);
            }
            var result = GetEnvVariable("MaxInterpretCount");
            if (result === "")
                return 0;
            else
                return Math.floor(result);
        }
        catch (e) {
            return 0;
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    // WTTLogger:
    // Logs directly to WTT.
    ///////////////////////////////////////////////////////////////////////////////
    var loggers = [];
    function WTTLogger() {
        var WTT_TRACE_LVL_MSG = 0x10000000;
        var WTT_TRACE_LVL_USER = 0x20000000;

        var WTT_TRACE_LVL_ERR = 0x01000000;
        var WTT_TRACE_LVL_ASSERT = 0x02000000;
        var WTT_TRACE_LVL_INVALID_PARAM = 0x04000000;
        var WTT_TRACE_LVL_BUG = 0x08000000;

        var WTT_TRACE_LVL_BREAK = 0x00100000;
        var WTT_TRACE_LVL_WARN = 0x00200000;

        var WTT_TRACE_LVL_FUNCTION_ENTRY = 0x00010000;
        var WTT_TRACE_LVL_FUNCTION_EXIT = 0x00020000;
        var WTT_TRACE_LVL_CONTEXT_ENTRY = 0x00040000;
        var WTT_TRACE_LVL_CONTEXT_EXIT = 0x00080000;

        var WTT_TRACE_LVL_START_TEST = 0x0100;
        var WTT_TRACE_LVL_END_TEST = 0x0200;
        var WTT_TRACE_LVL_TCMINFO = 0x0400;
        var WTT_TRACE_LVL_MACHINEINFO = 0x0800;

        var WTT_TRACE_LVL_ROLLUP = 0x0010;

        var WTT_ERROR_TYPE_HRESULT = 0x1;
        var WTT_ERROR_TYPE_NTSTATUS = 0x2;
        var WTT_ERROR_TYPE_WIN32 = 0x4;
        var WTT_ERROR_TYPE_BOOL = 0x8;

        var WTT_ERROR_LIST_EXPECTED = 0x1;
        var WTT_ERROR_LIST_BREAKON = 0x2;

        var WTT_TC_RESULT_PASS = 0x1;
        var WTT_TC_RESULT_FAIL = 0x2;
        var WTT_TC_RESULT_BLOCKED = 0x3;
        var WTT_TC_RESULT_WARN = 0x4;
        var WTT_TC_RESULT_SKIPPED = 0x5;

        var WTT_BUG_TYPE_BLOCKING = 0x1;
        var WTT_BUG_TYPE_NONBLOCKING = 0x2;

        var loggerObj = createActiveXObject("WTTLogger.Logger");
        var logHandle;
        var wwahostTimeout = 1000;
        var maxMsgSize = 3000; //actually 3901, but do we really even need 3000 in wtt

        this.start = function (filename, priority) {
            logHandle = loggerObj.CreateLogDevice(null);
            loggerObj.Trace(WTT_TRACE_LVL_MSG, logHandle, "Running " + filename);
        }

        this.testStart = function (test) {
            loggerObj.StartTest(test.desc, logHandle);
        }

        this.verify = function (test, passed, act, exp, msg) {
            if (!passed)
                loggerObj.Trace(WTT_TRACE_LVL_MSG, logHandle, "Failed - Act: " + act + ", Exp: " + exp + ":" + msg);
        }

        this.pass = function (test) {
            loggerObj.EndTest(test.desc, WTT_TC_RESULT_PASS, "", logHandle);
        }

        this.fail = function (test) {
            loggerObj.EndTest(test.desc, WTT_TC_RESULT_FAIL, "", logHandle);
        }

        this.error = function (test, error) {
            loggerObj.Trace(WTT_TRACE_LVL_MSG, logHandle, "Error: " + error.name + " - " + error.description);
            loggerObj.EndTest(test.desc, WTT_TC_RESULT_FAIL, "", logHandle);
        }

        this.end = function () {
            loggerObj.CloseLogDevice("", logHandle);
            //If we're running WTTLogger in a IE environment, we are probably running tests in wwahost,
            //so we should close the window after we finish in order to write logs and rollup. The timeout
            //is to ensure the rest of glue shutdown happens
            //if(typeof document !== "undefined" && Utils.getHOSTType() == Utils.WWAHOST)
            //    setTimeout(function() { window.close(); }, wwahostTimeout);
        }

        this.comment = function (str) {
            var msg = str.toString().substring(0, maxMsgSize);
            loggerObj.Trace(WTT_TRACE_LVL_MSG, logHandle, msg);
        }

        this.bug = function (number, msg, db) {
            db = db || "Windows 8 Bugs";
            loggerObj.Trace(WTT_TRACE_LVL_BUG, logHandle, db, number, WTT_BUG_TYPE_BLOCKING);
            this.comment(msg);
        }
    }

    WTTLogger.shouldEnable = function () {
        try {
            var test = createActiveXObject("WTTLogger.Logger");
            return true;
        } catch (e) {
            return false;
        }
    }

    loggers.push(WTTLogger);
    ///////////////////////////////////////////////////////////////////////////////
    // HTMLLogger:
    // Provides HTML output. Must be run in a browser to function.
    ///////////////////////////////////////////////////////////////////////////////

    function HTMLLogger() {
        var resultsContainer;
        var testProgress = [];
        var results = [];
        var domReady = false;
        var summaryTable;
        var passes = 0;
        var failures = 0;
        var failureList = [];
        var executingTest = false;
        var done = false;
        var loggerInstance = this;
        var filename;
        var priority;
        var verifies = 0;

        if (document.addEventListener) {
            document.addEventListener("DOMContentLoaded", whenDomReady, false);
        } else {
            setTimeout(waitForDomReady, 1);
        }

        function waitForDomReady() {
            // Otherwise use the oft-used doScroll hack which, according to MSDN, will always thrown an
            // error unless the document is ready.
            try {
                document.documentElement.doScroll("left");
            }
            catch (e) {
                // We're not ready yet. Check again in a bit.
                setTimeout(waitForDomReady, 1);
                return;
            }
            whenDomReady();
        }

        function whenDomReady() {
            domReady = true;
            insertStyles();
            setupMutators();
            createSummaryTable();
            createCheckBox();
            setupTable();
            printResults();
        }

        // Inserts a stylesheet to the DOM and fills it with our styles.
        function insertStyles() {
            var css = [
            "body{font-family:'Segoe UI';font-size:1em}",
            "table{border-collapse:collapse;padding:0;margin:0}",
            "table td,table th{padding:3px}",
            "table tr.test_result td.result{border:1px solid #000;border-right:none}",
            "table tr.test_result td.details{border:1px solid #000;border-left:none}",
            "table td.result{font-size:2em;font-family:'Segoe UI Semibold';text-align:center;padding:3px 20px}",
            "table tr.pass td.result{background-color:#c3d69b}",
            "table tr.fail td.result{background-color:#d99694}",
            "table tr.error td.result{background-color:#d99694}",
            "table tr.bug td.result{background-color:#ffefbb}",
            "table tr.comment td.result, table tr.comment td.details {font-size:1em;text-align:left;}",
            "table td.details{padding:0px}",
            "table td.details table{width:100%;height:100%;}",
            "table tr.pass td.details th{background-color:#c3d69b}",
            "table tr.fail td.details th{background-color:#d99694}",
            "table tr.error td.details th{background-color:#d99694}",
            "table tr.hidden {display:none;}",
            "table td.details tr.detail_pass{background-color:#ebf1dd}",
            "table td.details tr.detail_fail{background-color:#f2dcdb}",
            "table td.details tr.detail_comment{background-color:#f2ffe9;}",
            "table td.details tr.detail_comment td{padding: 10px 20px;}",
            "table td.details tr.detail_comment td.detail_result{font-family:'Segoe UI Semibold';}",
            "table td.details tr.detail_bug{background-color:#ffefbb;}",
            "table td.details tr.detail_bug td{padding: 10px 20px;}",
            "table td.details tr.detail_bug td.detail_result{font-family:'Segoe UI Semibold';}",
            "table td.details th{font-size:1.1em;font-weight:normal;font-family:'Segoe UI Semibold';text-align:left}",
            "table td.details table td{padding:3px 20px}",
            "table td.details table td.error_message{background-color:#d99694;padding:3px}",
            "table td.details td.detail_message{width:95%}",
            "table td.details td.detail_result{width:5%}",
            "#results_summary{margin-bottom:1em;font-size:1.5em;}",
            "div.mutators { float: right }",
            "#results_summary td{padding:0 20px;}"];

            // Create a style dom element
            var style = document.createElement('style');
            style.type = 'text/css';
            style.rel = 'stylesheet';
            style.media = 'screen';

            // Append the element to the page's header
            document.getElementsByTagName("head")[0].appendChild(style);

            // Get a stylesheet object for the element we just inserted
            var stylesheet = document.styleSheets[document.styleSheets.length - 1];

            // Add the styles to the sheet. If there is a cssText property, we can just join together
            // all of the rules and add them at once. Otherwise, add them one at a time.
            if (typeof stylesheet.cssText == "string")
                document.styleSheets[document.styleSheets.length - 1].cssText = css.join('');
            else
                for (var i = 0; i < css.length; i++)
                    stylesheet.insertRule(css[i], i);
        }

        //Replace angle brackets for the text
        function replaceAngleBrackets(text) {
            if (text == undefined)
                return "";
            if (text.replace == undefined)
                return text;
            return text.replace(/&/g, '&amp;')
                   .replace(/</g, '&lt;')
                   .replace(/>/g, '&gt;')
                   .replace(/\n/g, '<br/>')
                   .replace(/\s/g, '&nbsp;');
        }

        // Sets up the master table that will hold the results.
        function setupTable() {
            var masterTable = document.createElement("table");
            var body = document.createElement("tbody");
            masterTable.appendChild(body);
            document.body.appendChild(masterTable);

            resultsContainer = body;
        }

        // Add a dropdown box to select mutators to apply if we have the mutators ActiveX.
        function setupMutators() {
            function applyMutator(e) {
                // Undefined in IE < 8.
                if (typeof e === "undefined")
                    e = window.event;

                var target = (typeof e.target === "undefined") ? e.srcElement : e.target;

                if (target.value === "")
                    runner.mutators = [];
                else
                    runner.mutators = [target.value];

                // Clear logger and reset.
                loggerInstance.clear();
                Run();
            }

            try {
                if (typeof mutator === "undefined") {
                    // assigns to global mutator
                    mutator = createActiveXObject("MutatorsHost.MutatorsActiveX");
                }
            } catch (e) {
                return
            }

            var mutators = mutator.GetMutatorsList().split(",");

            // Create a select box and fill it with options for each mutator.
            var select = document.createElement("select");
            select.appendChild(document.createElement("option"));
            for (var i = 0; i < mutators.length; i++) {
                option = document.createElement("option");
                option.innerHTML = mutators[i];
                option.value = mutators[i];
                select.appendChild(option);
            }

            select.onchange = applyMutator;

            // Put the box in a label, put the label in a div, and put the div in the document body.
            var container = document.createElement("div");
            var label = document.createElement("label");
            label.innerHTML = "Mutate With: ";
            label.appendChild(select);
            container.appendChild(label);
            container.setAttribute("class", "mutators");

            document.body.appendChild(container);
        }
        // Adds a result to the results array..
        function addResult(type, result, test, testProgress, error) {
            results.push({
                type: type,
                result: result,
                test: test,
                testProgress: testProgress,
                error: error
            });

            if (domReady)
                printResults();
        }

        // Goes through all the results and prints them to the table. This results array
        // fills up when the DOM is not ready yet.
        //
        function printResults() {
            while (results.length > 0) {
                var result = results.shift();
                if (result.type === "comment" || result.type === "bug")
                    addRow(result);
                else
                    addResultRow(result.result, result.test, result.testProgress, result.error);
            }
        }
        //Add a comment or bug to the status table
        function addRow(result) {
            var comment = result.result;
            var row = resultsContainer.insertRow(-1);
            var res = row.insertCell(-1);
            var details = row.insertCell(-1);

            res.innerHTML = replaceAngleBrackets(result.type);
            details.innerHTML = replaceAngleBrackets(comment);

            row.className = result.type.toLowerCase();
            res.className = "result";
        }
        // Adds a result to the status table.
        function addResultRow(result, test, testProgress, error) {
            var row = resultsContainer.insertRow(-1);
            var res = row.insertCell(-1);
            var details = row.insertCell(-1);

            res.innerHTML = result.toUpperCase();

            row.className = "test_result " + result;
            res.className = "result";
            details.className = "details";

            var detailsTable = constructDetailsTable(test, testProgress, error);
            details.appendChild(detailsTable);
        }

        // Constructs and returns the table that holds all the test progress that were
        // run.
        function constructDetailsTable(test, testProgress, error) {
            var table = document.createElement('table');
            var body, row, cell, text, msgCell;

            // Create the row with the test description
            body = document.createElement("tbody");
            row = document.createElement("tr");
            cell = document.createElement("th");
            cell.innerHTML = replaceAngleBrackets(test.id + ": " + test.desc);
            cell.setAttribute("colSpan", "2");
            row.appendChild(cell);
            body.appendChild(row);

            // Create an error row if there's an error.
            if (typeof error != "undefined") {
                row = document.createElement("tr");
                cell = document.createElement("td");
                cell.innerHTML = error.name + ": " + error.message;
                cell.setAttribute("colSpan", "2");
                cell.className = "error_message";
                row.appendChild(cell);
                body.appendChild(row);
            }


            // Loop through all the test progress adding rows for each.
            var assertionResult;
            for (var i = 0; i < testProgress.length; i++) {
                if (testProgress[i].type === 'comment')
                    assertionResult = "comment";
                else if (testProgress[i].type === 'bug')
                    assertionResult = "bug";
                else if (testProgress[i].passed)
                    assertionResult = "pass";
                else
                    assertionResult = "fail";

                row = document.createElement("tr");
                row.className = "detail_" + assertionResult;

                if (testProgress[i].passed)
                    row.className += " hidden";
                cell = document.createElement("td");
                cell.className = "detail_result";
                msgCell = document.createElement("td");
                msgCell.className = "detail_message";

                cell.innerHTML = replaceAngleBrackets(assertionResult);
                msgCell.innerHTML = replaceAngleBrackets(testProgress[i].message);

                if (testProgress[i].type !== 'comment' && testProgress[i].type !== 'bug') {
                    if (!testProgress[i].passed) {
                        msgCell.innerHTML += "<br>Expected: " + testProgress[i].expected;
                        msgCell.innerHTML += "<br>Actual: " + testProgress[i].actual;
                    }
                }
                row.appendChild(cell);
                row.appendChild(msgCell);
                body.appendChild(row);
            }

            table.appendChild(body);
            return table;
        }

        function createSummaryTable() {
            var table = document.createElement("table");
            table.id = "results_summary";
            var row = table.insertRow(-1);

            var cell = row.insertCell(-1);
            cell.id = "testName";

            cell = row.insertCell(-1);
            cell.id = "priority";

            cell = row.insertCell(-1);
            cell.id = "passes";

            cell = row.insertCell(-1);
            cell.id = "failures";

            cell = row.insertCell(-1);
            cell.id = "total";

            var rowForFailure = table.insertRow(-1);
            var cellForFailure = rowForFailure.insertCell(-1);
            cellForFailure.id = "FailedCases";

            document.body.appendChild(table);

            if (done)
                populateSummaryTable();
        }

        function populateSummaryTable() {
            if (!domReady)
                return; // we'll come back later if we need at the end of createSummaryTable.

            document.getElementById("testName").innerHTML = "TEST: " + filename;
            document.getElementById("priority").innerHTML = "PRIORITY: " + priority;
            document.getElementById("passes").innerHTML = "PASS: " + passes;
            document.getElementById("failures").innerHTML = "FAIL: " + failures;
            document.getElementById("total").innerHTML = "TOTAL: " + (passes + failures);

            if (failures > 0)
                document.getElementById("FailedCases").innerHTML = "Failed Cases: " + failureList.toString();

            if ((passes + failures) === 0) {
                document.write("<h1>No tests were run! There's probably a SyntaxError in the test file.</h1>")
            }

            // Added for XBox team to parse out pass/fail from page title
            document.title =
                'PASS:' + passes +
                ';FAIL:' + failures +
                ';TOTAL:' + (passes + failures);
        }
        function createCheckBox() {
            var box = document.createElement("input");
            box.type = "checkbox";
            box.checked = false;
            box.id = "displayCheckbox";

            var label = document.createElement('label');
            label.innerHTML = "Display passed test result"

            box.onclick = function () {
                var trList = document.getElementsByTagName("tr");
                for (var x = 0; x < trList.length; x++) {
                    if (this.checked) {
                        //Remove hidden in class name
                        trList[x].className = trList[x].className.replace("hidden", "");
                    }
                    else {
                        //Add hidden to detail_pass
                        trList[x].className = trList[x].className.replace("detail_pass", "detail_pass hidden");
                    }
                }
            }

            document.body.appendChild(box);
            document.body.appendChild(label);
        }
        function internalComment(type, str) {
            if (executingTest)
                testProgress.push({ type: type, message: str });
            else
                addResult(type, str);
        }

        this.start = function (file, pri) {
            filename = file;
            priority = pri;
        }

        this.testStart = function (test) {
            testProgress = []; // track assertions and comments from tests.
            executingTest = true;
        }

        this.verify = function (test, passed, act, exp, msg) {
            verifies++;
            if (verifies < 1000 || !passed) {
                testProgress.push({ type: 'assertion', passed: passed, actual: act, expected: exp, message: msg });
            }
        }

        this.pass = function (test) {
            passes++;
            addResult('result', 'pass', test, testProgress);

            executingTest = false;
        }

        this.fail = function (test) {
            failures++;
            failureList.push(test.id);
            addResult('result', 'fail', test, testProgress);

            executingTest = false;
        }

        this.error = function (test, error) {
            failures++;
            // Throw the error so we can debug in browser.
            setTimeout(function () { throw error }, 0);
            addResult('result', 'error', test, testProgress, error);

            executingTest = false;
        }

        this.end = function () {
            done = true;
            populateSummaryTable();
        }

        this.comment = function (str) {
            internalComment('comment', str);
        };

        this.bug = function (number, msg) {
            msg = msg || "";
            internalComment('bug', "Bug: " + number + ": " + msg);
        };

        this.clear = function () {
            if (typeof resultsContainer == "undefined")
                return;

            passes = 0;
            failures = 0;
            failureList = [];
            var children = resultsContainer.childNodes;
            for (var i = children.length - 1; i >= 0; i--) {
                resultsContainer.removeChild(children[i]);
            }
        }
    }

    HTMLLogger.shouldEnable = function () {
        return typeof document !== "undefined"
    }

    loggers.push(HTMLLogger);

    ///////////////////////////////////////////////////////////////////////////////
    // WScriptLogger:
    // Log to the console using WScript.
    ////////////////////////////////////////////////////////////////////////////////
    function WScriptLogger() {
        var passCount = 0;
        var failCount = 0;
        var verifications = [];
        var verbose = false;

        // Initialize the projection related stuff in JsHost
        if (Utils.getHOSTType() == Utils.WWAHOST) {
            WScript.InitializeProjection();
        }

        this.start = function (filename, priority) {
            if (!verbose) { 
                return;
            }
            
            if (priority === "all") {
                WScript.Echo(filename);
            } else {
                WScript.Echo(filename + " Priority " + priority);
            }
        }

        this.testStart = function (test) {
            verifications = [];
        }

        this.verify = function (test, passed, act, exp, msg) {
            //print(`test:${test} passed:${passed} act:${act} exp:${exp} msg:${msg}`)
            if (passed) {
                if (verbose) {
                    verifications.push("\tPass: " + msg + "\n\t\t  Actual: " + act + "\n");
                }
            } else {
                verifications.push("\n\tFail: " + msg + "\n\t\tExpected: " + exp + "\n\t\t  Actual: " + act + "\n");
            }
        }

        this.pass = function (test) {
            passCount++;
            
            if (verbose) {
                WScript.Echo("PASS\t" + test.id + ": " + test.desc);
                
                //Needed for baselines
                WScript.Echo(verifications.join("\n"));
            }
        }

        this.fail = function (test) {
            failCount++;
            WScript.Echo("");
            WScript.Echo("");
            WScript.Echo("FAIL\t" + test.id + ": " + test.desc);
            WScript.Echo("");
            WScript.Echo(verifications.join("\n"));
            WScript.Echo("");
            WScript.Echo("");
        }

        this.error = function (test, error) {
            failCount++;
            WScript.Echo("");
            WScript.Echo("");
            WScript.Echo("FAIL\t" + test.id + ": " + test.desc);
            WScript.Echo("\n\tError: " + error.name + " - " + error.description);
            if (verifications.length > 0) {
                WScript.Echo("");
                WScript.Echo(verifications.join("\n"));
                WScript.Echo("");
            }
            WScript.Echo("");
            WScript.Echo("");
        }

        this.end = function () {
            if (verbose) {
                WScript.Echo("");
                WScript.Echo("Passed: " + passCount);
                WScript.Echo("Failed: " + failCount);
                if ((passCount + failCount) === 0) {
                    WScript.Echo("");
                    WScript.Echo("No tests were run! There's probably a SyntaxError in the test file.")
                }
            } else {
                if (failCount === 0) {
                    WScript.Echo('pass');
                } else {
                    WScript.Echo('fail');
                }
            }
        }

        this.comment = function (str) {
            if (verbose) {
                verifications.push("\tComment: " + str);
            }
        };
    }


    WScriptLogger.shouldEnable = function () {
        return typeof WScript !== "undefined" && typeof WScript.Echo !== "undefined" && typeof window.location === "undefined";
    }

    loggers.push(WScriptLogger);


    // A simple logger object exposed on the window, giving the user access to the comment method.
    // Simply publishes a comment event.
    var windowLogger = {};
    windowLogger.comment = function (str) {
        runner.publish('comment', str);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // NightlyLogger:
    // Provides Logging for automated nightly runs using apgloblib. Must be run in a browser to
    // function.
    ///////////////////////////////////////////////////////////////////////////////

    function NightlyLogger() {
        var apPlatform;
        var runCount;

        function VBS_apglobal_init() {
            apPlatform = apGlobalObj.apGetPlatform();
            runCount = 0;
        }

        function apInitTest(stTestName) {
            if (window.runsExecuted == 0) {    // In MaxInterprentCount case do it only on first iteration
                apGlobalObj.apInitTest(stTestName);
            }
        }

        function apInitScenario(stScenarioName) {
            runCount++;
            apGlobalObj.apInitScenario(stScenarioName);
        }

        function apLogFailInfo(stMessage, stExpected, stActual, stBugNum) {
            if (stBugNum == null) stBugNum = ""; // and may Fajen suffer for it
            apGlobalObj.apLogFailInfo("" + stMessage, "" + stExpected, "" + stActual, "" + stBugNum);
        }

        function apEndTest() {
            if (window.runsExecuted >= window.runsToExecute - 1) {    // In MaxInterprentCount case do it only on last iteration
                if (runCount === 0) {
                    apLogFailInfo("Zero tests were run! Must be a SyntaxError in the test file...", true, false, '');
                }
                apGlobalObj.apEndTest();
            }
        }

        function apErrorTrap() {
            apGlobalObj.apLogFailInfo("call to apErrorTrap found!", "", "", "");
        }

        this.start = function (filename) {
            VBS_apglobal_init()
            apInitTest(filename)
        }

        this.testStart = function (test) {
            apInitScenario(test.id + " : " + test.desc)
        }

        this.verify = function (test, passed, act, exp, msg) {
            if (!passed)
                apLogFailInfo(msg, exp, act, '');
        }

        this.pass = function (test) { }

        this.fail = function (test) { }

        this.error = function (test, error) {
            apLogFailInfo("Exception thrown : " + error.name + "Message: " + error.message, true, false, '');
        }

        this.end = function () {
            apEndTest();
        }

        this.comment = function (msg) { }
    }

    NightlyLogger.shouldEnable = function () {
        try {
            var GetEnvVariable = function (variable) {
                var wscriptShell = createActiveXObject("WScript.Shell");
                var wshEnvironment = wscriptShell.Environment("Process");
                return wshEnvironment(variable);
            }
            var result = GetEnvVariable("JScript_NightlyRun");
            if (result === "")
                return false;
            else
                return true;
        }
        catch (e) {
            return false;
        }
    }

    loggers.push(NightlyLogger);

    ///////////////////////////////////////////////////////////////////////////////
    // Loader42Logger:
    // Provides Logging for automations runs with Loader42. Must be run in a browser to function.
    ///////////////////////////////////////////////////////////////////////////////

    function Loader42Logger() {
        // Provides logging back to Loader42 (and therefore WTT and Blackbeard)
        var filename = "File Name Not Specified";
        var Loader42Log = {
            // define some constants for reporting success/failure of a test
            FAIL: 0,
            PASS: 1,
            BLOCKED: 2,
            Logger: null,
            TestsComplete: false,
            Debug: false,

            // Initialize the ActiveX logger object
            InitLogger: function () {               
                if (Loader42Log.Logger === null) {
                    try {
                        Loader42Log.Logger = createActiveXObject("Loader42.Logger");
                    }
                    catch (ex) {
                        // Can't create ActiveXObject element in Edge mode, append Object element instead
                        Loader42Log.Logger = document.createElement("object");
                        Loader42Log.Logger.setAttribute('id', 'logger');
                        Loader42Log.Logger.setAttribute('type', 'application/vns.ms-loader42loggerax');
                        document.body.appendChild(Loader42Log.Logger);
                    }
                    try {
                        // Cleanup logger in case where HTMLLoader is not in use 
                        // but Loader42 is present.
                        if (Loader42Log.Logger.loaderName !== "HTMLLoader") {
                            Loader42Log.Logger = null;
                        }
                    }
                    catch (ex) {
                        Loader42Log.Logger = null;
                    }
                }
            },

            // Called at the start of every test case. Argument is the name of the test
            // case.
            TestStart: function (name) {
                Loader42Log.InitLogger();

                if (Loader42Log.Logger) {
                    Loader42Log.Logger.LogTestStart(name);
                }
                else if (Loader42Log.Debug) {
                    alert("TestStart(): " + name);
                }

            },

            // Called at the end of every test case. Name should match what is used in
            // TestStart(). Result should be one of the constants 
            // Logger42Log.FAIL,
            // Logger42Log.PASS,
            // Logger42Log.BLOCKED
            // Message is a simple descriptive error, mostly useful in explaining why
            // a test failed or was blocked.
            TestFinish: function (name, result, message) {
                Loader42Log.InitLogger();

                if (Loader42Log.Logger) {
                    Loader42Log.Logger.LogTestEnd(name, result, message);
                }
                else if (Loader42Log.Debug) {
                    var status;
                    if (result === Loader42Log.PASS) {
                        status = "PASS";
                    }
                    else if (result === Loader42Log.FAIL) {
                        status = "FAIL";
                    }
                    else if (result === Loader42Log.BLOCKED) {
                        status = "BLOCKED";
                    }
                    else {
                        status = "UNKNOWN";
                    }

                    alert("TestFinish(): " + name + "\n\n" + status + ": " + message);
                }
            },

            // Logs a message on behalf of the function. Used for tracing/debugging.
            TestComment: function (comment) {
                Loader42Log.InitLogger();
                if (Loader42Log.Logger) {
                    Loader42Log.Logger.LogComment(comment);
                }

            },

            // Signaled at the end of the test, when the page wishes to close the
            // browser. Should only be called once.
            AllTestsComplete: function () {
                Loader42Log.InitLogger();

                if (Loader42Log.TestsComplete) {
                    Loader42Log.TestStart("LogException");
                    Loader42Log.TestComment("AllTestsComplete() called twice.");
                    Loader42Log.TestFinish("LogException", Loader42Log.PASS, "AllTestsComplete() called twice.");
                }
                else {
                    Loader42Log.TestsComplete = true;
                    if (Loader42Log.Logger) {
                        // Since success/failure is really determined by if there are
                        // any failures in TestFinish, we don't have to really worry
                        // about logging success or failure here.
                        Loader42Log.Logger.LogResult(true, "AllTestsComplete");
                    }
                    else if (Loader42Log.Debug) {
                        alert("AllTestsComplete");
                    }
                }

            }
        };

        //Added for Loader42- True for the first Scenario 
        var firsttest = true;
        var prevScenario = "";
        var loggermessage = "";
        var loggerresult = 1;

        function apInitTest(stTestName) {
            if (window.runsExecuted == 0) {    // In MaxInterprentCount case do it only on first iteration
                filename = stTestName;
                //Added for Loader42 - InitLogger will initialize the logger 
                Loader42Log.InitLogger();
            }
        }

        function apInitScenario(stScenarioName) {
            //Added for Loader42-If this is the first scenario do not call Test Fininsh
            if (firsttest === true) {
                Loader42Log.TestStart(stScenarioName);
                firsttest = false; //Done with the first test 
            }
            else {
                //Added for Loader42- if the Scenario Succeeded log the result as passed 
                if (loggerresult === 1) {
                    Loader42Log.TestComment("");
                    Loader42Log.TestComment("File Name    " + filename);
                    Loader42Log.TestComment("Result    PASSED");
                    Loader42Log.TestComment("Scenario    " + prevScenario);
                    Loader42Log.TestComment("");
                }
                //TestFinish the previous scenario  
                Loader42Log.TestFinish(prevScenario, loggerresult, loggermessage);

                //Test start a New Scenario
                Loader42Log.TestStart(stScenarioName);

                //Added for Loader42-Reset the Variable back to the initial value for the New Scenario 
                loggermessage = ""; //Clear tge loggermessage 
                loggerresult = 1; //Set Default to Success 
            }
            prevScenario = stScenarioName;
        }

        function apLogFailInfo(stMessage, stExpected, stActual, stBugNum) {
            if (stBugNum == null) stBugNum = ""; // and may Fajen suffer for it

            //Added for Loader42 - Store the result in a global variable with all the values 
            Loader42Log.TestComment("");
            Loader42Log.TestComment("File Name    " + filename);
            Loader42Log.TestComment("Result    FAILED");
            Loader42Log.TestComment("Scenario    " + stMessage);
            Loader42Log.TestComment("EXPECTED Result    " + stExpected);
            Loader42Log.TestComment("ACTUAL Result    " + stActual);
            Loader42Log.TestComment("");
            loggerresult = 0; //this indicates failiure 
        }

        function apEndTest() {
            if (window.runsExecuted >= window.runsToExecute - 1) {    // In MaxInterprentCount case do it only on last iteration
                //Added for Loader42- Finish the last test that was started 
                if (loggerresult === 1) {
                    Loader42Log.TestComment("");
                    Loader42Log.TestComment("File Name    " + filename);
                    Loader42Log.TestComment("Result    PASSED");
                    Loader42Log.TestComment("Scenario    " + prevScenario);
                    Loader42Log.TestComment("");
                }
                //Finishing the last Scenario 
                Loader42Log.TestFinish(prevScenario, loggerresult, loggermessage);

                //Added for Loader42 -  - called once to finish the test 
                Loader42Log.TestComment("Ending Test" + filename);
                Loader42Log.AllTestsComplete();
            }
        }


        this.start = function (filename) {
            apInitTest(filename)
        }

        this.testStart = function (test) {
            apInitScenario(test.desc)
        }

        this.verify = function (test, passed, act, exp, msg) {
            if (!passed)
                apLogFailInfo(msg, exp, act, '');
        }

        this.pass = function (test) { }

        this.fail = function (test) { }

        this.error = function (test, error) {
            apLogFailInfo("Exception thrown : " + error.name + "Message: " + error.message, true, false, '');
        }

        this.end = function () {
            apEndTest();
        }

        this.comment = function (msg) {
            Loader42Log.TestComment(msg);
        }
    }

    Loader42Logger.shouldEnable = function () {
        try {
            var Logger = createActiveXObject("Loader42.Logger");
            return true;
        }
        catch (e) {
            return false;
        }

        return false;
    }

    loggers.push(Loader42Logger);

    ///////////////////////////////////////////////////////////////////////////////
    // StressLogger:
    // Provides logging support via the DrummerLogger ActiveX object to log and report to the Drummer test harness.  It also uses the HTML logger to output the results in IE.
    ///////////////////////////////////////////////////////////////////////////////

    function StressLogger() {
        var htmlLogger = new HTMLLogger();
        var htmlLoggerRegistered = false;
        var logger = new ActiveXObject("DrummerLogger.Logger");
        var that = this;
        var id;

        window.onerror = function () {
            this.end();
        }

        //logger.Config.NumberIterations -> Contains the number of iterations asked to perform
        this.CurrentTestGroupIteration = 1;

        function getRunId() {
            id = window.location.href.substring(window.location.href.indexOf("stressrun=") + 10)
        }

        this.start = function (fileName) {
            getRunId();
            logger.Initialize(id);
            that.Config = logger.Config;
            logger.Comment("Test File: " + fileName);
            logger.Comment("Runtime Iterations Requested: " + that.Config.RuntimeIterations);
            logger.Comment("Testcase Iterations Requested: " + that.Config.TestcaseIterations);
            logger.Comment("Collection Frequency Requested: " + that.Config.CollectionFrequency);
            if (that.Config.OutputHTML && !htmlLoggerRegistered) {
                runner.subscribe(htmlLogger);
                htmlLoggerRegistered = true;
            }
        }

        this.testStart = function (test) {

            logger.InitScenario(test.id + ": " + test.desc);
        }

        this.verify = function (test, passed, act, exp, msg) {
            logger.Comment("Passed: " + passed + "; Actual: " + act + "; Expected: " + exp + "; Message: " + msg);
        }

        this.pass = function (test) {
            logger.EndScenario("pass", test.desc);
        }

        this.fail = function (test) {
            logger.EndScenario("fail", test.desc);
        }

        this.error = function (test, error) {
            logger.Error(error.message);
        }

        this.end = function () {
            logger.Comment("TestGroupIteration Executed: " + (that.CurrentTestGroupIteration - 1));
            logger.Finished();
        }

        this.comment = function (msg) {
            logger.Comment(msg);
        }

        this.CollectMetrics = function () {
            logger.CollectMetrics();
        }
    }

    StressLogger.shouldEnable = function () {
        try {
            var logger = new ActiveXObject("DrummerLogger.Logger");
            var str = window.location.href.substring(window.location.href.indexOf("?") + 1);
            if (str.indexOf("stressrun") > -1)
                return true;
            else
                return false;
        }
        catch (e) {
            return false;
        }
    }

    loggers.push(StressLogger);

    ///////////////////////////////////////////////////////////////////////////////
    // End Loggers
    ///////////////////////////////////////////////////////////////////////////////
    //scheduler in charge of giving us tasks
    var scheduler = null;
    var runner = (function () {
        //holds onto objects that wish to be notified of pub/sub events
        var subscribers = [];
        //callbacks that get run before any tests execute
        var globalSetups = [];
        //callbacks that get run after all tests execute
        var globalTeardowns = [];
        //how many start calls we are waiting for
        var waitCount = 0;
        //current handle returned from setTimeout
        var waitHandle = null;
        //valid states
        //init - initial state
        //       => running
        //       => error
        //running - running test
        //          => waiting
        //          => ending
        //          => error
        //waiting - waiting for async op
        //          => waiting
        //          => resuming
        //          => timeout
        //          => error
        //resuming - resuming from async op
        //          => error
        //          => running
        //timeout - timed out during async op
        //          => resuming
        //          => error
        //ending - final state
        //error - error in the test
        //        => running
        //        => ending
        //current state of the runner
        var currentState = "init";

        //name of the currently executing file/suite. This is set via
        //setting Loader42_FileName on the global scope
        var fileName = null;
        //priority to run for filtering tests. Valid values are "all" or a number
        var priority = "all";
        //default timeout for async tasks
        var DEFAULT_TIMEOUT = 10000;
        //flag for the main run loop
        var ended = false;
        //object holding valid states and the action to take each iteration while running in them
        var states = {
            init: function () {
                if (window.runsExecuted == 0) {    // In MaxInterprentCount case do it only on first iteration
                    runner.publish('start', fileName, priority);
                }
                scheduler.prepend(globalSetups);
                scheduler.push(globalTeardowns);
                transitionTo("running");
            },
            resuming: function () {
                transitionTo("running");
            },
            running: function () {
                var task = scheduler.next();
                if (task) {
                    try {
                        task();
                    } catch (ex) {
                        runner.publish('error', currentTest, ex);
                        transitionTo("error");
                    }
                } else {
                    transitionTo('ending');
                }
            },
            ending: function () {
                if (++window.runsExecuted < window.runsToExecute) {
                    transitionTo("restarting");
                }
                else {
                    runner.publish("end", fileName, priority);
                    ended = true;
                }
            },
            restarting: function () {
                tasks = ranTasks;
                ranTasks = [];
                transitionTo("init");
            },
            waiting: function () { },
            timeout: function () {
                if (waitHandle === null)
                    return;
                waitHandle = null;
                currentTest.error = new Error("Timed out before runner.start() being called");
                testPassed = false;
                waitCount = 0;
                transitionTo("resuming");
            },
            error: function () {
                ended = true;
            }
        };

        // Subscribes to an event named prop for each property of obj, with the callback being the
        // value.
        function subscribeObject(obj) {
            for (var prop in obj)
                if (typeof obj[prop] === "function")
                    subscribeToEvent(prop, obj[prop]);
        }

        // Subscribes to an event of a given name with the given callback.
        function subscribeToEvent(e, callback) {
            if (typeof subscribers[e] === "undefined")
                subscribers[e] = [];

            subscribers[e].push(callback);
        }

        //transitions the runner to a new state and validates that the transition is allowed
        //parameters:
        // * newState: string representing the new state to transition to
        function transitionTo(newState) {
            function transitionAssert() {
                var res = false;
                for (var i = 0; i < arguments.length; i++)
                    if (arguments[i] === newState)
                        res = true;
                runnerAssert(res, "invalid state transition: " + currentState + "->" + newState)
            }
            switch (currentState) {
                case "init":
                    transitionAssert("running", "error");
                    break;
                case "running":
                    transitionAssert("waiting", "ending", "error");
                    break;
                case "waiting":
                    transitionAssert("resuming", "waiting", "error", "timeout");
                    break;
                case "timeout":
                    transitionAssert("resuming", "error");
                    break;
                case "resuming":
                    transitionAssert("running", "error");
                    break;
                case "error":
                    transitionAssert("running", "ending");
                    break;
                case "ending":
                    transitionAssert("restarting", "error");
                    break;
                case "restarting":
                    transitionAssert("init", "error");
                    break;
                default:
                    runnerAssert(false, "Invalid state");
                    break;
            }
            currentState = newState;
        }

        //internal check to validate assumptions at given points in the code
        //along the lines of the C ASSERT macro or C# asserts
        //parameters:
        // * bool - boolean value that is assumed to be true
        // * msg - message to publish if the bool is false
        //returns:
        // the bool passed in (for use in conditionals or assignment)
        function runnerAssert(bool, msg) {
            if (!bool) {
                runner.publish('comment', "Internal failure: " + msg);
                transitionTo("error");
                currentTest.error = new Error("Internal failure: " + msg);
            }
            return bool;
        }

        //main runloop for the runner. Loops continuously unless state
        //is waiting or the test is ended. Each iteration, it runs the action
        //associated with the current state
        function runTasks() {
            while (!ended) {
                states[currentState]();
                if (currentState === "waiting")
                    return;
            }
        }

        //wraps globalSetup or globalTeardown operations in a try catch block to allow logging
        function wrapGlobalOperation(type, task) {
            return function () {
                try {
                    task();
                } catch (e) {
                    runner.publish('comment', "Error during " + type + " - " + e.name + ": " + e.description);
                    runner.publish('end', fileName, priority);
                    transitionTo("error");
                }
            }
        }


        // Runner object is responsible for controlling the execution of tests. It is exposed on the global
        // object to external code can call any method on runner.
        return {
            // Subscribe to an event by passing an eventName and a callback function. Subscribe to
            // multiple events by passing an object with keys that correspond to the event names you
            // want to subscribe to.
            subscribe: function () {
                if (arguments.length == 1)
                    subscribeObject(arguments[0]);
                else
                    subscribeToEvent(arguments[0], arguments[1]);
            },

            // Publish an event. Calls all subscribed callbacks with the provided arguments.
            publish: function (e) {
                var args = Array.prototype.slice.call(arguments, 1);

                if (typeof subscribers[e] !== "undefined")
                    for (var i = 0; i < subscribers[e].length; i++)
                        subscribers[e][i].apply(undefined, args);
            },

            mutators: [],


            //Runs list of tests once.
            //parameters:
            // * testFileName - name of the test file/suite
            run: function (testFileName) {
                runnerAssert(scheduler !== null, "no scheduler set. This only happens if setScheduler is not called with a scheduler");
                if (ended) {    // We are running it again
                    window.runsExecuted = 0;
                    ended = false;
                    tasks = ranTasks;
                    ranTasks = [];
                    currentState = "init";
                }
                runnerAssert(currentState === "init", "run called while runner in bad state. Probably means multiple calls to run were made");
                if (typeof testFileName === "undefined") {
                    try {
                        testFileName = Loader42_FileName;  // Loader42_FileName may be undefined.
                    } catch (e) {
                        //noop
                    } finally {
                        testFileName = testFileName || "No filename given";
                    }
                }

                fileName = testFileName;
                runTasks();
            },

            //Decrements the wait counter and instructs the runner to resume running after a wait call
            // if the counter is 0. If the timeout has fired, this is a noop.
            //parameters:
            // * testWaitHandle - waithandle corresponding to the return value from start
            start: function (testWaitHandle) {
                if (typeof testWaitHandle !== "undefined" && testWaitHandle !== waitHandle)
                    return;

                waitCount--;
                if (waitCount === 0) {
                    clearTimeout && clearTimeout(testWaitHandle);
                    transitionTo("resuming");
                    waitHandle = null;
                    runTasks();
                }
            },

            //Increments the wait counter and instructs the runner to wait for an async op to finish
            //if the counter is > 0. If count is passed, the wait counter is incremented by that amount.
            //parameters:
            // * timeout - time (in milliseconds) to wait before timing out. Defaults to DEFAULT_TIMEOUT
            // * count - amount to increment counter by. Defaults to 1
            //returns:
            // * the waitHandle produced by setTimeout
            wait: function (timeout, count) {
                count = count || 1;
                timeout = timeout || DEFAULT_TIMEOUT;
                waitCount += count;
                if (waitCount > 0) {
                    transitionTo("waiting");
                    waitHandle = setTimeout && setTimeout(function () {
                        transitionTo("timeout");
                        runTasks();
                    }, timeout);
                    return waitHandle;
                }
            },

            // Create a new test object with the properties specified in obj.
            addTest: function (obj) {
                var test = new TestCase();

                for (var prop in obj) {
                    test[prop] = obj[prop]
                }

                test.AddTest();
            },

            globalSetup: function (callback) {
                globalSetups.push(wrapGlobalOperation("global setup", callback));
            },

            globalTeardown: function (callback) {
                globalTeardowns.push(wrapGlobalOperation("global teardown", callback));
            },

            //sets the scheduler for the runner. Note that this doesn't (currently) unregister the old scheduler.
            //As part of registering the scheduler, the 'schedule' and 'prepend' events are registered for pub/sub
            //parameters:
            // * sched - new scheduler object
            setScheduler: function (sched) {
                scheduler = sched;
                runner.subscribe("schedule", sched.schedule);
            }
        };
    })();

    for (var i = 0; i < loggers.length; i++)
        if (loggers[i].shouldEnable())
            runner.subscribe(new loggers[i]());

    // holds the list of tasks that need to be scheduled. Running is currently destructive to this list
    var tasks = [];
    // holds the list of tasks that have been run already, so that tasks[] list can be restored after the run
    var ranTasks = [];
    window.runsExecuted = 0;
    window.runsToExecute = Utils.getMaxInterpretCount() + 1;

    // populate and return default scheduler object
    var defaultScheduler = (function () {
        return {
            //adds an item to the tail of the list. If the item has a tasks property that is a function
            //the return value of that function is used instead of this item
            //parameters:
            // * item - item to schedule
            schedule: function (item) {
                var items;
                if (item.tasks && typeof item.tasks === "function")
                    items = item.tasks();
                else
                    items = [item];
                for (var i = 0; i < items.length; i++)
                    tasks.push(items[i]);
            },
            //prepends an item or array to the list. If the item is an array, it is concatenated onto
            //the front of the list (i.e. single level flatten). Otherwise it is just put into the list
            //parameters:
            // * obj - item to put onto the front of the list
            prepend: function (obj) {
                var type = hoozit(obj);
                if (type === "array") {
                    for (var i = obj.length - 1; i >= 0; i--) {
                        tasks.unshift(obj[i]);
                    }
                } else {
                    tasks.unshift(obj);
                }
            },
            //appends an item or array to the list. If the item is an array, it is concatenated onto
            //the end of the list (i.e. single level flatten). Otherwise it is just put into the list
            //parameters:
            // * obj - item to put onto the end of the list
            push: function (obj) {
                var type = hoozit(obj);
                if (type === "array") {
                    for (var i = 0; i < obj.length; i++) {
                        tasks.push(obj[i]);
                    }
                } else {
                    tasks.push(obj);
                }
            },
            //returns the next task to run
            //returns:
            //the next task to run or null if there are no more tasks
            next: function () {
                var task = null;
                if (tasks.length > 0) {
                    task = tasks.shift();
                    ranTasks.push(task);
                }

                return task;
            }
        };
    })();
    runner.setScheduler(defaultScheduler);

    ///////////////////////////////////////////////////////////////////////////////
    // Test Runner
    ///////////////////////////////////////////////////////////////////////////////
    var currentTest;
    var testPassed;

    // A test case object
    var TestCase = function () {
        //close over this for the tasks method
        var testCase = this;

        // Flag to execute the test in global scope
        this.useGlobalThis = false;
        this.baselineFile = false;
        this.baselineHandler = 0;
        this.baselineCounter = 0;
        this.pass = true;

        this.AddTest = function () {
            // Function to add use test case to testCases list
            if ((this.id === undefined) || (this.id === "")) {
                runner.publish('comment', "Test case id is not valid ");
            } else if (this.desc === undefined || this.desc === "") {
                runner.publish('comment', "Test case description not specified");
            } else if (this.preReq && (!(typeof (this.preReq) === "function"))) {
                runner.publish('comment', "Invalid preReq function");
            } else if ((this.test === undefined) || (!(typeof (this.test) === "function"))) {
                runner.publish('comment', "Invalid test function");
            } else {
                runner.publish('schedule', this);
            }
        };

        //splits the test work into setup, test and cleanup. This allows the runner to pause
        //between the test task and the cleanup task in order to wait for async operations
        //returns:
        // - an array of functions
        this.tasks = function () {
            function setup() {
                testPassed = true;
                currentTest = testCase;

                runner.publish('testStart', currentTest);
            }

            function test() {
                // if (!Utils.IEMatchesArch()) {
                    // throw new Error("IE architecture setting does not match Processor architecture and run settings: " + Utils.getIEArchitecture());
                // }
                if (!currentTest.preReq || currentTest.preReq()) {
                    try {
                        if (currentTest.useGlobalThis) {
                            testFunc = currentTest.test;
                            testFunc();
                        } else {
                            currentTest.test();
                        }
                    } catch (e) {
                        currentTest.error = e;
                    }
                } else {
                    runner.publish('comment', "Test prereq returned false. Skipping");
                }
            }

            function cleanup() {
                if (currentTest.error !== undefined) {
                    runner.publish('error', currentTest, currentTest.error);
                } else if (testPassed) {
                    runner.publish('pass', currentTest);
                } else {
                    runner.publish('fail', currentTest);
                }
                runner.publish('testEnd', currentTest);
            }

            return [setup, test, cleanup];
        }
    }

    /*
    * Baseline test cases
    */

    function addEscapeCharacter(txt) {
        txt = txt.replace(/\\/g, "\\\\");
        txt = txt.replace(/\"/g, "\\\"");
        txt = txt.replace(/\'/g, "\\\'");
        txt = txt.replace(/\r/g, "\\r");
        txt = txt.replace(/\n/g, "\\n");

        return txt;
    }

    //Testcase that will use baseline support
    function BaselineTestCase() {
        //close over this for the tasks method
        var testCase = this;

        // Flag to execute the test in global scope
        this.useGlobalThis = false;
        this.baselineFile = false;
        this.baselineHandler = 0;
        this.baselineCounter = 0;
        this.pass = true;

        this.AddTest = function () {
            // Function to add use test case to testCases list
            if ((this.id === undefined) || (this.id === "")) {
                runner.publish('comment', "Test case id is not valid ");
            } else if (this.desc === undefined || this.desc === "") {
                runner.publish('comment', "Test case description not specified");
            } else if (this.preReq && (!(typeof (this.preReq) === "function"))) {
                runner.publish('comment', "Invalid preReq function");
            } else if ((this.test === undefined) || (!(typeof (this.test) === "function"))) {
                runner.publish('comment', "Invalid test function");
            } else {
                runner.publish('schedule', this);
            }
        };

        //splits the test work into setup, test and cleanup. This allows the runner to pause
        //between the test task and the cleanup task in order to wait for async operations
        //returns:
        // - an array of functions
        this.tasks = function () {
            function setup() {
                testPassed = true;
                currentTest = testCase;
                BaselineTestCase.startBaseline();
                runner.publish('testStart', currentTest);
            }

            function test() {
                if (!currentTest.preReq || currentTest.preReq()) {
                    try {
                        if (currentTest.useGlobalThis) {
                            testFunc = currentTest.test;
                            testFunc();
                        } else {
                            currentTest.test();
                        }
                    } catch (e) {
                        currentTest.error = e;
                    }
                } else {
                    runner.publish('comment', "Test prereq returned false. Skipping");
                }
            }

            function cleanup() {
                if (currentTest.error !== undefined) {
                    runner.publish('error', currentTest, currentTest.error);
                } else if (testPassed) {
                    runner.publish('pass', currentTest);
                } else {
                    runner.publish('fail', currentTest);
                }
                BaselineTestCase.stopBaseline();
                runner.publish('testEnd', currentTest);
            }

            return [setup, test, cleanup];
        }
    }

    //Static variables for BaselineTestCase
    BaselineTestCase.baselineFileExt = ".baseline.js";
    BaselineTestCase.editMode = false;
    BaselineTestCase.useGlobalBaseline = false;
    BaselineTestCase.fileName = null;
    BaselineTestCase.fileHandler = null;
    BaselineTestCase.elementIndexInFile = 0;
    BaselineTestCase.currentTestcaseID = 0;

    //Static methods for BaselineTestCase
    BaselineTestCase.constructBLFileName = function (testCaseID) {
        var filePath = window.location.href;

        var fileName = filePath.substring(filePath.lastIndexOf("/") + 1);
        fileName = fileName.substring(0, fileName.lastIndexOf("."));


        if (typeof (testCaseID) == "undefined") {
            fileName = fileName + "." + getHOSTMode() + BaselineTestCase.baselineFileExt;
        }
        else {
            testCaseID = String(testCaseID).replace(/[^a-zA-Z 0-9]+/g, "_");
            fileName = fileName + testCaseID + "." + getHOSTMode() + BaselineTestCase.baselineFileExt;
        }

        var baselineFilePath = filePath.substring(0, filePath.lastIndexOf("/") + 1) + fileName;

        //redirecting the baseline file path to js directory when we find EzeHtmlTestFiles directory structure
        baselineFilePath = baselineFilePath.replace(/EzeHtmlTestFiles/, "EzeJSTestFiles");

        // mutator local run will fail in baseline testcases if don't reset this counters
        BaselineTestCase.elementIndexInFile = 0;
        BaselineTestCase.currentTestcaseID = 0;

        //loadjsfile(baselineFilePath);
        return baselineFilePath;
    }

    BaselineTestCase.openBaseline = function (fileName) {
        var objFSO = createActiveXObject("Scripting.FileSystemObject");

        if (fileName.indexOf("file:///") > -1) fileName = fileName.substring(8);

        BaselineTestCase.fileHandler = objFSO.CreateTextFile(fileName, true);
        BaselineTestCase.fileHandler.WriteLine("var baselineExpArr = [");
    }

    BaselineTestCase.closeBaseline = function () {
        if (BaselineTestCase.editMode) {
            BaselineTestCase.fileHandler.WriteLine();
            BaselineTestCase.fileHandler.WriteLine("];");
            BaselineTestCase.fileHandler.Close();
        }
        else {
            //If there is more elements in the baseline file has not been compared, fail the test.
            if (typeof (baselineExpArr) != "undefined") {
                if (BaselineTestCase.elementIndexInFile < baselineExpArr.length) {
                    fail("There is more elements in baseline file than expected. Total element in baselinefile=" + baselineExpArr.length + ", expected=" + BaselineTestCase.elementIndexInFile);
                }
            }
            if (!BaselineTestCase.useGlobalBaseline) {
                BaselineTestCase.elementIndexInFile = 0;
                BaselineTestCase.fileHandler = 0;
            }
        }
    }

    BaselineTestCase.verify = function (act, msg) {
        if (BaselineTestCase.editMode) {
            if (BaselineTestCase.elementIndexInFile > 0) {
                BaselineTestCase.fileHandler.WriteLine(",");
            }
            var escapedAct = act;
            if (act instanceof String || typeof act == "string") {
                escapedAct = addEscapeCharacter(act);
            }

            if (escapedAct != null) //don't add quotes to null values
            {
                BaselineTestCase.fileHandler.Write("\"" + escapedAct + "\"");
            }
            else {
                BaselineTestCase.fileHandler.Write("null");
            }
            BaselineTestCase.elementIndexInFile++;
        }
        else {
            if (BaselineTestCase.elementIndexInFile >= baselineExpArr.length) {
                fail("Test is expecting more elements in baseline file.Expecting " + (BaselineTestCase.elementIndexInFile + 1) + ", actual = " + baselineExpArr.length);
            }
            else {
                var exp = baselineExpArr[BaselineTestCase.elementIndexInFile++];
                if (act != null && act.toString)
                    verify(act.toString(), exp, msg);
                else
                    verify(act, exp, msg);
            }
        }
    }

    BaselineTestCase.loadBaseline = function (fileName) {
        var loadSuccess = true;
        var oXmlHttp = new XMLHttpRequest();
        oXmlHttp.OnReadyStateChange = function () {
            if (oXmlHttp.readyState == 4) {
                //XmlHttpRequest will return 0 if we request a file from local disk
                //Return value 200 on succees only if we request a file from web server
                if (oXmlHttp.status == 200 || window.location.href.indexOf("http") == -1) {
                    if (fileName != null) {
                        if (document.getElementById("blScriptTag") == null) {
                            var header = document.getElementsByTagName('HEAD').item(0);
                            var oScript = document.createElement("script");
                            oScript.language = "javascript";
                            oScript.type = "text/javascript";
                            oScript.defer = true;
                            oScript.id = "blScriptTag";
                            oScript.text = oXmlHttp.responseText;
                            header.appendChild(oScript);
                        }
                        else {
                            //Get response text and strip the "var baselineExpArr =" then assign the new value to baselineExpArr
                            var a = oXmlHttp.responseText;
                            baselineExpArr = eval(a.substring(a.indexOf("=") + 1, a.length - 2));
                        }
                    }
                }
                else {
                    loadSuccess = false;
                }
            }
        }
        oXmlHttp.open('GET', fileName, false);
        oXmlHttp.send(null);

        if (!loadSuccess) throw new Error('XML request error for file ' + fileName + ': ' + oXmlHttp.statusText + ' (' + oXmlHttp.status + ')');
    }

    BaselineTestCase.startBaseline = function () {
        if (BaselineTestCase.useGlobalBaseline)
            return;

        //Set baseline file name based on the fileName from Run method and test id
        //This block will be executed we are not using global baseline
        if (currentTest instanceof BaselineTestCase) {

            //Create baseline file if baselineEditMode is true, otherwise load the js file
            if (BaselineTestCase.editMode) {
                BaselineTestCase.openBaseline(BaselineTestCase.constructBLFileName(currentTest.id));
            }
            else {
                BaselineTestCase.loadBaseline(BaselineTestCase.constructBLFileName(currentTest.id));
            }
        }
    }

    BaselineTestCase.stopBaseline = function () {
        if (BaselineTestCase.useGlobalBaseline)
            return;

        if (currentTest instanceof BaselineTestCase) {
            //Close the baseline file
            BaselineTestCase.closeBaseline();
        }

        BaselineTestCase.currentTestcaseID++;
    }

    BaselineTestCase.startGlobalBaseline = function () {
        if (!BaselineTestCase.useGlobalBaseline)
            return;

        try {
            //Open baseline file if we use global baseline (one baseline for all test)
            if (BaselineTestCase.editMode) {
                BaselineTestCase.openBaseline(BaselineTestCase.constructBLFileName());
            }

            //load the global baseline file
            else {
                BaselineTestCase.loadBaseline(BaselineTestCase.constructBLFileName());
            }
        }
        catch (e) {
            fail("Exception occured on opening baseline file with message: " + e.message);
        }
    }

    BaselineTestCase.stopGlobalBaseline = function () {
        if (!BaselineTestCase.useGlobalBaseline)
            return;

        try {
            //Close the baseline file (only if we use global baseline file)
            BaselineTestCase.closeBaseline();
        }
        catch (e) {
            fail("Exception occured on closing baseline file with message: " + e.message);
        }
    }

    // Subscribe to various events to set up baseline.
    runner.subscribe('start', BaselineTestCase.startGlobalBaseline);
    runner.subscribe('end', BaselineTestCase.stopGlobalBaseline);

    /*
    * This object facilitates testing cross-context scenarios. Create a new instance first to getcd 
    * started. See http://devdiv/sites/bpt/team/eze/Eze%20Wiki/Cross%20Context%20Test%20Framework.aspx
    * for documentation.
    */
    var CrossContextTest = function () {
        var cct = this;
        cct.testIframe = true;
        cct.testWindow = (Utils.getHOSTType() == Utils.WWAHOST) ? false : true;

        var waitingForReady = false;
        var readyCallbacks = [];
        var childFunctions = [];
        var childProperties = {};

        /*
        * Wait for the document to be ready for DOM updates.
        * This should be updated to use DOMContentReady when that is supported.
        */
        function onReady(callback) {
            // Check if we're already ready, and if so, just call the callback.
            if (document.readyState == "complete") {
                callback.call();
                return;
            }

            // Otherwise, push to our ready list and start waiting.
            readyCallbacks.push(callback);

            // If we're not already waiting, wait for document ready and call callbacks.
            if (!waitingForReady) {
                waitingForReady = true;
                waitForReady();
            }
        }

        /*
        * Wait until doScroll doesn't throw an exception, and then run all the callbacks.
        */
        function waitForReady() {
            // Otherwise use the oft-used doScroll hack which, according to MSDN, will always thrown an
            // error unless the document is ready.
            try {
                document.documentElement.doScroll("left");

            }
            catch (e) {
                // We're not ready yet. Check again in a bit.
                setTimeout(waitForReady, 1);
                return;
            }

            // Assume we're ready, so call all the callbacks.
            for (var i = 0; i < readyCallbacks.length; i++) {
                readyCallbacks[i].call();
            }
        }

        /* 
        * Sets any properties on the newly opened window.
        * be careful -- this only reference obj from parent context
        */
        function setChildWindowProperties(win) {
            for (var prop in childProperties) {
                win[prop] = childProperties[prop];
            }
        }

        /* 
        * Writes the HTML to the child window by appending HTML tag. Used for WWA
        * * parentWindow, that refers to either window.parent or window.opener, depending on if this is a
        *   popup window or an iframe, 
        *   Append html, head, body and script content to iframe/popup window
        */

        function AppendChildHtml(doc) {

            function createChildTag(tagName, attributes) {
                tag = doc.createElement(tagName);
                for (var k in attributes) {
                    tag.setAttribute(k, attributes[k]);
                }
                return tag;
            }

            doc.open();
            doc.write("<html><head></head><body></body></html>");
            doc.close();

            var head = doc.getElementsByTagName("head")[0];

            var meta1 = createChildTag('meta', {
                'http-equiv': '"X-UA-Compatible"',
                'content': '"IE=' + document.documentMode + '"'
            }
        );
            head.appendChild(meta1);

            // add the body content
            if (typeof cct.childContent !== 'undefined') {
                var body = doc.getElementsByTagName("body")[0];
                body.innerHTML = cct.childContent;
            }

            // this function will be sent to child, be called when the script load complete
            function __init__(sender) {
                parentWindow = window.parent == window ? window.opener : window.parent;
                if (sender.readyState == "complete") {
                    sender.onreadystatechange = null;
                    parentWindow.childReady.ready();  // notify parent to start test   
                }
            }

            // init the script 
            var script = createChildTag('script', {
                type: 'text/javascript',
                onreadystatechange: "iamready(this)"
            }
        );
            script.text += __init__.toString() + "\n" + "var iamready =  __init__;\n";
            // write the childFunctions
            for (var i = 0; i < childFunctions.length; i++) {
                script.text += childFunctions[i].toString() + "\n";
            }
            head.appendChild(script);
        }

        /* 
        * Writes html content to doc. Used for IE
        * * parentWindow, that refers to either window.parent or window.opener, depending on if this is a
        *   popup window or an iframe, 
        */

        function WriteChildHtml(doc) {
            doc.open();
            doc.write("<html>\n<head>\n");
            doc.write("<meta http-equiv=\"X-UA-Compatible\" content=\"IE=" + document.documentMode + "\" />\n");
            doc.write("<scri" + "pt type='text/javascript'>\n");
            doc.write("var parentWindow = window.parent == window ? window.opener : window.parent;\n");

            //// function waitForReady will sent to child, to determin if child is ready
            function waitForReady() {
                if (document.addEventListener) {
                    document.addEventListener("DOMContentLoaded", whenDomReady, false);
                } else {
                    setTimeout(waitForDomReady, 1);
                }
                function waitForDomReady() {
                    // Otherwise use the oft-used doScroll hack which, according to MSDN, will always thrown an
                    // error unless the document is ready.
                    try {
                        document.documentElement.doScroll("left");
                    }
                    catch (e) {
                        // We're not ready yet. Check again in a bit.
                        setTimeout(waitForDomReady, 1);
                        return;
                    }
                    whenDomReady();
                }
                function whenDomReady() {
                    parentWindow.childReady.ready();
                }
            }
            //////

            doc.write(waitForReady.toString() + "\n");
            doc.write("waitForReady();\n");

            for (var i = 0; i < childFunctions.length; i++) {
                doc.write(childFunctions[i].toString() + "\n");
            }

            doc.write("</scr" + "ipt>\n</head>\n<body>\n");

            if (cct.childContent !== undefined) doc.write(cct.childContent)

            doc.write("</body></html>");
            doc.close();
        }

        var writeChildContent = (Utils.getHOSTType() == Utils.WWAHOST) ? AppendChildHtml : WriteChildHtml;

        // Set either of these to control the content of the child windows.
        this.childContent = undefined;

        /*
        * If we have Loader42Log, use that to log comments, otherwise use a nop. This is merely a
        * default and can be overridden by users should they need custom logging.
        */
        if (typeof window.logger !== 'undefined') {
            this.comment = logger.comment;
        } else {
            this.comment = new Function();
        }

        /*
        * Add a function to the list of functions to append to the child windows.
        * NOTE: While when you define these functions they look like closures, they are not used as such.
        * The toString representation is appended to the new windows, destroying any scope you might have
        * set up.
        */
        this.addChildFunction = function (func) {
            childFunctions.push(func);
        }

        /*
        * Add a property to the list of properties to give to the child windows.
        */
        this.addChildProperty = function (name, value) {
            childProperties[name] = value;
        }

        function ChildReady(waitHandle) {
            this.waitHandle = waitHandle;
            this.ready = function () {
                runner.start(this.waitHandle);
            }
        }

        var prepareIframe = function () {
            cct.comment("Starting iframe tests");
            cct.frame = document.createElement('iframe');
            document.body.appendChild(cct.frame);
            var waitHandle = runner.wait();
            window.childReady = new ChildReady(waitHandle);
            writeChildContent(cct.frame.contentDocument ? cct.frame.contentDocument : cct.frame.contentWindow.document);
        }

        var runIframe = function () {
            if (typeof BaselineTestCase !== 'undefined' && typeof mutator !== 'undefined') {
                // save the check point for window.open
                cct.baselineTCElementIndexInFile = BaselineTestCase.elementIndexInFile;
            }
            setChildWindowProperties(cct.frame.contentWindow);
            try {
                cct.callback.call(undefined, cct.frame.contentWindow);
            }
            catch (e) {
                currentTest.error = e;
            }
        }

        var cleanupIframe = function () {
            window.childReady = undefined;
            // Remove the iframe when we're done so it doesn't interfere with any other tests.  
            document.body.removeChild(cct.frame);
        }

        var preparePopWin = function () {
            cct.comment("Starting new window tests");
            cct.win = window.open();
            var waitHandle = runner.wait();
            window.childReady = new ChildReady(waitHandle);
            writeChildContent(cct.win.document);
        }

        var runPopWin = function () {
            if (typeof BaselineTestCase !== 'undefined' && typeof mutator !== 'undefined') {
                // save the check point for window.open
                BaselineTestCase.elementIndexInFile = cct.baselineTCElementIndexInFile;
            }
            setChildWindowProperties(cct.win);
            try {
                cct.callback.call(undefined, cct.win);
            }
            catch (e) {
                currentTest.error = e;
            }
        }

        var cleanupPopWin = function () {
            window.childReady = undefined;
            cct.win.close();
            window.parentWaitHandle = undefined;

            // make sure the popup window is closed and then move on
            var waitHandle = runner.wait();
            setTimeout(function () {
                try {
                    if (!cct.win.closed) {
                        setTimeout(arguments.callee, 10);
                        return;
                    }
                } catch (e) { }
                runner.start(waitHandle);
            }, 10);
        }

        /*
        * Run a test. Do any verification in the callback. The callback will be called with the new
        * window as the first parameter. If you want to wait for the document to be ready, pass true
        * for the second parameter. You should note that this will currently break our test framework
        * as nothing in the test function body can be asyncrhonous. When a test is asynchronous, the
        * test will pass (with no verify statements executed) while the verifies wait in the
        * background for the document to become ready.
        */
        this.test = function (callback, waitForReady) {

            var run = function () {

                cct.callback = callback;

                if (cct.testWindow) {
                    // Test with window.open
                    scheduler.prepend([preparePopWin, runPopWin, cleanupPopWin]);
                }

                if (cct.testIframe) {
                    // Test with iframe.    
                    scheduler.prepend([prepareIframe, runIframe, cleanupIframe]);
                }
            };

            if (waitForReady) onReady(run);
            else run();
        }
    }

    //Mutator support
    var mutator;

    // env will be the empty string if the environment variable is not present.
    var env = "";
    try {
        env = (createActiveXObject("WScript.Shell")).Environment("Process")("JScript_Mutators");

        if (env.length > 0)
            runner.mutators = env.split(",");
    } catch (e) { }


    (function () {
        var originalTest;

        runner.subscribe('testStart', function (test) {
            if (runner.mutators.length === 0)
                return;

            try {
                if (typeof mutator === "undefined")
                    mutator = createActiveXObject("MutatorsHost.MutatorsActiveX");

                runner.publish('comment', "Mutating with " + runner.mutators.join(","));

                originalTest = test.test;
                originalTestString = originalTest.toString();

                // Parse out just the function body to mutate.
                testBody = originalTestString.substring(originalTestString.indexOf("{") + 1,
                                                    originalTestString.lastIndexOf("}"));

                newTestString = mutator.Mutate(runner.mutators.join(","), testBody);

                // Create a new function with the mutated function body. Note that this destroys the
                // lexical environment of the original test function, so test code that relies on
                // identifiers declared outside of the function body but not in global scope will fail.
                test.test = new Function(newTestString);

                runner.publish('comment', "<pre>" + test.test.toString() + "</pre>");
            } catch (e) { }
        })

        /* Restore the original test object after the test has finished.
        */
        runner.subscribe('testEnd', function (test) {
            if (runner.mutators.length === 0)
                return;

            test.test = originalTest;
        })

    })()

    /****************************************************************************
    //Function for stress testing that determines if the test group should loop again
    //Currently not working as scheduler does not call this function. 
    //Need to change the stressLogger to StressLogger.shouldEnable()
    ****************************************************************************/
    function ContinueTestGroupIteration() {

        if (typeof stressLogger !== "undefined") {
            stressLogger.CurrentTestGroupIteration++;

            if (stressLogger.Config.RuntimeIterations < stressLogger.CurrentTestGroupIteration) {
                return false;
            }

            return true;
        }

        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Verification functions.
    ///////////////////////////////////////////////////////////////////////////////
    //the wrapping function is really just to allow folding in editors
    var verify = (function () {
        function verify(act, exp, msg) {
            var result = equiv(act, exp);
            testPassed = testPassed && result;
            runner.publish('verify', currentTest, result, act, exp, msg);
        }
        verify.equal = verify;
        verify.notEqual = function (act, exp, msg) {
            var result = !equiv(act, exp);
            testPassed = testPassed && result;
            runner.publish('verify', currentTest, result, act, "not " + exp, msg);
        };

        verify.strictEqual = function (act, exp, msg) {
            var result = act === exp;
            testPassed = testPassed && result;
            runner.publish('verify', currentTest, result, act, exp, msg);
        }

        verify.notStrictEqual = function (act, exp, msg) {
            var result = act !== exp;
            testPassed = testPassed && result;
            runner.publish('verify', currentTest, result, act, "not " + exp, msg);
        }

        verify.noException = function (callback, msg) {
            try {
                var res = callback();
                verify.equal(res, res, msg);
            } catch (e) {
                verify.equal(res, "no exception", msg);
            }
        };

        verify.exception = function (callback, exception, msg) {
            try {
                var res = callback();
                verify.equal(res, "exception", msg);
            } catch (e) {
                verify.instanceOf(e, exception)
            }
        };

        verify.defined = function (obj, msg) {
            return verify.notStrictEqual(obj, undefined, msg + " should be defined");
        };

        verify.notDefined = function (obj, msg) {
            return verify.strictEqual(obj, undefined, msg + " should not be defined");
        };

        verify.typeOf = function (obj, expected) {
            return verify.equal(typeof obj, expected, "typeof " + obj);
        };

        verify.instanceOf = function (obj, expected) {
            return verify.equal(obj instanceof expected, true, "instanceof " + obj);
        };

        return verify;
    })();

    function fail(msg) {
        verify(true, false, msg);
    }

    function assert(condition, msg) {
        verify(condition, true, msg);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Below here is code to support the old test case format. These old test     
    // cases call a series of ap* functions directly. The object below basically  
    // proxies calls to ap* functions to the appropriate logger. Because the old  
    // test case format doesn't instantiate a test case object, the code below    
    // creates a stub to pass to the loggers.                                     
    ///////////////////////////////////////////////////////////////////////////////

    function OldGlueProxy() {
        var test;
        var testStarted = false;
        var scenarioStarted = false;
        var scenarioPassed = true;

        function finishScenario() {
            if (scenarioPassed)
                runner.publish('pass', test);
            else
                runner.publish('fail', test);
        }

        this.apInitTest = function (name) {
            if (window.runsExecuted == 0) {    // In MaxInterprentCount case do it only on first iteration
                if (testStarted)
                    runner.publish('end');

                testStarted = true;

                if (typeof Loader42_FileName == 'undefined')
                    runner.publish('start', name);
                else
                    runner.publish('start', Loader42_FileName);
            }
        }

        this.apInitScenario = function (name) {
            if (scenarioStarted)
                finishScenario();

            var id = /^\d+/.exec(name);
            if (id === null)
                id = "";

            test = { id: id, desc: name, test: Function() };

            runner.publish('testStart', test);
            scenarioStarted = true;
            scenarioPassed = true;
        }

        this.apEndScenario = function () {
            finishScenario();
            scenarioStarted = false;
        }

        this.apLogFailInfo = function (msg, exp, act) {
            runner.publish('verify', test, false, act, exp, msg);
            scenarioPassed = false;
        }

        this.apEndTest = function () {
            if (window.runsExecuted >= window.runsToExecute - 1) {    // In MaxInterprentCount case do it only on last iteration
                if (scenarioStarted)
                    finishScenario();
                runner.publish('end');
                testStarted = false;
            }
        }

        this.apWriteDebug = function (msg) {
            runner.publish('comment', msg);
        }

        this.VBS_apglobal_init = function () {
            try {
                window.apGlobalObj = createActiveXObject("apgloballib.apglobal");
            } catch (e) {
                // Can't create ActiveX, so make a stub object with the same methods.
                window.apGlobalObj = {
                    apInitTest: apInitTest,
                    apInitScenario: apInitScenario,
                    apLogFailInfo: apLogFailInfo,
                    apEndScenario: apEndScenario,
                    apEndTest: apEndTest,
                    apGetLangExt: function (Lcid) {
                        var LangExt = "";
                        switch (apGlobalObj.apPrimaryLang(Lcid)) {
                            case 0x9:
                                LangExt = "EN"; // LANG_ENGLISH
                                break;
                            case 0xC:
                                LangExt = "FR"; // LANG_FRENCH
                                break;
                            case 0xA:
                                LangExt = "ES"; // LANG_SPANISH
                                break;
                            case 0x7:
                                LangExt = "DE"; // LANG_GERMAN
                                break;
                            case 0x10:
                                LangExt = "IT"; // LANG_ITALIAN
                                break;
                            case 0x16:
                                LangExt = "PT"; // LANG_PORTUGUESE
                                break;
                            case 0x1D:
                                LangExt = "SV"; // LANG_SWEDISH
                                break;
                            case 0x14:
                                LangExt = "NO"; // LANG_NORWEGIAN
                                break;
                        }
                        if (LangExt == "") { //Hack for DBCS
                            switch (Lcid) {
                                case 0x411:
                                    LangExt = "JP"; // LANG_JAPANESE
                                    break;
                                case 0x412:
                                    LangExt = "KO"; // LANG_KOREAN
                                    break;
                                case 0x404:
                                    LangExt = "CHT"; // Chinese
                                    break;
                                case 0x804:
                                    LangExt = "CHS"; // PRC
                                    break;
                            }
                        }
                        return LangExt;
                    },
                    apPrimaryLang: function (Lcid) {
                        return Lcid & 0x3FF;
                    },
                    apGetLocFormatDate: function (Lcid, Fmt) {
                        var strToken = "";
                        switch (Fmt) // am not sure whenstr.toUpper will be supported..
                        {
                            case "LONG DATE":
                            case "long date":
                                strToken = "datelong";
                                break;
                            case "MEDIUM DATE":
                            case "medium date":
                                strToken = "datemed";
                                break;
                            case "SHORT DATE":
                            case "short date":
                                strToken = "dateshort";
                                break;
                            case "GENERAL DATE":
                            case "general date":
                                strToken = "gendate";
                                break;
                            case "LONG TIME":
                            case "long time":
                                strToken = "timelong";
                                break;
                            case "MEDIUM TIME":
                            case "medium time":
                                strToken = "timemed";
                                break;
                            case "SHORT TIME":
                            case "short time":
                                strToken = "timeshort";
                                break;
                        }
                        return apGlobalObj.apGetToken(Lcid, "fmt_named_" + strToken, "OLB");
                    },
                    apGetToken: function (Lcid, strTokenName, strFileName) {
                        // not needed right now..
                        var strToken = "";
                        return (strToken == "") ? "[[Token Not Found]]" : strToken;
                    },
                    apGetLocInfo: function (Lcid, lctype, ProjectOverride, Flags) { },
                    apGetOSVer: function () {
                        //not needed
                        return "NT;";
                    },
                    apGetPathName: function () {
                        //not needed
                        apGlobalObj.apLogFailInfo("FAILURE: TC requested function apGetPathName", "", "", "");
                        apGlobalObj.apEndTest();
                        return "";
                    },
                    apGetPlatform: function () {
                        //not needed
                        return "NT;";
                    },
                    apGetVolumeName: function (OS) {
                        //not needed
                        return "NT;";
                    },
                    LangHost: function () {
                        //  return GetUserDefaultLCID();
                        return 1033;
                    },
                    apGetLocalizedString: function (resID) {
                        return "No localized string returned";
                    },
                    apGetLocalizedStringWithSubstitution: function (resID, stringSubstitution) {
                        return "No localized string returned";
                    }
                }
            }
            window.apPlatform = apGlobalObj.apGetPlatform();
        }

        this.apGetLocale = function () {
            return apGlobalObj.LangHost();
        }

    }

    ///////////////////////////////////////////////////////////////////////////////
    // Object Comparison functions.
    ///////////////////////////////////////////////////////////////////////////////

    // Determine what is o.
    function hoozit(o) {
        if (typeof o === "undefined") {
            return "undefined";
        } else if (o === null) {
            return "null";
        } else if (o.constructor === String) {
            return "string";

        } else if (o.constructor === Boolean) {
            return "boolean";

        } else if (o.constructor === Number) {

            if (isNaN(o)) {
                return "nan";
            } else {
                return "number";
            }

            // consider: typeof [] === object
        } else if (o instanceof Array) {
            return "array";

            // consider: typeof new Date() === object
        } else if (o instanceof Date) {
            return "date";

            // consider: /./ instanceof Object;
            //           /./ instanceof RegExp;
            //          typeof /./ === "function"; // => false in IE and Opera,
            //                                          true in FF and Safari
        } else if (o instanceof RegExp) {
            return "regexp";

        } else if (typeof o === "object") {
            return "object";

        } else if (o instanceof Function) {
            return "function";
        } else {
            return undefined;
        }
    }

    // Call the o related callback with the given arguments.
    function bindCallbacks(o, callbacks, args) {
        var prop = hoozit(o);
        if (prop) {
            if (hoozit(callbacks[prop]) === "function") {
                return callbacks[prop].apply(callbacks, args);
            } else {
                return callbacks[prop]; // or undefined
            }
        }
    }
    // Test for equality any JavaScript type.
    // Discussions and reference: http://philrathe.com/articles/equiv
    // Test suites: http://philrathe.com/tests/equiv
    // Author: Philippe Rath? <prathe@gmail.com>
    var equiv = function () {

        var innerEquiv; // the real equiv function
        var callers = []; // stack to decide between skip/abort functions


        var callbacks = function () {

            // for string, boolean, number and null
            function useStrictEquality(b, a) {
                if (b instanceof a.constructor || a instanceof b.constructor) {
                    // to catch short annotaion VS 'new' annotation of a declaration
                    // e.g. var i = 1;
                    //      var j = new Number(1);
                    return a == b;
                } else {
                    return a === b;
                }
            }

            return {
                "string": useStrictEquality,
                "boolean": useStrictEquality,
                "number": useStrictEquality,
                "null": useStrictEquality,
                "undefined": useStrictEquality,

                "nan": function (b) {
                    return isNaN(b);
                },

                "date": function (b, a) {
                    return hoozit(b) === "date" && a.valueOf() === b.valueOf();
                },

                "regexp": function (b, a) {
                    return hoozit(b) === "regexp" &&
                    a.source === b.source && // the regex itself
                    a.global === b.global && // and its modifers (gmi) ...
                    a.ignoreCase === b.ignoreCase &&
                    a.multiline === b.multiline;
                },

                // - skip when the property is a method of an instance (OOP)
                // - abort otherwise,
                //   initial === would have catch identical references anyway
                "function": function () {
                    var caller = callers[callers.length - 1];
                    return caller !== Object &&
                        typeof caller !== "undefined";
                },

                "array": function (b, a) {
                    var i;
                    var len;

                    // b could be an object literal here
                    if (!(hoozit(b) === "array")) {
                        return false;
                    }

                    len = a.length;
                    if (len !== b.length) { // safe and faster
                        return false;
                    }
                    for (i = 0; i < len; i++) {
                        if (!innerEquiv(a[i], b[i])) {
                            return false;
                        }
                    }
                    return true;
                },

                "object": function (b, a) {
                    var i;
                    var eq = true; // unless we can proove it
                    var aProperties = [], bProperties = []; // collection of strings

                    // comparing constructors is more strict than using instanceof
                    if (a.constructor !== b.constructor) {
                        return false;
                    }

                    // stack constructor before traversing properties
                    callers.push(a.constructor);

                    for (i in a) { // be strict: don't ensures hasOwnProperty and go deep

                        aProperties.push(i); // collect a's properties

                        if (!innerEquiv(a[i], b[i])) {
                            eq = false;
                        }
                    }

                    callers.pop(); // unstack, we are done

                    for (i in b) {
                        bProperties.push(i); // collect b's properties
                    }

                    // Ensures identical properties name
                    return eq && innerEquiv(aProperties, bProperties);
                }
            };
        } ();

        innerEquiv = function () { // can take multiple arguments
            var args = Array.prototype.slice.apply(arguments);
            if (args.length < 2) {
                return true; // end transition
            }

            return (function (a, b) {
                if (a === b) {
                    return true; // catch the most you can
                } else if (a === null || b === null || typeof a === "undefined" || typeof b === "undefined" || hoozit(a) !== hoozit(b)) {
                    return false; // don't lose time with error prone cases
                } else {
                    return bindCallbacks(a, callbacks, [b, a]);
                }

                // apply transition with (1..n) arguments
            })(args[0], args[1]) && arguments.callee.apply(this, args.splice(1, args.length - 1));
        };

        return innerEquiv;

    } ();

    ///////////////////////////////////////////////////////////////////////////////
    //
    //   Assign globals to the global object.
    //   These are underscore prefixed because they are called by functions of the same name outside the
    //   framework closure. This is done because window.verify will not be overwritten by a
    //   function verify() {} in global scope, which many old tests depend on.
    //
    ///////////////////////////////////////////////////////////////////////////////
    window._verify = verify;
    window._assert = assert;
    window._fail = fail;
    window.getLocalizedError = Utils.getLocalizedError;

    window.Run = runner.run;
    window.runner = runner;
    window.TestCase = TestCase;
    window.BaselineTestCase = BaselineTestCase;
    window.CrossContextTest = CrossContextTest;
    window.logger = windowLogger;
    // globals for versioning
    window.IE7STANDARDSMODE = Utils.IE7STANDARDSMODE;
    window.IE8STANDARDSMODE = Utils.IE8STANDARDSMODE;
    window.IE9STANDARDSMODE = Utils.IE9STANDARDSMODE;
    window.IE10STANDARDSMODE = Utils.IE10STANDARDSMODE;
    window.IE11STANDARDSMODE = Utils.IE11STANDARDSMODE;
    window.IE12STANDARDSMODE = Utils.IE12STANDARDSMODE;
    window.getHOSTMode = Utils.getHOSTMode;

    window.X86MODE = Utils.X86MODE;
    window.X64MODE = Utils.X64MODE;
    window.ARMMODE = Utils.ARMMODE;
    window.getIEArchitecture = Utils.getIEArchitecture;

    window.Utils = Utils;

    // Function returns true/false based on equality, same as verify, but excludes logging. 
    Utils.equiv = equiv;



    // An object to hold various implementational details of our engine that our test cases rely upon
    window.Chakra = {
        // The amount of properties that have to be declared to transition object from path to
        // dictionary type.
        PathToDictionaryTransitionThreshold: 16
    }

    // if this is called, set up globals required for old test cases.
    window.VBS_apglobal_init = function () {
        // Old test cases require conditional compilation in a few cases, so turn it on.
        //@cc_on
        var proxy = new OldGlueProxy();
        window.apInitTest = proxy.apInitTest;
        window.apInitScenario = proxy.apInitScenario;
        window.apEndScenario = proxy.apEndScenario;
        window.apLogFailInfo = proxy.apLogFailInfo;
        window.apEndTest = proxy.apEndTest;
        window.apWriteDebug = proxy.apWriteDebug;
        window.VBS_apglobal_init = proxy.VBS_apglobal_init;
        window.apGetLocale = proxy.apGetLocale;
        proxy.VBS_apglobal_init();
    }

    // It used to be defined via 'function VBS_apglobal_init()'
    // 'window.apGlobalObj' has to be defined
    if (typeof window.apGlobalObj == "undefined") {
        try {
            // Define the object
            window.apGlobalObj = createActiveXObject("apgloballib.apglobal");
        } catch (e) {
            // Cannot instantiate ActiveXObject
            window.apGlobalObj = null;
        }
    }

})(this);

// Default version of these verification functions simply call the framework version of them.
verify = _verify;
assert = _assert;
fail = _fail;

// Disable simple print from doing anything
VBS_apglobal_init();
function ScriptEngineMajorVersion() { return 5; }
function ScriptEngineMinorVersion() { return 8; }
apGlobalObj.osSetIntlValue = function() { return 0; }
apGlobalObj.osGetIntlValue = function() { return 0; }
