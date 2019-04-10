const fs       = require('fs');
const path     = require('path');
const Complete = require('../src/complete');
const assert   = require('assert');

describe('Complete', () => {

  beforeEach(() => {
    this.complete = new Complete({
      name: 'tabtab',
      cache: false
    });
  });

  it('Complete#new', () => {
    assert.ok(this.complete instanceof require('events').EventEmitter);
  });

  it('Complete#parseEnv', () => {
    var env = this.complete.parseEnv();
    assert.ok(env.args && env.args.length);
    assert.ok(!env.complete);
    assert.strictEqual(env.words, 0);
    assert.strictEqual(env.point, 0);
    assert.strictEqual(env.line, '');

    env = this.complete.parseEnv({
      env: {
        COMP_CWORD: '3',
        COMP_POINT: '5',
        COMP_LINE: 'foo bar --foobar'
      }
    });

    assert.strictEqual(env.words, 3);
    assert.strictEqual(env.point, 5);
    assert.strictEqual(env.line, 'foo bar --foobar');

    assert.equal(env.partial, 'foo b');
    assert.equal(env.last, '--foobar');
    assert.equal(env.lastPartial, 'b');
    assert.equal(env.prev, 'bar');
  });

  describe('Complete#handle cache off', () => {
    it('Emits appropriate event', (done) => {
      this.complete.handle({
        _: ['completion', '--', 'tabtab'],
        cache: false,
        env: {
          COMP_LINE: 'tabtab',
          COMP_CWORD: 'tabtab',
          COMP_POINT: 0
        }
      });


      this.complete.on('tabtab', (data, callback) => {
        assert.equal(data.line, 'tabtab');
        callback(null, ['foo', 'bar']);
        done();
      });
    });
  });

  describe('Complete#handle cache on', () => {
    beforeEach((done) => {
      this.cachefile = path.join(__dirname, '../.completions/cache.json');

      let next = (err) => {
        if (err) return done(err);

        this.complete = new Complete({
          name: 'tabtab',
          _: ['completion', '--', 'tabtab'],
          env: {
            COMP_LINE: 'tabtab',
            COMP_CWORD: 'tabtab',
            COMP_POINT: 0
          },

          ttl: 100
        });

        done();
      }

      fs.stat(this.cachefile, (err, stats) => {
        if (err && err.code === 'ENOENT') return next();
        else if (err) return next(err);

        fs.unlink(this.cachefile, next);
      });
    });

    it('Emits appropriate event and writes response to cache', (done) => {
      this.complete.handle();

      this.complete.on('tabtab', (env, callback) => {
        assert.equal(env.line, 'tabtab');
        var results = ['foo', 'bar', 'baz'];
        callback(null, results);

        this.complete.handle();
        this.complete.on('tabtab:cache', (env) => {
          assert.equal(env.line, 'tabtab');

          var cache = JSON.parse(fs.readFileSync(this.cachefile, 'utf8')).cache;
          assert.deepEqual(cache.tabtab.value, ['foo', 'bar', 'baz']);
          done();
        });
      });
    });

    describe('TTL duration', () => {
      it('Honors TTL duration', (done) => {
        this.complete.handle();

        this.complete.on('tabtab', (env, callback) => {
          assert.equal(env.line, 'tabtab');
          var results = ['foo', 'bar', 'baz'];
          callback(null, results);

          setTimeout(() => {
            this.complete.handle();
            this.complete.on('tabtab:cache', (env) => {
              assert.equal(env.line, 'tabtab');

              var cache = JSON.parse(fs.readFileSync(this.cachefile, 'utf8')).cache;
              assert.ok(!cache.tabtab);
              done();
            });
          }, 300);
        });
      });
    });
  });


});
