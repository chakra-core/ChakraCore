/*
 * This is a demo component the Eletrode app generator included
 * to show using Milligram CSS lib and Redux
 * store for display HTML elements and managing states.
 *
 * To start your own app, please replace or remove these files:
 *
 * - this file (home.jsx)
 * - demo-buttons.jsx
 * - demo-pure-states.jsx
 * - demo-states.jsx
 * - reducers/index.jsx
 * - styles/*.css
 *
 */

import React from "react";
import { connect } from "react-redux";
import "../styles/raleway.css";
import custom from "../styles/custom.css"; // eslint-disable-line no-unused-vars
import electrodePng from "../images/electrode.png";
import DemoStates from "./demo-states";
import DemoPureStates from "./demo-pure-states";
import { DemoButtons } from "./demo-buttons";
import DemoDynamicImport from "./demo-dynamic-import";
import { Nav } from "./nav";

//<% if (cookiesModule) { %>
import DemoCookies from "./demo-cookies";
//<% } %>

//<% if (uiConfigModule) { %>
import config from "<%= uiConfigModule %>";
//<% } %>

//<% if (pwa) { %>
import Notifications from "react-notify-toast";
//<% } %>

class Home extends React.Component {
  constructor(props) {
    super(props);
  }

  render() {
    return (
      <div styleName={"custom.container"}>
        <Nav {...this.props} />

        {/*<% if (pwa) { %>*/}
        <Notifications />
        {/*<% } %>*/}

        <section styleName={"custom.header"}>
          <h2>
            <span>Hello from </span>
            <a href="https://github.com/electrode-io">
              {"Electrode"}
              <img src={electrodePng} />
            </a>
          </h2>
        </section>

        <div styleName={"custom.docs-section"}>
          <DemoStates />
        </div>

        <div styleName={"custom.docs-section"}>
          <DemoPureStates />
        </div>

        {/*<% if (cookiesModule) { %>*/}
        <div styleName={"custom.docs-section"}>
          <DemoCookies />
        </div>
        {/*<% } %>*/}

        {/*<% if (uiConfigModule) { %>*/}
        <div styleName={"custom.docs-section"}>
          <h6 styleName={"custom.docs-header"}>Demo Isomorphic UI Config</h6>
          <div>config.ui.demo: "{config.ui.demo}"</div>
        </div>
        {/*<% } %>*/}

        <div styleName={"custom.docs-section"}>
          <DemoButtons />
        </div>

        <div styleName={"custom.docs-section"}>
          <DemoDynamicImport/>
        </div>

      </div>
    );
  }
}

Home.propTypes = {};

const mapStateToProps = state => state;

export default connect(
  mapStateToProps,
  dispatch => ({ dispatch })
)(Home);
