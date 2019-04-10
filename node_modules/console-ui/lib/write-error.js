'use strict';
const chalk = require('chalk');
const fs = require('fs');
const summarizeProcess = require('./summarize-process');

function extractBroccoliError(error) {
  let broccoliPayload = error.broccoliPayload || { error: {} };
  let broccoliNode = broccoliPayload.broccoliNode || {};

  let defaultLocation = {
    file: error.file || error.filename, // Uglify errors have `filename` instead
    line: error.line,
    column: error.col
  };

  return {
    name: error.name,
    code: error.code,
    message: error.message,
    stack: error.stack,
    broccoliBuilderErrorStack: broccoliPayload.error.stack,
    errorMessage: error.message,
    originalErrorMessage: broccoliPayload.error.message,
    codeFrame: broccoliPayload.error.codeFrame,
    nodeName: broccoliNode.nodeName,
    nodeAnnotation: broccoliNode.nodeAnnotation,
    errorType: broccoliPayload.error.errorType,
    location: broccoliPayload.error.location || defaultLocation
  };
}

// Removes temp folder path from the file name
function cleanupFileName(fileName) {
  if (!fileName) {
    return;
  }

  let tmpPosition = fileName.indexOf('.tmp/');

  if (tmpPosition !== -1) {
    return fileName.substring(tmpPosition + 5);
  }

  return fileName;
}

module.exports = function writeError(ui, error) {
  if (!error) {
    return null;
  }

  let extracted = extractBroccoliError(error);
  let stack = extracted.stack;
  let location = extracted.location;
  let codeFrame = extracted.codeFrame;
  let originalErrorMessage = extracted.originalErrorMessage;
  let errorType = extracted.errorType;
  let nodeName = extracted.nodeName;
  let errorMessage = extracted.errorMessage;

  let fileName = cleanupFileName(location.file);
  if (fileName) {
    if (location.line) {
      fileName += typeof location.column !== 'undefined'
        ? ':' + location.line + ':' + location.column
        : ':' + location.line;
    }
  }

  let title = '';
  if (errorType) {
    title = errorType

    if (nodeName) {
      title += ` (${nodeName})`;
    }
  }

  if (fileName) {
    if (title !== '') {
      title += ` in ${fileName}`;
    } else {
      title = `File: ${fileName}`
    }
  }

  if (title) {
    ui.writeLine(chalk.red(title), 'ERROR');
    ui.writeLine('', 'ERROR'); // Empty line
  }

  if (codeFrame) {
    if (originalErrorMessage && originalErrorMessage !== codeFrame) {
      ui.writeLine(chalk.red(originalErrorMessage), 'ERROR');
      ui.writeLine('', 'ERROR'); // Empty line
    }

    ui.writeLine(chalk.red(codeFrame), 'ERROR');
  } else if (errorMessage) {
    ui.writeLine(chalk.red(errorMessage), 'ERROR');
  } else {
    ui.writeLine(chalk.red(error), 'ERROR');
  }
  ui.writeLine('', 'ERROR'); // Empty line

  if (stack) {
    ui.writeLine(stack, 'DEBUG');
  }

  return `=================================================================================

ENV Summary:
${summarizeProcess(process)}
ERROR Summary:
${summarizeProcess.obj(extracted)}

=================================================================================
`;
}
