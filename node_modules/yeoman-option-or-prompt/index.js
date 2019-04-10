'use strict';

module.exports = function(prompts) {
  // This method will only show prompts that haven't been supplied as options. This makes the generator more composable.
  const filteredPrompts = [];
  const props = new Map();

  prompts.forEach(function prompts(prompt) {
    const option = this.options[prompt.name];

    if (option === undefined) {
      // No option supplied, user will be prompted
      filteredPrompts.push(prompt);
    } else {      
      // Options supplied, add to props
      props[prompt.name] = normalize(option); 
    }
  }, this);

  if (filteredPrompts.length) {
    return this.prompt(filteredPrompts).then(function mergeProps(mergeProps) {
      // Merge mergeProps into props/
      Object.assign(props, mergeProps);
      return props;
    });
  }

  // No prompting required call the callback right away.
  return Promise.resolve(props);
};

function normalize(option){
    // TODO: 
    // accept other types
    
    if (typeof option === 'boolean') {
      return option;
    }

    if (typeof option === 'string'){
      let lc = option.toLowerCase();

      // it's a boolean in string format
      if (lc === 'true' || lc === 'false') {
        return (lc === 'true');
      } else {
        return option;
      }
    }
}