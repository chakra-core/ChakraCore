'use strict';

const printSlowNodes = require('broccoli-slow-trees');

module.exports = {
  onBuildSuccess(builder) {
    printSlowNodes(builder.outputNodeWrapper.__heimdall__);
    console.log(
      'Built - ' +
        Math.round(builder.outputNodeWrapper.buildState.totalTime) +
        ' ms @ ' +
        new Date().toString()
    );
  },

  onBuildFailure(err) {
    console.log('Built with error:');
    console.log(err.message);
    if (!err.broccoliPayload || !err.broccoliPayload.location.file) {
      console.log('');
      console.log(err.stack);
    }
    console.log('');
  },
};
