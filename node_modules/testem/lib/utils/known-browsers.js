'use strict';

const browserArgs = require('./browser-args');
const path = require('path');
const fs = require('fs');
const isWin = require('./is-win');

function chromeWinPaths(homeDir, name) {
  return [
    homeDir + '\\Local Settings\\Application Data\\Google\\' + name + '\\Application\\chrome.exe',
    homeDir + '\\AppData\\Local\\Google\\' + name + '\\Application\\chrome.exe',
    'C:\\Program Files\\Google\\' + name + '\\Application\\Chrome.exe',
    'C:\\Program Files (x86)\\Google\\' + name + '\\Application\\Chrome.exe'
  ];
}

function chromeOSXPaths(name) {
  return [
    process.env.HOME + '/Applications/' + name + '.app/Contents/MacOS/' + name,
    '/Applications/' + name + '.app/Contents/MacOS/' + name
  ];
}

function chromeArgs(browserTmpDir, url) {
  return [
    '--user-data-dir=' + browserTmpDir,
    '--no-default-browser-check',
    '--no-first-run',
    '--ignore-certificate-errors',
    '--test-type',
    '--disable-renderer-backgrounding',
    '--disable-background-timer-throttling',
    url
  ];
}

function headlessChromeArgs(browserTmpDir, url) {
  return [
    '--headless',
    '--disable-gpu',
    '--disable-software-rasterizer',
    '--mute-audio',
    '--remote-debugging-port=0',
    '--window-size=1440,900'].concat(chromeArgs(browserTmpDir, url));
}

