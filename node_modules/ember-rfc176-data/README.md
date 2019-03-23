
ember-rfc176-data
==============================================================================

JSON data for [RFC #176](https://github.com/emberjs/rfcs/blob/master/text/0176-javascript-module-api.md)

### Related Projects

- [babel-plugin-ember-modules-api-polyfill](https://github.com/ember-cli/babel-plugin-ember-modules-api-polyfill)
- [ember-modules-codemod](https://github.com/ember-cli/ember-modules-codemod)

## Contents

### New Modules to Globals

| Before                                   | After                                                                       |
| ---                                      | ---                                                                         |
| `Ember._action`                          | `import { action } from '@ember/object';`                                   |
| `Ember._componentManagerCapabilities`    | `import { capabilities } from '@ember/component';`                          |
| `Ember._setComponentManager`             | `import { setComponentManager } from '@ember/component';`                   |
| `Ember._tracked`                         | `import { tracked } from '@glimmer/tracking';`                              |
| `Ember.$`                                | `import $ from 'jquery';`                                                   |
| `Ember.A`                                | `import { A } from '@ember/array';`                                         |
| `Ember.addListener`                      | `import { addListener } from '@ember/object/events';`                       |
| `Ember.addObserver`                      | `import { addObserver } from '@ember/object/observers';`                    |
| `Ember.aliasMethod`                      | `import { aliasMethod } from '@ember/object';`                              |
| `Ember.Application`                      | `import Application from '@ember/application';`                             |
| `Ember.ApplicationInstance`              | `import ApplicationInstance from '@ember/application/instance';`            |
| `Ember.Array`                            | `import EmberArray from '@ember/array';`                                    |
| `Ember.ArrayProxy`                       | `import ArrayProxy from '@ember/array/proxy';`                              |
| `Ember.assert`                           | `import { assert } from '@ember/debug';`                                    |
| `Ember.assign`                           | `import { assign } from '@ember/polyfills';`                                |
| `Ember.AutoLocation`                     | `import AutoLocation from '@ember/routing/auto-location';`                  |
| `Ember.cacheFor`                         | `import { cacheFor } from '@ember/object/internals';`                       |
| `Ember.Checkbox`                         | `import Checkbox from '@ember/component/checkbox';`                         |
| `Ember.compare`                          | `import { compare } from '@ember/utils';`                                   |
| `Ember.Component`                        | `import Component from '@ember/component';`                                 |
| `Ember.computed`                         | `import { computed } from '@ember/object';`                                 |
| `Ember.computed.alias`                   | `import { alias } from '@ember/object/computed';`                           |
| `Ember.computed.and`                     | `import { and } from '@ember/object/computed';`                             |
| `Ember.computed.bool`                    | `import { bool } from '@ember/object/computed';`                            |
| `Ember.computed.collect`                 | `import { collect } from '@ember/object/computed';`                         |
| `Ember.computed.deprecatingAlias`        | `import { deprecatingAlias } from '@ember/object/computed';`                |
| `Ember.computed.empty`                   | `import { empty } from '@ember/object/computed';`                           |
| `Ember.computed.equal`                   | `import { equal } from '@ember/object/computed';`                           |
| `Ember.computed.filter`                  | `import { filter } from '@ember/object/computed';`                          |
| `Ember.computed.filterBy`                | `import { filterBy } from '@ember/object/computed';`                        |
| `Ember.computed.filterProperty`          | `import { filterProperty } from '@ember/object/computed';`                  |
| `Ember.computed.gt`                      | `import { gt } from '@ember/object/computed';`                              |
| `Ember.computed.gte`                     | `import { gte } from '@ember/object/computed';`                             |
| `Ember.computed.intersect`               | `import { intersect } from '@ember/object/computed';`                       |
| `Ember.computed.lt`                      | `import { lt } from '@ember/object/computed';`                              |
| `Ember.computed.lte`                     | `import { lte } from '@ember/object/computed';`                             |
| `Ember.computed.map`                     | `import { map } from '@ember/object/computed';`                             |
| `Ember.computed.mapBy`                   | `import { mapBy } from '@ember/object/computed';`                           |
| `Ember.computed.mapProperty`             | `import { mapProperty } from '@ember/object/computed';`                     |
| `Ember.computed.match`                   | `import { match } from '@ember/object/computed';`                           |
| `Ember.computed.max`                     | `import { max } from '@ember/object/computed';`                             |
| `Ember.computed.min`                     | `import { min } from '@ember/object/computed';`                             |
| `Ember.computed.none`                    | `import { none } from '@ember/object/computed';`                            |
| `Ember.computed.not`                     | `import { not } from '@ember/object/computed';`                             |
| `Ember.computed.notEmpty`                | `import { notEmpty } from '@ember/object/computed';`                        |
| `Ember.computed.oneWay`                  | `import { oneWay } from '@ember/object/computed';`                          |
| `Ember.computed.or`                      | `import { or } from '@ember/object/computed';`                              |
| `Ember.computed.readOnly`                | `import { readOnly } from '@ember/object/computed';`                        |
| `Ember.computed.reads`                   | `import { reads } from '@ember/object/computed';`                           |
| `Ember.computed.setDiff`                 | `import { setDiff } from '@ember/object/computed';`                         |
| `Ember.computed.sort`                    | `import { sort } from '@ember/object/computed';`                            |
| `Ember.computed.sum`                     | `import { sum } from '@ember/object/computed';`                             |
| `Ember.computed.union`                   | `import { union } from '@ember/object/computed';`                           |
| `Ember.computed.uniq`                    | `import { uniq } from '@ember/object/computed';`                            |
| `Ember.computed.uniqBy`                  | `import { uniqBy } from '@ember/object/computed';`                          |
| `Ember.ComputedProperty`                 | `import ComputedProperty from '@ember/object/computed';`                    |
| `Ember.ContainerDebugAdapter`            | `import ContainerDebugAdapter from '@ember/debug/container-debug-adapter';` |
| `Ember.Controller`                       | `import Controller from '@ember/controller';`                               |
| `Ember.copy`                             | `import { copy } from '@ember/object/internals';`                           |
| `Ember.CoreObject`                       | `import CoreObject from '@ember/object/core';`                              |
| `Ember.create`                           | `import { create } from '@ember/polyfills';`                                |
| `Ember.DataAdapter`                      | `import DataAdapter from '@ember/debug/data-adapter';`                      |
| `Ember.debug`                            | `import { debug } from '@ember/debug';`                                     |
| `Ember.Debug.registerDeprecationHandler` | `import { registerDeprecationHandler } from '@ember/debug';`                |
| `Ember.Debug.registerWarnHandler`        | `import { registerWarnHandler } from '@ember/debug';`                       |
| `Ember.DefaultResolver`                  | `import GlobalsResolver from '@ember/application/globals-resolver';`        |
| `Ember.defineProperty`                   | `import { defineProperty } from '@ember/object';`                           |
| `Ember.deprecate`                        | `import { deprecate } from '@ember/application/deprecations';`              |
| `Ember.deprecateFunc`                    | `import { deprecateFunc } from '@ember/application/deprecations';`          |
| `Ember.Engine`                           | `import Engine from '@ember/engine';`                                       |
| `Ember.EngineInstance`                   | `import EngineInstance from '@ember/engine/instance';`                      |
| `Ember.Enumerable`                       | `import Enumerable from '@ember/enumerable';`                               |
| `Ember.Error`                            | `import EmberError from '@ember/error';`                                    |
| `Ember.Evented`                          | `import Evented from '@ember/object/evented';`                              |
| `Ember.expandProperties`                 | `import { expandProperties } from '@ember/object/computed';`                |
| `Ember.FEATURES`                         | `import { FEATURES } from '@ember/canary-features';`                        |
| `Ember.FEATURES.isEnabled`               | `import { isEnabled } from '@ember/canary-features';`                       |
| `Ember.get`                              | `import { get } from '@ember/object';`                                      |
| `Ember.getEngineParent`                  | `import { getEngineParent } from '@ember/engine';`                          |
| `Ember.getOwner`                         | `import { getOwner } from '@ember/application';`                            |
| `Ember.getProperties`                    | `import { getProperties } from '@ember/object';`                            |
| `Ember.getWithDefault`                   | `import { getWithDefault } from '@ember/object';`                           |
| `Ember.guidFor`                          | `import { guidFor } from '@ember/object/internals';`                        |
| `Ember.HashLocation`                     | `import HashLocation from '@ember/routing/hash-location';`                  |
| `Ember.Helper`                           | `import Helper from '@ember/component/helper';`                             |
| `Ember.Helper.helper`                    | `import { helper as buildHelper } from '@ember/component/helper';`          |
| `Ember.HistoryLocation`                  | `import HistoryLocation from '@ember/routing/history-location';`            |
| `Ember.HTMLBars.compile`                 | `import { compileTemplate } from '@ember/template-compilation';`            |
| `Ember.HTMLBars.precompile`              | `import { precompileTemplate } from '@ember/template-compilation';`         |
| `Ember.HTMLBars.template`                | `import { createTemplateFactory } from '@ember/template-factory';`          |
| `Ember.inject.controller`                | `import { inject } from '@ember/controller';`                               |
| `Ember.inject.service`                   | `import { inject } from '@ember/service';`                                  |
| `Ember.inspect`                          | `import { inspect } from '@ember/debug';`                                   |
| `Ember.Instrumentation.instrument`       | `import { instrument } from '@ember/instrumentation';`                      |
| `Ember.Instrumentation.reset`            | `import { reset } from '@ember/instrumentation';`                           |
| `Ember.Instrumentation.subscribe`        | `import { subscribe } from '@ember/instrumentation';`                       |
| `Ember.Instrumentation.unsubscribe`      | `import { unsubscribe } from '@ember/instrumentation';`                     |
| `Ember.isArray`                          | `import { isArray } from '@ember/array';`                                   |
| `Ember.isBlank`                          | `import { isBlank } from '@ember/utils';`                                   |
| `Ember.isEmpty`                          | `import { isEmpty } from '@ember/utils';`                                   |
| `Ember.isEqual`                          | `import { isEqual } from '@ember/utils';`                                   |
| `Ember.isNone`                           | `import { isNone } from '@ember/utils';`                                    |
| `Ember.isPresent`                        | `import { isPresent } from '@ember/utils';`                                 |
| `Ember.keys`                             | `import { keys } from '@ember/polyfills';`                                  |
| `Ember.LinkComponent`                    | `import LinkComponent from '@ember/routing/link-component';`                |
| `Ember.Location`                         | `import Location from '@ember/routing/location';`                           |
| `Ember.makeArray`                        | `import { makeArray } from '@ember/array';`                                 |
| `Ember.Map`                              | `import EmberMap from '@ember/map';`                                        |
| `Ember.MapWithDefault`                   | `import MapWithDefault from '@ember/map/with-default';`                     |
| `Ember.merge`                            | `import { merge } from '@ember/polyfills';`                                 |
| `Ember.Mixin`                            | `import Mixin from '@ember/object/mixin';`                                  |
| `Ember.MutableArray`                     | `import MutableArray from '@ember/array/mutable';`                          |
| `Ember.Namespace`                        | `import Namespace from '@ember/application/namespace';`                     |
| `Ember.NoneLocation`                     | `import NoneLocation from '@ember/routing/none-location';`                  |
| `Ember.notifyPropertyChange`             | `import { notifyPropertyChange } from '@ember/object';`                     |
| `Ember.Object`                           | `import EmberObject from '@ember/object';`                                  |
| `Ember.ObjectProxy`                      | `import ObjectProxy from '@ember/object/proxy';`                            |
| `Ember.Observable`                       | `import Observable from '@ember/object/observable';`                        |
| `Ember.observer`                         | `import { observer } from '@ember/object';`                                 |
| `Ember.on`                               | `import { on } from '@ember/object/evented';`                               |
| `Ember.onLoad`                           | `import { onLoad } from '@ember/application';`                              |
| `Ember.platform.hasPropertyAccessors`    | `import { hasPropertyAccessors } from '@ember/polyfills';`                  |
| `Ember.PromiseProxyMixin`                | `import PromiseProxyMixin from '@ember/object/promise-proxy-mixin';`        |
| `Ember.removeListener`                   | `import { removeListener } from '@ember/object/events';`                    |
| `Ember.removeObserver`                   | `import { removeObserver } from '@ember/object/observers';`                 |
| `Ember.Resolver`                         | `import Resolver from '@ember/application/resolver';`                       |
| `Ember.Route`                            | `import Route from '@ember/routing/route';`                                 |
| `Ember.Router`                           | `import EmberRouter from '@ember/routing/router';`                          |
| `Ember.RSVP`                             | `import RSVP from 'rsvp';`                                                  |
| `Ember.RSVP.all`                         | `import { all } from 'rsvp';`                                               |
| `Ember.RSVP.allSettled`                  | `import { allSettled } from 'rsvp';`                                        |
| `Ember.RSVP.defer`                       | `import { defer } from 'rsvp';`                                             |
| `Ember.RSVP.denodeify`                   | `import { denodeify } from 'rsvp';`                                         |
| `Ember.RSVP.filter`                      | `import { filter } from 'rsvp';`                                            |
| `Ember.RSVP.hash`                        | `import { hash } from 'rsvp';`                                              |
| `Ember.RSVP.hashSettled`                 | `import { hashSettled } from 'rsvp';`                                       |
| `Ember.RSVP.map`                         | `import { map } from 'rsvp';`                                               |
| `Ember.RSVP.off`                         | `import { off } from 'rsvp';`                                               |
| `Ember.RSVP.on`                          | `import { on } from 'rsvp';`                                                |
| `Ember.RSVP.Promise`                     | `import { Promise } from 'rsvp';`                                           |
| `Ember.RSVP.race`                        | `import { race } from 'rsvp';`                                              |
| `Ember.RSVP.reject`                      | `import { reject } from 'rsvp';`                                            |
| `Ember.RSVP.resolve`                     | `import { resolve } from 'rsvp';`                                           |
| `Ember.run`                              | `import { run } from '@ember/runloop';`                                     |
| `Ember.run.begin`                        | `import { begin } from '@ember/runloop';`                                   |
| `Ember.run.bind`                         | `import { bind } from '@ember/runloop';`                                    |
| `Ember.run.cancel`                       | `import { cancel } from '@ember/runloop';`                                  |
| `Ember.run.debounce`                     | `import { debounce } from '@ember/runloop';`                                |
| `Ember.run.end`                          | `import { end } from '@ember/runloop';`                                     |
| `Ember.run.join`                         | `import { join } from '@ember/runloop';`                                    |
| `Ember.run.later`                        | `import { later } from '@ember/runloop';`                                   |
| `Ember.run.next`                         | `import { next } from '@ember/runloop';`                                    |
| `Ember.run.once`                         | `import { once } from '@ember/runloop';`                                    |
| `Ember.run.schedule`                     | `import { schedule } from '@ember/runloop';`                                |
| `Ember.run.scheduleOnce`                 | `import { scheduleOnce } from '@ember/runloop';`                            |
| `Ember.run.throttle`                     | `import { throttle } from '@ember/runloop';`                                |
| `Ember.runInDebug`                       | `import { runInDebug } from '@ember/debug';`                                |
| `Ember.runLoadHooks`                     | `import { runLoadHooks } from '@ember/application';`                        |
| `Ember.sendEvent`                        | `import { sendEvent } from '@ember/object/events';`                         |
| `Ember.Service`                          | `import Service from '@ember/service';`                                     |
| `Ember.set`                              | `import { set } from '@ember/object';`                                      |
| `Ember.setOwner`                         | `import { setOwner } from '@ember/application';`                            |
| `Ember.setProperties`                    | `import { setProperties } from '@ember/object';`                            |
| `Ember.String.camelize`                  | `import { camelize } from '@ember/string';`                                 |
| `Ember.String.capitalize`                | `import { capitalize } from '@ember/string';`                               |
| `Ember.String.classify`                  | `import { classify } from '@ember/string';`                                 |
| `Ember.String.dasherize`                 | `import { dasherize } from '@ember/string';`                                |
| `Ember.String.decamelize`                | `import { decamelize } from '@ember/string';`                               |
| `Ember.String.fmt`                       | `import { fmt } from '@ember/string';`                                      |
| `Ember.String.htmlSafe`                  | `import { htmlSafe } from '@ember/template';`                               |
| `Ember.String.isHTMLSafe`                | `import { isHTMLSafe } from '@ember/template';`                             |
| `Ember.String.loc`                       | `import { loc } from '@ember/string';`                                      |
| `Ember.String.underscore`                | `import { underscore } from '@ember/string';`                               |
| `Ember.String.w`                         | `import { w } from '@ember/string';`                                        |
| `Ember.Test.Adapter`                     | `import TestAdapter from '@ember/test/adapter';`                            |
| `Ember.Test.registerAsyncHelper`         | `import { registerAsyncHelper } from '@ember/test';`                        |
| `Ember.Test.registerHelper`              | `import { registerHelper } from '@ember/test';`                             |
| `Ember.Test.registerWaiter`              | `import { registerWaiter } from '@ember/test';`                             |
| `Ember.Test.unregisterHelper`            | `import { unregisterHelper } from '@ember/test';`                           |
| `Ember.Test.unregisterWaiter`            | `import { unregisterWaiter } from '@ember/test';`                           |
| `Ember.TextArea`                         | `import TextArea from '@ember/component/text-area';`                        |
| `Ember.TextField`                        | `import TextField from '@ember/component/text-field';`                      |
| `Ember.tryInvoke`                        | `import { tryInvoke } from '@ember/utils';`                                 |
| `Ember.trySet`                           | `import { trySet } from '@ember/object';`                                   |
| `Ember.typeOf`                           | `import { typeOf } from '@ember/utils';`                                    |
| `Ember.VERSION`                          | `import { VERSION } from '@ember/version';`                                 |
| `Ember.warn`                             | `import { warn } from '@ember/debug';`                                      |

### New Modules to Globals

#### `@ember/application`
| Module                                                               | Global                      |
| ---                                                                  | ---                         |
| `import Application from '@ember/application';`                      | `Ember.Application`         |
| `import { getOwner } from '@ember/application';`                     | `Ember.getOwner`            |
| `import { onLoad } from '@ember/application';`                       | `Ember.onLoad`              |
| `import { runLoadHooks } from '@ember/application';`                 | `Ember.runLoadHooks`        |
| `import { setOwner } from '@ember/application';`                     | `Ember.setOwner`            |
| `import { deprecate } from '@ember/application/deprecations';`       | `Ember.deprecate`           |
| `import { deprecateFunc } from '@ember/application/deprecations';`   | `Ember.deprecateFunc`       |
| `import GlobalsResolver from '@ember/application/globals-resolver';` | `Ember.DefaultResolver`     |
| `import ApplicationInstance from '@ember/application/instance';`     | `Ember.ApplicationInstance` |
| `import Namespace from '@ember/application/namespace';`              | `Ember.Namespace`           |
| `import Resolver from '@ember/application/resolver';`                | `Ember.Resolver`            |

#### `@ember/array`
| Module                                             | Global               |
| ---                                                | ---                  |
| `import EmberArray from '@ember/array';`           | `Ember.Array`        |
| `import { A } from '@ember/array';`                | `Ember.A`            |
| `import { isArray } from '@ember/array';`          | `Ember.isArray`      |
| `import { makeArray } from '@ember/array';`        | `Ember.makeArray`    |
| `import MutableArray from '@ember/array/mutable';` | `Ember.MutableArray` |
| `import ArrayProxy from '@ember/array/proxy';`     | `Ember.ArrayProxy`   |

#### `@ember/canary-features`
| Module                                                | Global                     |
| ---                                                   | ---                        |
| `import { FEATURES } from '@ember/canary-features';`  | `Ember.FEATURES`           |
| `import { isEnabled } from '@ember/canary-features';` | `Ember.FEATURES.isEnabled` |

#### `@ember/component`
| Module                                                             | Global                                |
| ---                                                                | ---                                   |
| `import Component from '@ember/component';`                        | `Ember.Component`                     |
| `import { capabilities } from '@ember/component';`                 | `Ember._componentManagerCapabilities` |
| `import { setComponentManager } from '@ember/component';`          | `Ember._setComponentManager`          |
| `import Checkbox from '@ember/component/checkbox';`                | `Ember.Checkbox`                      |
| `import Helper from '@ember/component/helper';`                    | `Ember.Helper`                        |
| `import { helper as buildHelper } from '@ember/component/helper';` | `Ember.Helper.helper`                 |
| `import TextArea from '@ember/component/text-area';`               | `Ember.TextArea`                      |
| `import TextField from '@ember/component/text-field';`             | `Ember.TextField`                     |

#### `@ember/controller`
| Module                                        | Global                    |
| ---                                           | ---                       |
| `import Controller from '@ember/controller';` | `Ember.Controller`        |
| `import { inject } from '@ember/controller';` | `Ember.inject.controller` |

#### `@ember/debug`
| Module                                                                      | Global                                   |
| ---                                                                         | ---                                      |
| `import { assert } from '@ember/debug';`                                    | `Ember.assert`                           |
| `import { debug } from '@ember/debug';`                                     | `Ember.debug`                            |
| `import { inspect } from '@ember/debug';`                                   | `Ember.inspect`                          |
| `import { registerDeprecationHandler } from '@ember/debug';`                | `Ember.Debug.registerDeprecationHandler` |
| `import { registerWarnHandler } from '@ember/debug';`                       | `Ember.Debug.registerWarnHandler`        |
| `import { runInDebug } from '@ember/debug';`                                | `Ember.runInDebug`                       |
| `import { warn } from '@ember/debug';`                                      | `Ember.warn`                             |
| `import ContainerDebugAdapter from '@ember/debug/container-debug-adapter';` | `Ember.ContainerDebugAdapter`            |
| `import DataAdapter from '@ember/debug/data-adapter';`                      | `Ember.DataAdapter`                      |

#### `@ember/engine`
| Module                                                 | Global                  |
| ---                                                    | ---                     |
| `import Engine from '@ember/engine';`                  | `Ember.Engine`          |
| `import { getEngineParent } from '@ember/engine';`     | `Ember.getEngineParent` |
| `import EngineInstance from '@ember/engine/instance';` | `Ember.EngineInstance`  |

#### `@ember/enumerable`
| Module                                        | Global             |
| ---                                           | ---                |
| `import Enumerable from '@ember/enumerable';` | `Ember.Enumerable` |

#### `@ember/error`
| Module                                   | Global        |
| ---                                      | ---           |
| `import EmberError from '@ember/error';` | `Ember.Error` |

#### `@ember/instrumentation`
| Module                                                  | Global                              |
| ---                                                     | ---                                 |
| `import { instrument } from '@ember/instrumentation';`  | `Ember.Instrumentation.instrument`  |
| `import { reset } from '@ember/instrumentation';`       | `Ember.Instrumentation.reset`       |
| `import { subscribe } from '@ember/instrumentation';`   | `Ember.Instrumentation.subscribe`   |
| `import { unsubscribe } from '@ember/instrumentation';` | `Ember.Instrumentation.unsubscribe` |

#### `@ember/map`
| Module                                                  | Global                 |
| ---                                                     | ---                    |
| `import EmberMap from '@ember/map';`                    | `Ember.Map`            |
| `import MapWithDefault from '@ember/map/with-default';` | `Ember.MapWithDefault` |

#### `@ember/object`
| Module                                                               | Global                            |
| ---                                                                  | ---                               |
| `import EmberObject from '@ember/object';`                           | `Ember.Object`                    |
| `import { action } from '@ember/object';`                            | `Ember._action`                   |
| `import { aliasMethod } from '@ember/object';`                       | `Ember.aliasMethod`               |
| `import { computed } from '@ember/object';`                          | `Ember.computed`                  |
| `import { defineProperty } from '@ember/object';`                    | `Ember.defineProperty`            |
| `import { get } from '@ember/object';`                               | `Ember.get`                       |
| `import { getProperties } from '@ember/object';`                     | `Ember.getProperties`             |
| `import { getWithDefault } from '@ember/object';`                    | `Ember.getWithDefault`            |
| `import { notifyPropertyChange } from '@ember/object';`              | `Ember.notifyPropertyChange`      |
| `import { observer } from '@ember/object';`                          | `Ember.observer`                  |
| `import { set } from '@ember/object';`                               | `Ember.set`                       |
| `import { setProperties } from '@ember/object';`                     | `Ember.setProperties`             |
| `import { trySet } from '@ember/object';`                            | `Ember.trySet`                    |
| `import ComputedProperty from '@ember/object/computed';`             | `Ember.ComputedProperty`          |
| `import { alias } from '@ember/object/computed';`                    | `Ember.computed.alias`            |
| `import { and } from '@ember/object/computed';`                      | `Ember.computed.and`              |
| `import { bool } from '@ember/object/computed';`                     | `Ember.computed.bool`             |
| `import { collect } from '@ember/object/computed';`                  | `Ember.computed.collect`          |
| `import { deprecatingAlias } from '@ember/object/computed';`         | `Ember.computed.deprecatingAlias` |
| `import { empty } from '@ember/object/computed';`                    | `Ember.computed.empty`            |
| `import { equal } from '@ember/object/computed';`                    | `Ember.computed.equal`            |
| `import { expandProperties } from '@ember/object/computed';`         | `Ember.expandProperties`          |
| `import { filter } from '@ember/object/computed';`                   | `Ember.computed.filter`           |
| `import { filterBy } from '@ember/object/computed';`                 | `Ember.computed.filterBy`         |
| `import { filterProperty } from '@ember/object/computed';`           | `Ember.computed.filterProperty`   |
| `import { gt } from '@ember/object/computed';`                       | `Ember.computed.gt`               |
| `import { gte } from '@ember/object/computed';`                      | `Ember.computed.gte`              |
| `import { intersect } from '@ember/object/computed';`                | `Ember.computed.intersect`        |
| `import { lt } from '@ember/object/computed';`                       | `Ember.computed.lt`               |
| `import { lte } from '@ember/object/computed';`                      | `Ember.computed.lte`              |
| `import { map } from '@ember/object/computed';`                      | `Ember.computed.map`              |
| `import { mapBy } from '@ember/object/computed';`                    | `Ember.computed.mapBy`            |
| `import { mapProperty } from '@ember/object/computed';`              | `Ember.computed.mapProperty`      |
| `import { match } from '@ember/object/computed';`                    | `Ember.computed.match`            |
| `import { max } from '@ember/object/computed';`                      | `Ember.computed.max`              |
| `import { min } from '@ember/object/computed';`                      | `Ember.computed.min`              |
| `import { none } from '@ember/object/computed';`                     | `Ember.computed.none`             |
| `import { not } from '@ember/object/computed';`                      | `Ember.computed.not`              |
| `import { notEmpty } from '@ember/object/computed';`                 | `Ember.computed.notEmpty`         |
| `import { oneWay } from '@ember/object/computed';`                   | `Ember.computed.oneWay`           |
| `import { or } from '@ember/object/computed';`                       | `Ember.computed.or`               |
| `import { readOnly } from '@ember/object/computed';`                 | `Ember.computed.readOnly`         |
| `import { reads } from '@ember/object/computed';`                    | `Ember.computed.reads`            |
| `import { setDiff } from '@ember/object/computed';`                  | `Ember.computed.setDiff`          |
| `import { sort } from '@ember/object/computed';`                     | `Ember.computed.sort`             |
| `import { sum } from '@ember/object/computed';`                      | `Ember.computed.sum`              |
| `import { union } from '@ember/object/computed';`                    | `Ember.computed.union`            |
| `import { uniq } from '@ember/object/computed';`                     | `Ember.computed.uniq`             |
| `import { uniqBy } from '@ember/object/computed';`                   | `Ember.computed.uniqBy`           |
| `import CoreObject from '@ember/object/core';`                       | `Ember.CoreObject`                |
| `import Evented from '@ember/object/evented';`                       | `Ember.Evented`                   |
| `import { on } from '@ember/object/evented';`                        | `Ember.on`                        |
| `import { addListener } from '@ember/object/events';`                | `Ember.addListener`               |
| `import { removeListener } from '@ember/object/events';`             | `Ember.removeListener`            |
| `import { sendEvent } from '@ember/object/events';`                  | `Ember.sendEvent`                 |
| `import { cacheFor } from '@ember/object/internals';`                | `Ember.cacheFor`                  |
| `import { copy } from '@ember/object/internals';`                    | `Ember.copy`                      |
| `import { guidFor } from '@ember/object/internals';`                 | `Ember.guidFor`                   |
| `import Mixin from '@ember/object/mixin';`                           | `Ember.Mixin`                     |
| `import Observable from '@ember/object/observable';`                 | `Ember.Observable`                |
| `import { addObserver } from '@ember/object/observers';`             | `Ember.addObserver`               |
| `import { removeObserver } from '@ember/object/observers';`          | `Ember.removeObserver`            |
| `import PromiseProxyMixin from '@ember/object/promise-proxy-mixin';` | `Ember.PromiseProxyMixin`         |
| `import ObjectProxy from '@ember/object/proxy';`                     | `Ember.ObjectProxy`               |

#### `@ember/polyfills`
| Module                                                     | Global                                |
| ---                                                        | ---                                   |
| `import { assign } from '@ember/polyfills';`               | `Ember.assign`                        |
| `import { create } from '@ember/polyfills';`               | `Ember.create`                        |
| `import { hasPropertyAccessors } from '@ember/polyfills';` | `Ember.platform.hasPropertyAccessors` |
| `import { keys } from '@ember/polyfills';`                 | `Ember.keys`                          |
| `import { merge } from '@ember/polyfills';`                | `Ember.merge`                         |

#### `@ember/routing`
| Module                                                           | Global                  |
| ---                                                              | ---                     |
| `import AutoLocation from '@ember/routing/auto-location';`       | `Ember.AutoLocation`    |
| `import HashLocation from '@ember/routing/hash-location';`       | `Ember.HashLocation`    |
| `import HistoryLocation from '@ember/routing/history-location';` | `Ember.HistoryLocation` |
| `import LinkComponent from '@ember/routing/link-component';`     | `Ember.LinkComponent`   |
| `import Location from '@ember/routing/location';`                | `Ember.Location`        |
| `import NoneLocation from '@ember/routing/none-location';`       | `Ember.NoneLocation`    |
| `import Route from '@ember/routing/route';`                      | `Ember.Route`           |
| `import EmberRouter from '@ember/routing/router';`               | `Ember.Router`          |

#### `@ember/runloop`
| Module                                           | Global                   |
| ---                                              | ---                      |
| `import { begin } from '@ember/runloop';`        | `Ember.run.begin`        |
| `import { bind } from '@ember/runloop';`         | `Ember.run.bind`         |
| `import { cancel } from '@ember/runloop';`       | `Ember.run.cancel`       |
| `import { debounce } from '@ember/runloop';`     | `Ember.run.debounce`     |
| `import { end } from '@ember/runloop';`          | `Ember.run.end`          |
| `import { join } from '@ember/runloop';`         | `Ember.run.join`         |
| `import { later } from '@ember/runloop';`        | `Ember.run.later`        |
| `import { next } from '@ember/runloop';`         | `Ember.run.next`         |
| `import { once } from '@ember/runloop';`         | `Ember.run.once`         |
| `import { run } from '@ember/runloop';`          | `Ember.run`              |
| `import { schedule } from '@ember/runloop';`     | `Ember.run.schedule`     |
| `import { scheduleOnce } from '@ember/runloop';` | `Ember.run.scheduleOnce` |
| `import { throttle } from '@ember/runloop';`     | `Ember.run.throttle`     |

#### `@ember/service`
| Module                                     | Global                 |
| ---                                        | ---                    |
| `import Service from '@ember/service';`    | `Ember.Service`        |
| `import { inject } from '@ember/service';` | `Ember.inject.service` |

#### `@ember/string`
| Module                                        | Global                    |
| ---                                           | ---                       |
| `import { camelize } from '@ember/string';`   | `Ember.String.camelize`   |
| `import { capitalize } from '@ember/string';` | `Ember.String.capitalize` |
| `import { classify } from '@ember/string';`   | `Ember.String.classify`   |
| `import { dasherize } from '@ember/string';`  | `Ember.String.dasherize`  |
| `import { decamelize } from '@ember/string';` | `Ember.String.decamelize` |
| `import { fmt } from '@ember/string';`        | `Ember.String.fmt`        |
| `import { loc } from '@ember/string';`        | `Ember.String.loc`        |
| `import { underscore } from '@ember/string';` | `Ember.String.underscore` |
| `import { w } from '@ember/string';`          | `Ember.String.w`          |

#### `@ember/template`
| Module                                          | Global                    |
| ---                                             | ---                       |
| `import { htmlSafe } from '@ember/template';`   | `Ember.String.htmlSafe`   |
| `import { isHTMLSafe } from '@ember/template';` | `Ember.String.isHTMLSafe` |

#### `@ember/template-compilation`
| Module                                                              | Global                      |
| ---                                                                 | ---                         |
| `import { compileTemplate } from '@ember/template-compilation';`    | `Ember.HTMLBars.compile`    |
| `import { precompileTemplate } from '@ember/template-compilation';` | `Ember.HTMLBars.precompile` |

#### `@ember/template-factory`
| Module                                                             | Global                    |
| ---                                                                | ---                       |
| `import { createTemplateFactory } from '@ember/template-factory';` | `Ember.HTMLBars.template` |

#### `@ember/test`
| Module                                               | Global                           |
| ---                                                  | ---                              |
| `import { registerAsyncHelper } from '@ember/test';` | `Ember.Test.registerAsyncHelper` |
| `import { registerHelper } from '@ember/test';`      | `Ember.Test.registerHelper`      |
| `import { registerWaiter } from '@ember/test';`      | `Ember.Test.registerWaiter`      |
| `import { unregisterHelper } from '@ember/test';`    | `Ember.Test.unregisterHelper`    |
| `import { unregisterWaiter } from '@ember/test';`    | `Ember.Test.unregisterWaiter`    |
| `import TestAdapter from '@ember/test/adapter';`     | `Ember.Test.Adapter`             |

#### `@ember/utils`
| Module                                      | Global            |
| ---                                         | ---               |
| `import { compare } from '@ember/utils';`   | `Ember.compare`   |
| `import { isBlank } from '@ember/utils';`   | `Ember.isBlank`   |
| `import { isEmpty } from '@ember/utils';`   | `Ember.isEmpty`   |
| `import { isEqual } from '@ember/utils';`   | `Ember.isEqual`   |
| `import { isNone } from '@ember/utils';`    | `Ember.isNone`    |
| `import { isPresent } from '@ember/utils';` | `Ember.isPresent` |
| `import { tryInvoke } from '@ember/utils';` | `Ember.tryInvoke` |
| `import { typeOf } from '@ember/utils';`    | `Ember.typeOf`    |

#### `@ember/version`
| Module                                      | Global          |
| ---                                         | ---             |
| `import { VERSION } from '@ember/version';` | `Ember.VERSION` |

#### `@glimmer/tracking`
| Module                                         | Global           |
| ---                                            | ---              |
| `import { tracked } from '@glimmer/tracking';` | `Ember._tracked` |

#### `jquery`
| Module                    | Global    |
| ---                       | ---       |
| `import $ from 'jquery';` | `Ember.$` |

#### `rsvp`
| Module                                | Global                   |
| ---                                   | ---                      |
| `import RSVP from 'rsvp';`            | `Ember.RSVP`             |
| `import { Promise } from 'rsvp';`     | `Ember.RSVP.Promise`     |
| `import { all } from 'rsvp';`         | `Ember.RSVP.all`         |
| `import { allSettled } from 'rsvp';`  | `Ember.RSVP.allSettled`  |
| `import { defer } from 'rsvp';`       | `Ember.RSVP.defer`       |
| `import { denodeify } from 'rsvp';`   | `Ember.RSVP.denodeify`   |
| `import { filter } from 'rsvp';`      | `Ember.RSVP.filter`      |
| `import { hash } from 'rsvp';`        | `Ember.RSVP.hash`        |
| `import { hashSettled } from 'rsvp';` | `Ember.RSVP.hashSettled` |
| `import { map } from 'rsvp';`         | `Ember.RSVP.map`         |
| `import { off } from 'rsvp';`         | `Ember.RSVP.off`         |
| `import { on } from 'rsvp';`          | `Ember.RSVP.on`          |
| `import { race } from 'rsvp';`        | `Ember.RSVP.race`        |
| `import { reject } from 'rsvp';`      | `Ember.RSVP.reject`      |
| `import { resolve } from 'rsvp';`     | `Ember.RSVP.resolve`     |


### Scripts

The tables above can be generated using the scripts in the `scripts` folder, e.g.:

```
node scripts/generate-markdown-table.js
```


## Contributing


### mappings.json format

The `mappings.json` file contains an array of entries with the following format:

```ts
interface Mapping {
  /**
    The globals based API that this module and export replace.
   */
  global: string;

  /**
    The module to import.
   */
  module: string;

  /**
    The export name from the module.
   */
  export: string;

  /**
    `true` if this module / export combination has been deprecated.
   */
  deprecated: boolean;

  /**
    The recommended `localName` to use for a given module/export. Only present
    when a name other than the value for `export` should be used.

    This is useful for things like ember-modules-codemod or eslint-plugin-ember
    so that they can provide a nice suggested import for a given global path usage.
   */
  localName?: string;

  /**
    When this mapping is deprecated it may include a replacement module/export which
    should be used instead.
  */
  replacement?: {
    module: string;
    export: string;
  }
}
```

### Reserved Words

In some cases, Ember's names may conflict with names built in to the language.
In those cases, we should not inadvertently shadow those identifiers.

```js
import Object from "@ember/object";

// ...later
Object.keys(obj);
// oops! TypeError: Object.keys is not a function
```

A list of reserved identifiers (including `Object`) is included in
`reserved.json`. Anything that appears in this list should be prefixed with
`Ember`; so, for example, `import Object from "@ember/object"` should become
`import EmberObject from "@ember/object"`.
