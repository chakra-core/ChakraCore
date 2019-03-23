'use strict';

const fs = require('fs');
const path = require('path');

const handlebars = require('handlebars');
const errorTemplate = handlebars.compile(fs.readFileSync(path.resolve(__dirname, '..', 'templates/error.html')).toString());
const errorHandlerUtils = require('./error-handler-utils');

const toVersionString = errorHandlerUtils.toVersionString;

module.exports = function errorHandler(response, options) {
  // All errors thrown from builder.build() are guaranteed to be
  // Builder.BuildError instances.
  const buildError = options.buildError;
  const broccoliPayload = buildError.broccoliPayload;
  const broccoliNode = broccoliPayload.broccoliNode || {};
  const versions = broccoliPayload.versions || {};

  const versionString = toVersionString(versions);

  const context = {
    stack: broccoliPayload.error.stack,
    broccoliBuilderErrorStack: buildError.stack,
    instantiationStack: broccoliPayload.instantiationStack,
    errorMessage: buildError.message,
    liveReloadPath: options.liveReloadPath,
    codeFrame: broccoliPayload.error.codeFrame,
    nodeName: broccoliNode.nodeName,
    nodeAnnotation: broccoliNode.nodeAnnotation,
    errorType: broccoliPayload.error.errorType,
    location: broccoliPayload.error.location,
    versionString
  };
  response.setHeader('Content-Type', 'text/html');
  response.writeHead(500);
  response.end(errorTemplate(context));
}
