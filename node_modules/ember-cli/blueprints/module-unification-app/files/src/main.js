import Application from "@ember/application";
import Resolver from "./resolver";
import loadInitializers from "ember-load-initializers";
import config from "../config/environment";

const App = Application.extend({
  modulePrefix: config.modulePrefix,
  podModulePrefix: config.podModulePrefix,
  Resolver
});

loadInitializers(App, config.modulePrefix + "/src/init");

/*
 * This line is added to support initializers in the `app/` directory
 */
loadInitializers(App, config.modulePrefix);

export default App;
