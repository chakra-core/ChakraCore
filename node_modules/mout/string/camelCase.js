var toString = require('../lang/toString');
var replaceAccents = require('./replaceAccents');
var removeNonWord = require('./removeNonWord');
var upperCase = require('./upperCase');
var lowerCase = require('./lowerCase');
    /**
    * Convert string to camelCase text.
    */
    function camelCase(str){
        str = toString(str);
        str = replaceAccents(str);
        str = removeNonWord(str)
            .replace(/[\-_]/g, ' '); // convert all hyphens and underscores to spaces

        // handle acronyms
        // matches lowercase chars && uppercase words
        if (/[a-z]/.test(str) && /^|\s[A-Z]+\s|$/.test(str)) {
            // we convert any word that isn't all caps into lowercase
            str = str.replace(/\s(\w+)/g, function(word, m) {
                return /^[A-Z]+$/.test(m) ? word : lowerCase(word);
            });
        } else if (/\s/.test(str)) {
            // if it doesn't contain an acronym and it has spaces we should
            // convert every word to lowercase
            str = lowerCase(str);
        }

        return str
            .replace(/\s[a-z]/g, upperCase) // convert first char of each word to UPPERCASE
            .replace(/^\s*[A-Z]+/g, lowerCase) // convert first word to lowercase
            .replace(/\s+/g, ''); // remove spaces
    }
    module.exports = camelCase;

