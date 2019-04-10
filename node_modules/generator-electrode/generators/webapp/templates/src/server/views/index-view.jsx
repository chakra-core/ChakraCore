//
// This is the server side entry point for the React app.
//

import ReduxRouterEngine from "electrode-redux-router-engine";
import { routes } from "../../client/routes";

//
// This function is exported as the content for the webapp plugin.
//
// See config/default.json under plugins.webapp on specifying the content.
//
// When the Web server hits the routes handler installed by the webapp plugin, it
// will call this function to retrieve the content for SSR if it's enabled.
//
//

let routesEngine;

module.exports = req => {
  if (!routesEngine) {
    routesEngine = new ReduxRouterEngine({ routes });
  }

  return routesEngine.render(req);
};