// Return the catalogue of the browsers that Testem supports for the platform. Each 'browser object'
// will contain these fields:
//
// * `name` - the display name of the browser
// * `exe` - path to the executable to use to launch the browser
// * `setup(app, done)` - any initial setup needed before launching the executable(this is async,
//        the second parameter `done()` must be invoked when done).
// * `supported(cb)` - an async function which tells us whether the browser is supported by the current machine.
module.exports = function knownBrowsers(platform, config) {
  let userHomeDir = config.getHomeDir();

  let browsers = [
    {
      name: 'Firefox',
      possiblePath: [
        'C:\\Program Files\\Mozilla Firefox\\firefox.exe',
        'C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe',
        process.env.HOME + '/Applications/Firefox.app/Contents/MacOS/firefox',
        '/Applications/Firefox.app/Contents/MacOS/firefox'
      ],
      possibleExe: 'firefox',

      args(config, url) {
        return ['-profile', this.browserTmpDir(), url];
      },
      setup(config, done) {
        // use user.js instead of prefs.js http://kb.mozillazine.org/User.js_file
        let perfJS = path.join(this.browserTmpDir(), 'user.js');

        if (config.get('firefox_user_js')) {
          return fs.readFile(config.get('firefox_user_js'), (err, content) => {
            if (err) {
              return done(err);
            }

            fs.writeFile(perfJS, content, done);
          });
        }

        // using user.js to suppress the check default browser popup
        // and the welcome start page
        let prefs = [
          'user_pref("browser.shell.checkDefaultBrowser", false);',
          'user_pref("browser.cache.disk.smart_size.first_run", false);',
          'user_pref("datareporting.policy.dataSubmissionEnabled", false);',
          'user_pref("datareporting.policy.dataSubmissionPolicyNotifiedTime", "1481830156314");'
        ];
        fs.writeFile(perfJS, prefs.join('\n'), done);
      }
    },
    {
      name: 'Chrome',
      possiblePath: chromeWinPaths(userHomeDir, 'Chrome').concat(chromeOSXPaths('Google Chrome')),
      possibleExe: [
        'google-chrome',
        'google-chrome-stable',
        'chrome'
      ],
      args(config, url) {
        return chromeArgs(this.browserTmpDir(), url);
      }
    },
    {
      name: 'Headless Chrome',
      possiblePath: chromeWinPaths(userHomeDir, 'Chrome').concat(chromeOSXPaths('Google Chrome')),
      possibleExe: [
        'google-chrome',
        'google-chrome-stable',
        'chrome'
      ],
      args(config, url) {
        return headlessChromeArgs(this.browserTmpDir(), url);
      }
    },

    // On Windows, Chrome Stable/Beta/Dev are installed on the same path
    {
      name: 'Chrome Beta',
      possiblePath: chromeOSXPaths('Google Chrome Beta'),
      possibleExe: [
        'google-chrome-beta'
      ],
      args(config, url) {
        return chromeArgs(this.browserTmpDir(), url);
      }
    },
    {
      name: 'Headless Chrome Beta',
      possiblePath: chromeOSXPaths('Google Chrome Beta'),
      possibleExe: [
        'google-chrome-beta'
      ],
      args(config, url) {
        return headlessChromeArgs(this.browserTmpDir(), url);
      }
    },
    {
      name: 'Chrome Dev',
      possiblePath: chromeOSXPaths('Google Chrome Dev'),
      possibleExe: [
        'google-chrome-unstable'
      ],
      args(config, url) {
        return chromeArgs(this.browserTmpDir(), url);
      }
    },

    // Chrome Canary is not available on Linux
    {
      name: 'Chrome Canary',
      possiblePath: chromeWinPaths(userHomeDir, 'Chrome SxS').concat(chromeOSXPaths('Google Chrome Canary')),
      args(config, url) {
        return chromeArgs(this.browserTmpDir(), url);
      }
    },
    {
      name: 'Chromium',
      possiblePath: chromeWinPaths(userHomeDir, 'Chromium').concat(chromeOSXPaths('Chromium')),
      possibleExe: [
        'chromium-browser',
        'chromium'
      ],
      args(config, url) {
        return chromeArgs(this.browserTmpDir(), url);
      }
    },
    {
      name: 'Safari',
      possiblePath: [
        'C:\\Program Files\\Safari\\safari.exe',
        'C:\\Program Files (x86)\\Safari\\safari.exe',
        process.env.HOME + '/Applications/Safari.app/Contents/MacOS/Safari',
        '/Applications/Safari.app/Contents/MacOS/Safari'
      ],
      setup(config, done) {
        let url = this.getUrl();
        fs.writeFile(
          path.join(this.browserTmpDir(), 'start.html'),
          '<script>window.location = \'' + url + '\'</script>',
          done
        );
      },
      args() {
        return [path.join(this.browserTmpDir(), 'start.html')];
      }
    },
    {
      name: 'Opera',
      possiblePath: [
        'C:\\Program Files\\Opera\\opera.exe',
        'C:\\Program Files (x86)\\Opera\\opera.exe',
        process.env.HOME + '/Applications/Opera.app/Contents/MacOS/Opera',
        '/Applications/Opera.app/Contents/MacOS/Opera'
      ],
      args(config, url) {
        let operaDataDir = this.browserTmpDir();
        return ['--user-data-dir=' + operaDataDir, '-pd', operaDataDir, url];
      }
    },
    {
      name: 'PhantomJS',
      possibleExe: 'phantomjs',
      args() {
        let launch_script = config.get('phantomjs_launch_script');
        if (!launch_script) {
          launch_script = path.resolve(__dirname, '../../assets/phantom.js');
        }
        let options = [launch_script, this.getUrl()];
        let debug_port = config.get('phantomjs_debug_port');
        if (debug_port) {
          options.unshift('--remote-debugger-autorun=true');
          options.unshift('--remote-debugger-port=' + debug_port);
        }
        let phantom_args = config.get('phantomjs_args');
        if (phantom_args) {
          options = phantom_args.concat(options);
        }
        return options;
      }
    }
  ];

  if (isWin(platform)) {
    browsers.push({
      name: 'IE',
      possiblePath: 'C:\\Program Files\\Internet Explorer\\iexplore.exe'
    });
  } else {
    browsers.push({
      name: 'Safari Technology Preview',
      possiblePath: [
        process.env.HOME + '/Applications/Safari Technology Preview.app/Contents/MacOS/Safari Technology Preview',
        '/Applications/Safari Technology Preview.app/Contents/MacOS/Safari Technology Preview'
      ],
      setup(config, done) {
        let url = this.getUrl();
        fs.writeFile(
          path.join(this.browserTmpDir(), 'start.html'),
          '<script>window.location = \'' + url + '\'</script>',
          done
        );
      },
      args() {
        return [path.join(this.browserTmpDir(), 'start.html')];
      }
    });
  }

  // Add user-defined configs to browser objects
  let browserPaths = config.get('browser_paths'); // Add config defined 'path' for binary
  let browserExes = config.get('browser_exes');  // Add config defined 'exe' for binary
  if (browserPaths || browserExes) {
    browsers.forEach(browserObject => {
      let browserPath = browserPaths ? browserPaths[browserObject.name] : undefined;
      let browserExe = browserExes ? browserExes[browserObject.name] : undefined;
      if (browserPath) {
        browserObject.possiblePath = browserPath;
      }
      if (browserExe) {
        browserObject.possibleExe = browserExe;
      }
    });
  }

  return browserArgs.addCustomArgs(browsers, config);
};
