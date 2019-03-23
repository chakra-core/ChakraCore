'use strict';

const exec = c => require('child_process').execSync(c).toString();
const assert = require('assert');
const path = require('path');
const fs = require('fs');
let cmd = `node ${path.resolve(__dirname+'/../bin/beautify')}`;

describe('CLI', function() {
    it('should beautify css', function() {
        assert.equal(exec(`echo "html{color:white}body{border: '1px'}" | ${cmd} -f css`), 
`html {
    color: white;
}

body {
    border: '1px';
}
`
        );
    });

    it('should add semicolons to css', function() {
        assert.equal(exec(`echo "body{color:white}" | ${cmd} -f css`), 
`body {
    color: white;
}
`
        );
    });

    it('should beautify JSON', function() {
        assert.equal(exec(`echo '{"color":"white","a":[1,2,3, {"a":1}]}' | ${cmd}`), 
`{
    "color": "white",
    "a": [1, 2, 3, {
        "a": 1
    }]
}
`
        );
    });

    it('should beautify html', function() {
        assert.equal(exec(`echo "<!DOCTYPE html><html><head></head><body><p>test</p></body></html>" | ${cmd} -f html`), 
`<!DOCTYPE html>
<html>
    
    <head></head>
    
    <body>
        <p>test</p>
    </body>

</html>
`
        );
    });

    it('should correctly save file', function() {
        exec(`echo "<!DOCTYPE html><html><head></head><body><p>test</p></body></html>" | ${cmd} -f html -o ./test/test.html`);
        assert.equal(fs.readFileSync('./test/test.html', 'utf8'),
`<!DOCTYPE html>
<html>
    
    <head></head>
    
    <body>
        <p>test</p>
    </body>

</html>`
        );
    });

    it('should correctly update file', function() {
        exec(`echo "<!DOCTYPE html><html><head></head><body><p>test123</p></body></html>" | ${cmd} -f html -o ./test/test.html`);
        assert.equal(fs.readFileSync('./test/test.html', 'utf8'),
`<!DOCTYPE html>
<html>
    
    <head></head>
    
    <body>
        <p>test123</p>
    </body>

</html>`
        );
    });
    
    it('should create folder when working with multiple files', function() {
        exec(`${cmd} -o ./test/test2 ./test/mock/test1.json ./test/mock/test2.json`);
        assert.deepEqual(fs.readdirSync('./test/test2'), ['test1.json', 'test2.json']);
    });

    it('should update folder contents', function() {
        exec(`${cmd} -o ./test/test2 ./test/mock2/test3.json ./test/mock2/test4.json`);
        assert.deepEqual(fs.readdirSync('./test/test2'), ['test1.json', 'test2.json','test3.json', 'test4.json']);
    });
});
