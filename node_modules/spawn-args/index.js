module.exports = function(args, opts){
	opts = opts || {}
	args = args || '';
	var arr = [];

	var current = null;
	var quoted = null;
	var quoteType = null;

	function addcurrent(){
		if(current){
			// trim extra whitespace on the current arg
			arr.push(current.trim());
			current = null;
		}
	}

	// remove escaped newlines
	args = args.replace(/\\\n/g, '');

	for(var i=0; i<args.length; i++){
		var c = args.charAt(i);

		if(c==" "){
			if(quoted){
				quoted += c;
			}
			else{
				addcurrent();
			}
		}
		else if(c == '\'' || c == '"'){
			if(quoted){
				quoted += c;
				// only end this arg if the end quote is the same type as start quote
				if (quoteType === c) {
					// make sure the quote is not escaped
					if (quoted.charAt(quoted.length - 2) !== '\\') {
						arr.push(quoted);
						quoted = null;
						quoteType = null;
					}
				}
			}
			else{
				addcurrent();
				quoted = c;
				quoteType = c;
			}
		}
		else{
			if(quoted){
				quoted += c;
			}
			else{
				if(current){
					current += c;
				}
				else{
					current = c;
				}
			}
		}
	}

	addcurrent();

	if(opts.removequotes){
		arr = arr.map(function(arg){
			if(opts.removequotes==='always'){
				return arg.replace(/^["']|["']$/g, "");
			}
			else{
				if(arg.match(/\s/)) return arg
				return arg.replace(/^"|"$/g, "");
			}
		});
	}

	return arr;
}
