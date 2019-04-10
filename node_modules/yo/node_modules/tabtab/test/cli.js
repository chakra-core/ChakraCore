const assert = require('assert');
const cli    = require('gentle-cli');

const path = require('path');

const tabpath = path.join(__dirname, '../bin/tabtab');

function tabtab(cmd) {
  return cli().use(cmd ? 'node ' + tabpath + ' ' + cmd : 'node ' + tabpath + '');
}

describe('CLI', () => {
  it('outputs help', (done) => {
    tabtab()
      .expect(0, 'tabtab <command>')
      .expect(0, 'Commands:')
      .end((err, res) => {
        done(err);
      });
  });

  it('install stdout', (done) => {
    tabtab('install --stdout')
      .expect('complete -o default -F _tabtab_completion tabtab')
      .expect('compdef _tabtab_completion tabtab')
      .expect('compctl -K _tabtab_completion tabtab')
      .end(done);
  });

  it('install prompt', (done) => {
    tabtab('install')
      // Prompt first answer, output to stdout
      .prompt(/Where do you want/, '\n')
      .expect('tabtab_completion ')
      .end(done);
  });
});
