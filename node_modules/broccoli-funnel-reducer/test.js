var data = [
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'animated-overlay.gif',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_flat_30_cccccc_40x100.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_flat_50_5c5c5c_40x100.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_glass_20_555555_1x400.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_glass_40_0078a3_1x400.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_glass_40_ffc73d_1x400.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_gloss-wave_25_333333_500x100.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_highlight-soft_80_eeeeee_1x100.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_inset-soft_25_000000_1x100.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-bg_inset-soft_30_f58400_1x100.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-icons_222222_256x240.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-icons_4b8e0b_256x240.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-icons_a83300_256x240.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-icons_cccccc_256x240.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/jquery-ui/themes/ui-darkness/images',
    file: 'ui-icons_ffffff_256x240.png',
    dest: '/assets/images'
  },
  {
    src: 'bower_components/ember-qunit-notifications',
    file: 'passed.png',
    dest: '/assets'
  },
  {
    src: 'bower_components/ember-qunit-notifications',
    file: 'failed.png',
    dest: '/assets'
  },
  {
    src: 'bower_components/ember-data',
    file: 'ember-data.js.map',
    dest: 'assets'
  },
  {
    src: 'bower_components/material-design-icons/iconfont',
    file: 'MaterialIcons-Regular.woff2',
    dest: 'assets/fonts'
  },
  {
    src: 'bower_components/material-design-icons/iconfont',
    file: 'MaterialIcons-Regular.woff',
    dest: 'assets/fonts'
  },
  {
    src: 'bower_components/material-design-icons/iconfont',
    file: 'MaterialIcons-Regular.ttf',
    dest: 'assets/fonts'
  },
  {
    src: 'bower_components/material-design-icons/iconfont',
    file: 'MaterialIcons-Regular.eot',
    dest: 'assets/fonts'
  },
  {
    src: 'bower_components/zeroclipboard/dist',
    file: 'ZeroClipboard.swf',
    dest: 'assets'
  }
];


var expect = require('chai').expect;
var funnelReducer = require('./');

describe('funnelReducer', function() {
  it('reduces noop scenario', function() {
    expect(funnelReducer([])).to.deep.equal([ ]);
  });

 it('reduces basic scenario', function() {
    expect(funnelReducer([{
      src: 'foo',
      dest: 'bar',
      file: 'apple'
    }])).to.deep.equal([{
      srcDir: 'foo',
      destDir: 'bar',
      include: ['apple']
    }]);
  });

 it('reduces basic scenario (With possible reductions)', function() {
    expect(funnelReducer([
      {
        src: 'foo',
        dest: 'bar',
        file: 'apple'
      },
      {
        src: 'foo',
        dest: 'bar',
        file: 'pie'
      }
    ])).to.deep.equal([{
      srcDir: 'foo',
      destDir: 'bar',
      include: ['apple', 'pie']
    }]);
  });

 it('reduces overlap scenario', function() {
    expect(funnelReducer([
      {
        src: 'foo/bar',
        dest: 'baz',
        file: 'apple'
      },
      {
        src: 'foo',
        dest: 'bar/baz',
        file: 'pie'
      }
    ])).to.deep.equal([
      {
        srcDir: 'foo/bar',
        destDir: 'baz',
        include: ['apple'],
      },
      {
        srcDir: 'foo',
        destDir: 'bar/baz',
        include: ['pie']
      }
    ]);
  });


  it('reduces complex scenario', function() {
    expect(funnelReducer(data)).to.deep.equal([
      {
        "destDir": "/assets/images",
        "include": [
          "animated-overlay.gif",
          "ui-bg_flat_30_cccccc_40x100.png",
          "ui-bg_flat_50_5c5c5c_40x100.png",
          "ui-bg_glass_20_555555_1x400.png",
          "ui-bg_glass_40_0078a3_1x400.png",
          "ui-bg_glass_40_ffc73d_1x400.png",
          "ui-bg_gloss-wave_25_333333_500x100.png",
          "ui-bg_highlight-soft_80_eeeeee_1x100.png",
          "ui-bg_inset-soft_25_000000_1x100.png",
          "ui-bg_inset-soft_30_f58400_1x100.png",
          "ui-icons_222222_256x240.png",
          "ui-icons_4b8e0b_256x240.png",
          "ui-icons_a83300_256x240.png",
          "ui-icons_cccccc_256x240.png",
          "ui-icons_ffffff_256x240.png",
        ],
        "srcDir": "bower_components/jquery-ui/themes/ui-darkness/images"
      },
      {
        "destDir": "/assets",
        "include": [
          "passed.png",
          "failed.png",
        ],
        "srcDir": "bower_components/ember-qunit-notifications"
      },
      {
        "destDir": "assets",
        "include": [
          "ember-data.js.map"
        ],
        "srcDir": "bower_components/ember-data"
      },
      {
        "destDir": "assets/fonts",
        "include": [
          "MaterialIcons-Regular.woff2",
          "MaterialIcons-Regular.woff",
          "MaterialIcons-Regular.ttf",
          "MaterialIcons-Regular.eot",
        ],
        "srcDir": "bower_components/material-design-icons/iconfont",
      },
      {
        "destDir": "assets",
        "include": [
          "ZeroClipboard.swf"
        ],
        "srcDir": "bower_components/zeroclipboard/dist"
      }
    ]);
  });
});
