/*

runner_tabs.js
==============

Implementation of the tabbed UI. Each tab contains its own log panel.
When the tab is not selected, it hides the associated log panel.

*/
'use strict';

const SplitLogPanel = require('./split_log_panel');
const View = require('./view');
const Backbone = require('backbone');
const pad = require('../../strutils').pad;
const Chars = require('../../chars');
const Screen = require('./screen');
const notifier = require('node-notifier');
const constants = require('./constants');
const TabWidth = constants.TabWidth;
const TabStartLine = constants.TabStartLine;
const TabHeight = constants.TabHeight;
const TabStartCol = constants.TabStartCol;
const RunnerTab = exports.RunnerTab = View.extend({
  defaults: {
    allPassed: true
  },
  col: TabStartCol,
  line: TabStartLine,
  height: TabHeight,
  width: TabWidth,
  initialize() {
    let runner = this.get('runner');
    let results = runner.get('results');
    let index = this.get('index');
    let appview = this.get('appview');
    let app = appview.app;
    let config = app.config;
    let self = this;
    let visible = appview.get('currentTab') === index;
    if (!this.get('screen')) {
      this.set('screen', new Screen());
    }

    this.splitPanel = new SplitLogPanel({
      runner: runner,
      appview: appview,
      visible: visible,
      screen: this.get('screen')
    });

    this.spinnerIdx = 0;

    function handleCurrentTab() {
      self.set('selected', appview.get('currentTab') === self.get('index'));
    }

    this.observe(appview, {
      'change:currentTab': handleCurrentTab
    });
    this.observe(runner, {
      'change:name'() {
        self.renderRunnerName();
      },
      'tests-start'() {
        self.set('allPassed', true);
        self.splitPanel.resetScrollPositions();
        self.startSpinner();
      },
      'tests-end'() {
        self.stopSpinner();
        self.renderResults();
        self.renderRunnerName();
        if (config.get('growl')) {
          self.growlResults();
        }
      },
      'change:allPassed'(model, value) {
        self.set('allPassed', value);
      }
    });

    if (results) {
      this.observe(results, {
        change() {
          let results = runner.get('results');
          if (!results) {
            self.set('allPassed', true);
          } else {
            let passed = results.get('passed');
            let total = results.get('total');
            let pending = results.get('pending');

            let allPassed = (passed + pending) === total;
            let hasTests = total > 0;
            let failCuzNoTests = !hasTests && config.get('fail_on_zero_tests');
            let hasError = runner.get('messages').filter(function(m) {
              return m.get('type') === 'error';
            }).length > 0;
            self.set('allPassed',
              allPassed && !failCuzNoTests && !hasError);
          }
        },
        'change:all'() {
          self.renderResults();
        }
      });
    }

    this.observe(appview, 'change:isPopupVisible', () => {
      this.updateSplitPanelVisibility();
    });

    this.observe(this, {
      'change:selected'() {
        self.updateSplitPanelVisibility();
      },
      'change:index change:selected'() {
        self.render();
      },
      'change:allPassed'() {
        process.nextTick(() => {
          self.renderRunnerName();
          self.renderResults();
        });
      }
    });
    this.render();

    handleCurrentTab();
  },

  updateSplitPanelVisibility() {
    let appview = this.get('appview');
    this.splitPanel.set('visible', this.get('selected') && !appview.isPopupVisible());
  },

  color() {
    let appview = this.get('appview');
    let config = appview.app.config;
    let runner = this.get('runner');
    let results = runner.get('results');
    let equal = true;
    let hasTests = false;
    let pending = false;
    if (results) {
      let passed = results.get('passed');
      pending = results.get('pending');
      let total = results.get('total');
      equal = (passed + pending) === total;
      hasTests = total > 0;
    }
    let failCuzNoTests = !hasTests && config.get('fail_on_zero_tests');
    let success = !failCuzNoTests && equal;
    return success ? (pending ? 'yellow' : 'green') : 'red';
  },

  startSpinner() {
    this.stopSpinner();
    let self = this;
    function render() {
      self.renderResults();
      self.setTimeoutID = setTimeout(render, 150);
    }
    render();
  },

  stopSpinner() {
    if (this.setTimeoutID) {
      clearTimeout(this.setTimeoutID);
    }
  },

  isPopupVisible() {
    let appview = this.get('appview');
    return appview && appview.isPopupVisible();
  },

  render() {
    if (this.isPopupVisible()) {
      return;
    }
    this.renderTab();
    this.renderRunnerName();
    this.renderResults();
  },

  renderRunnerName() {
    if (this.isPopupVisible()) {
      return;
    }

    let screen = this.get('screen');
    let index = this.get('index');
    let line = this.line;
    let width = this.width;
    let col = this.col + index * width;
    let runner = this.get('runner');
    let runnerName = runner.get('name');

    // write line 1
    screen
      .foreground(this.color());

    if (this.get('selected')) {
      screen.display('bright');
    }

    let runnerDisplayName = pad(runnerName || '', width - 2, ' ', 2);
    screen
      .position(col + 1, line + 1)
      .write(runnerDisplayName)
      .display('reset');
  },

  renderResults() {
    if (this.isPopupVisible()) {
      return;
    }

    let screen = this.get('screen');
    let index = this.get('index');
    let line = this.line;
    let width = this.width;
    let col = this.col + index * width;
    let runner = this.get('runner');
    let results = runner.get('results');
    let resultsDisplay = '';
    let equal = true;

    if (results) {
      let total = results.get('total');
      let passed = results.get('passed');
      let pending = results.get('pending');
      resultsDisplay = passed + '/' + total;
      equal = (passed + pending) === total;
    }

    if (results && results.get('all')) {
      resultsDisplay += ' ' + ((this.get('allPassed') && equal) ? Chars.success : Chars.fail);
    } else if (!results && runner.get('allPassed') !== undefined) {
      resultsDisplay = runner.get('allPassed') ? Chars.success : Chars.fail;
    } else {
      resultsDisplay += ' ' + Chars.spinner[this.spinnerIdx++];
      if (this.spinnerIdx >= Chars.spinner.length) {
        this.spinnerIdx = 0;
      }
    }

    resultsDisplay = pad(resultsDisplay, width - 4, ' ', 2);

    // write line 1
    screen
      .foreground(this.color());

    if (this.get('selected')) {
      screen.display('bright');
    }

    screen
      .position(col + 1, line + 2)
      .write(resultsDisplay)
      .display('reset');
  },

  growlResults() {
    let runner = this.get('runner');
    let results = runner.get('results');
    let name = runner.get('name');
    let resultsDisplay = results ?
      (results.get('passed') + '/' + results.get('total')) : 'finished';

    notifier.notify({
      title: 'Test\'em',
      message: name + ' : ' + resultsDisplay
    });
  },

  renderTab() {
    if (this.isPopupVisible()) {
      return;
    }
    if (this.get('selected')) {
      this.renderSelected();
    } else {
      this.renderUnselected();
    }
  },

  renderUnselected() {
    if (this.isPopupVisible()) {
      return;
    }

    let screen = this.get('screen');
    let index = this.get('index');
    let width = this.width;
    let height = this.height;
    let line = this.line;
    let col = this.col + index * width;
    let firstCol = index === 0;
    screen.position(col, line);

    screen.write(new Array(width + 1).join(' '));
    for (let i = 1; i < height - 1; i++) {
      if (!firstCol) {
        screen.position(col, line + i);
        screen.write(' ');
      }
      screen.position(col + width - 1, line + i);
      screen.write(' ');
    }

    let bottomLine = new Array(width + 1).join(Chars.horizontal);
    screen.position(col, line + height - 1);
    screen.write(bottomLine);
  },

  renderSelected() {
    if (this.isPopupVisible()) {
      return;
    }
    let screen = this.get('screen');
    let index = this.get('index');
    let width = this.width;
    let height = this.height;
    let line = this.line;
    let col = this.col + index * width;
    let firstCol = index === 0;
    screen.position(col, line);

    screen.write((firstCol ? Chars.horizontal : Chars.topLeft) +
      new Array(width - 1).join(Chars.horizontal) +
        Chars.topRight);
    for (let i = 1; i < height - 1; i++) {
      if (!firstCol) {
        screen.position(col, line + i);
        screen.write(Chars.vertical);
      }
      screen.position(col + width - 1, line + i);
      screen.write(Chars.vertical);
    }

    let bottomLine = (firstCol ? ' ' : Chars.bottomRight) +
      new Array(width - 1).join(' ') + Chars.bottomLeft;
    screen.position(col, line + height - 1);
    screen.write(bottomLine);
  },

  destroy() {
    this.stopSpinner();
    this.splitPanel.destroy();
    View.prototype.destroy.call(this);
  }
});

