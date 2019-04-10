'use strict';

const Task = require('../models/task');
const AssetSizePrinter = require('../models/asset-size-printer');

class ShowAssetSizesTask extends Task {
  run(options) {
    let sizePrinter = new AssetSizePrinter({
      ui: this.ui,
      outputPath: options.outputPath,
    });

    if (options.json) {
      return sizePrinter.printJSON();
    }

    return sizePrinter.print();
  }
}

module.exports = ShowAssetSizesTask;
