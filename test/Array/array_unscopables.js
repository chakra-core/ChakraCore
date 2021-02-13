print("------ Array.prototype[@@unscopables] test ------");

const unscopables = Array.prototype[Symbol.unscopables];

const list = ["copyWithin", "entries", "fill", "find", "findIndex", "flat", "flatMap", "includes", "keys", "values"]
const length = list.length;

for (let index = 0; index < length; index++) {
    const propName = list[index];
    const value = unscopables[propName];
    if (value === true) {
        print(`Array.prototype[@@unscopables].${ propName } - ok`);
    } else {
        throw new Error(`Array.prototype[@@unscopables].${ propName } - \`true\` excepted, got - \`${ value }\``);
    }
}