// View container for all the tabs. It'll handle clean up of removed tabs and draw
// the edge for where there are no tabs.
exports.RunnerTabs = Backbone.Collection.extend({
  model: RunnerTab,
  initialize(arr, attrs) {
    this.appview = attrs.appview;
    let self = this;
    this.screen = attrs.screen || new Screen();
    this.on('remove', (removed) => {
      let currentTab = self.appview.get('currentTab');
      if (currentTab >= self.length) {
        currentTab--;
        self.appview.set('currentTab', currentTab, {silent: true});
      }
      self.forEach((runner, idx) => {
        runner.set({
          index: idx,
          selected: idx === currentTab
        });
      });
      self.eraseLast();
      removed.destroy();
      if (self.length === 0) {
        self.blankOutBackground();
      }
    });
    this.appview.on('change:isPopupVisible change:lines change:cols', function() {
      self.reRenderAll();
    });
  },

  reRenderAll() {
    this.blankOutBackground();
    this.render();
  },

  blankOutBackground() {
    if (this.isPopupVisible()) {
      return;
    }
    let screen = this.screen;
    let cols = this.appview.get('cols');
    for (let i = 0; i < TabHeight; i++) {
      screen
        .position(0, TabStartLine + i)
        .write(pad('', cols, ' ', 1));
    }
  },

  render() {
    if (this.isPopupVisible()) {
      return;
    }
    this.invoke('render');
    if (this.length > 0) {
      this.renderLine();
    }
  },

  renderLine() {
    if (this.isPopupVisible()) {
      return;
    }
    let screen = this.screen;
    let startCol = this.length * TabWidth;
    let lineLength = this.appview.get('cols') - startCol + 1;
    if (lineLength > 0) {
      screen
        .position(startCol + 1, TabStartLine + TabHeight - 1)
        .write(new Array(lineLength).join(Chars.horizontal));
    }
  },

  eraseLast() {
    if (this.isPopupVisible()) {
      return;
    }
    let screen = this.screen;
    let index = this.length;
    let width = TabWidth;
    let height = TabHeight;
    let line = TabStartLine;
    let col = TabStartCol + index * width;

    for (let i = 0; i < height - 1; i++) {
      screen
        .position(col, line + i)
        .write(new Array(width + 1).join(' '));
    }

    let bottomLine = new Array(width + 1).join(Chars.horizontal);
    screen.position(col, line + height - 1);
    screen.write(bottomLine);
  },

  isPopupVisible() {
    let appview = this.appview;
    return appview && appview.isPopupVisible();
  }
});
