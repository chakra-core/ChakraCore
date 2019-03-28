var fs      = require('fs')
var Diff    = require('../../');
var expect  = require('chai').expect;

describe("Package Dependency Diff", function () {
  describe("Chainable Interface", function () {
    var left;
    var right;
    beforeEach(function () {
      left = JSON.parse(fs.readFileSync(__dirname + '/' + 'left.json'))
      right = JSON.parse(fs.readFileSync(__dirname + '/' + 'right.json'))
    })
    it('should set the left package.json', function (done) {
      var chain = Diff().left(left);
      expect(chain)
        .to.have.property('_files')
          .and.to.have.property('left')
            .and.to.deep.equal(Diff.stripDeps(left))
            .and.to.not.deep.equal(Diff.stripDeps(right))
      done();
    })
    it('should set the right package.json', function (done) {
      var chain = Diff().right(right);
      expect(chain)
        .to.have.property('_files')
          .and.to.have.property('right')
            .and.to.deep.equal(Diff.stripDeps(right))
            .and.to.not.deep.equal(Diff.stripDeps(left))
      done();
    })

    it('should set both package.jsons', function (done) {
      var chain = Diff()
                    .left(left)
                    .right(right)
      expect(chain)
        .to.have.property('_files')
          .and.to.have.property('left')
            .and.to.deep.equal(Diff.stripDeps(left))
            .and.to.not.deep.equal(Diff.stripDeps(right))

      expect(chain)
        .to.have.property('_files')
          .and.to.have.property('right')
            .and.to.deep.equal(Diff.stripDeps(right))
            .and.to.not.deep.equal(Diff.stripDeps(left))

      done();
    })

    describe('method toObject', function () {
      it('should return an object', function (done) {
        var diffObj = Diff()
                        .left(left)
                        .right(right)
                        .toObject();
        expect(diffObj).to.be.an('object')
        expect(diffObj)
          .to.have.property('dependencies')
            .and.to.have.property('length')
              .and.to.equal(2)
        expect(diffObj)  
          .to.have.property('devDependencies')
            .and.to.have.property('length')
              .and.to.equal(2)
        expect(diffObj) 
          .to.have.property('optionalDependencies')
            .and.to.have.property('length')
              .and.to.equal(3)

        expect(diffObj.dependencies[0].operation).to.equal('edit')
        expect(diffObj.dependencies[1].operation).to.equal('new')

        expect(diffObj.devDependencies[0].operation).to.equal('edit')
        expect(diffObj.devDependencies[1].operation).to.equal('new')

        expect(diffObj.optionalDependencies[0].operation).to.equal('edit')
        expect(diffObj.optionalDependencies[1].operation).to.equal('delete')
        expect(diffObj.optionalDependencies[2].operation).to.equal('new')

        done();
      })
    })

    describe('method toCmdList', function () {
      it('should return a valid array of npm commands', function (done) {
        var diffObj = Diff()
                        .left(left)
                        .right(right)
                        .toCmdList();
        expect(diffObj).to.be.an('array')
        expect(diffObj).to.have.property('length').and.to.equal(7)

        expect(diffObj[5].indexOf('uninstall') !== -1).to.equal(true);

        expect(diffObj[0].indexOf('install') !== -1).to.equal(true);
        expect(diffObj[1].indexOf('install') !== -1).to.equal(true);
        expect(diffObj[2].indexOf('install') !== -1).to.equal(true);
        expect(diffObj[3].indexOf('install') !== -1).to.equal(true);
        expect(diffObj[4].indexOf('install') !== -1).to.equal(true);
        expect(diffObj[6].indexOf('install') !== -1).to.equal(true);
        done();
      })
    });
  })
})