js-object-pretty-print
======================

Serializes a javascript object to a printable string. String is formatted to be used in a pure text environments, like a console log, as an HTML output,  or to create a JSON string.

## Installation

npm install js-object-pretty-print

## Usage

```
var pretty = require('js-object-pretty-print').pretty,
    address,
    value;

function onAnother(foo, bar) {
    var lola;

    lola = foo + bar;
    return lola;
}

address = {
    'street': 'Callejon de las ranas 128',
    'city': 'Falfurrias',
    'state':
    'Texas',
    'zip': '88888-9999'
};

value = {
    'name': 'Damaso Infanzon Manzo',
    'address': address,
    'favorites': {
        'music': ['Mozart', 'Beethoven', 'The Beatles'],
        'authors': ['John Grisham', 'Isaac Asimov', 'P.L. Travers'],
        'books': [ 'Pelican Brief', 'I, Robot', 'Mary Poppins' ]
    },
    'dates': [ new Date(), new Date("05/25/1954") ],
    'numbers': [ 10, 883, 521 ],
    'boolean': [ true, false, false, false ],
    'isObject': true,
    'isDuck': false,
    'onWhatever': function (foo, bar) {
        var lola;

        lola = foo + bar;
        return lola;
    },
    'onAnother': onAnother
};

address.value = value;

console.log(pretty(value));
```

It is also possible to use a minified version of the code

```
...
var prettyMin = require('js0object-pretty-print/index-min.js').pretty;
...
```

Either the full or the minified versions render the same. Both are unit tested with Mocha and Chai

## Options

Function pretty accepts four arguments:

    pretty(object, indentSize, outputTo, fullFunction);

### object (mandatory)
Is the javascript object to serialize. If no object is present the function will return a string with an error.

### indentSize (optional)
Number of spaces in a one level indent. Default 4

### outputTo (optional)
String to determine the formatting of the output. One of "PRINT", "HTML" or "JSON". This argument is case insensitive. Default value is "PRINT"

### fullFunction
A boolean to determine to expand all the text of a function or to display only the signature. The default value is to display only the signature, that is the word **function** followed by the function name, if any, followed by the arguments of the function in parenthesis.
* **fullFunction == false** Passing the object above will result in
```
{
...
    onWhatever: "function (foo, bar)",
    onAnother: "function onAnother(foo, bar)"
}
```

* **fullFunction == true** Passing the object above will result in
```
{
...
    onWhatever: function (foo, bar) {
        var lola;

        lola = foo + bar;
        return lola;
    },
    onAnother: function onAnother(foo, bar) {
    var lola;

    lola = foo + bar;
    return lola;
}
}
```
Notice that the program does not attempt to pretty print the function. It is rendered as written.

Expected behavior
* **PRINT** Indentation is done with the space character, line breaks are done with the newLine character "\n" and the attribute names are not surrounded with quotes. Pretty similar to what you see in the -webkit debugger
* **HTML** Indentation is done with non breakable spaces &amp;nbsp; line breaks are done with &lt;br/&gt;. Otherwise identical to **JSON**. Handy to dump into a div inside a page and get a decent formatting
* **JSON** Only difference with PRINT is that attribute names are surrounded with quotes

## Circular references
Sometimes some objects have circular references. This will can cause a heap overflow if not dealt properly. The code now detects circular references and stop inspecting the object

## null and undefined
Objects and properties with value null or undefined are now properly reported.

## Release History
* 0.1.0 Initial Release
* 0.1.1 Bug fixes
* 0.1.2 Add JSON output, create robust testing with Mocha, add minified version of the code
* 0.1.3 Circular reference detection, option to print only the signature of functions
* 0.1.4 Better processing of undefined or null values
* 0.2.0 Fixed several bugs, including proper treatment of null and undefined values. The output for HTML now has the property names enclosed in quotes.