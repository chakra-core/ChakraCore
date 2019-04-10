(function() {
  function vendorModule() {
    'use strict';

    return {
      'default': self['<%= name %>'],
      __esModule: true,
    };
  }

  define('<%= name %>', [], vendorModule);
})();
