'use strict';

const ScrollableTextPanel = require('./scrollable_text_panel');
const Chars = require('../../chars');

module.exports = ScrollableTextPanel.extend({
  initialize(attrs) {
    this.updateVisibility();
    this.on('change:text', () => this.updateVisibility());
    ScrollableTextPanel.prototype.initialize.call(this, attrs);
  },
  updateVisibility() {
    let text = this.get('text');
    this.set('visible', !!text);
  },
  render() {
    let  visible = this.get('visible');
    if (!visible) {
      return;
    }

    ScrollableTextPanel.prototype.render.call(this);

    // draw a box
    let line = this.get('line');
    let col = this.get('col') - 1;
    let width = this.get('width') + 1;
    let height = this.get('height') + 1;

    // TODO Figure this out
    if (isNaN(width)) {
      return;
    }

    let screen = this.get('screen');
    screen.foreground('red');
    screen.position(col, line);
    screen.write(Chars.topLeft + new Array(width).join(Chars.horizontal) + Chars.topRight);

    for (let l = line + 1; l < line + height; l++) {
      screen.position(col, l);
      screen.write(Chars.vertical);
      screen.position(col + width, l);
      screen.write(Chars.vertical);
    }

    screen.position(col, line + height);
    screen.write(Chars.bottomLeft + new Array(width).join(Chars.horizontal) + Chars.bottomRight);
    screen.display('reset');
  }
});
