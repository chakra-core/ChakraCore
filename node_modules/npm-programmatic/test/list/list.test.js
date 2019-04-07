var npm = require("../../index");

describe("Test listing of installed packages", ()=>{
	it("should list all installed packages by npm-programmatic", function(done){
		this.timeout(5000);
		npm.list('.')
		.then(function(packages){
			for(var i in packages){
				if(packages[i].indexOf('bluebird')!=-1){
					return done();
				}
			}
			return done(new Error("bluebird not found!"));
		})
		.catch(function(err, stderr){
			return done(err, stderr);
		})
	});
});
