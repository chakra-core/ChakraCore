# fast-plist [![Build Status](https://travis-ci.org/Microsoft/node-fast-plist.svg?branch=master)](https://travis-ci.org/Microsoft/node-fast-plist) [![Coverage Status](https://coveralls.io/repos/github/Microsoft/node-fast-plist/badge.svg?branch=master)](https://coveralls.io/github/Microsoft/node-fast-plist?branch=master)
A fast PLIST parser.


## Installing

```sh
npm install fast-plist
```

## Using

```javascript
var parse = require('fast-plist').parse;

console.log(
    parse(`
        <?xml version="1.0" encoding="UTF-8"?>
        <plist version="1.0">
        <dict>
            <key>name</key>
            <string>Brogrammer</string>
            <key>settings</key>
            <dict>
                <key>background</key>
                <string>#1a1a1a</string>
                <key>caret</key>
                <string>#ecf0f1</string>
                <key>foreground</key>
                <string>#ecf0f1</string>
                <key>invisibles</key>
                <string>#F3FFB51A</string>
                <key>lineHighlight</key>
                <string>#2a2a2a</string>
            </dict>
        </dict>
        </plist>`
    )
);
/* Output:
{
        "name": "Brogrammer",
        "settings": {
                "background": "#1a1a1a",
                "caret": "#ecf0f1",
                "foreground": "#ecf0f1",
                "invisibles": "#F3FFB51A",
                "lineHighlight": "#2a2a2a"
        }
}
*/
```

```javascript

parse(`bad string`);

/* Output:

Error: Near offset 1: expected < ~~~ad string~~~
*/
```

## Development

* `npm run watch`
* `npm run test`

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.


## License
[MIT](https://github.com/Microsoft/node-fast-plist/blob/master/LICENSE.md)
