/*
 * Demo universal setting and getting cookies directly with electrode-cookies.
 *
 * Server side only works for HapiJS for now.
 *
 * This is mainly to demo this.context.ssr. The pattern directly access the
 * cookie store and introduce a potential side effect and/or makes your
 * component stateful.
 *
 * It's useful if you need to just drop a cookie value.
 *
 * electrode-redux-router-engine creates this.context.ssr with the legacy API.
 * On the server this.context.ssr.request is available.
 *
 * this.context.ssr is undefined when running on the browser.
 *
 */

import React from "react";
import custom from "../styles/custom.css"; // eslint-disable-line no-unused-vars
import Cookies from "<%= cookiesModule %>";
import PropTypes from "prop-types";
import milligram from "milligram/dist/milligram.css"; // eslint-disable-line no-unused-vars

const COOKIE_NAME = "democookie";

class DemoCookies extends React.Component {
  constructor(props) {
    super(props);
  }

  _refresh() {
    this.setState({ cookie: Cookies.get(COOKIE_NAME, this.context.ssr) });
  }

  _setCookie(evt) {
    evt.preventDefault();
    const cookieValue = this.refs.newCookieValue.value;
    this.refs.newCookieValue.value = "";
    Cookies.set(COOKIE_NAME, cookieValue, this.context.ssr);
    this._refresh();
  }

  _expireCookie() {
    Cookies.expire(COOKIE_NAME, this.context.ssr);
    this._refresh();
  }

  render() {
    // server side rendering
    let cookieValue;

    if (this.context.ssr) {
      cookieValue = `From_SSR_${Date.now()}`;
      Cookies.set(COOKIE_NAME, cookieValue, this.context.ssr);
    } else {
      cookieValue = Cookies.get(COOKIE_NAME);
    }

    return (
      <div>
        <h6 styleName={"custom.docs-header"}>Demo Non-Redux Universal Cookies</h6>
        <p>Test cookie value: {cookieValue || "<NOT SET>"}</p>

        <input type="text" style={{ width: "100px", marginRight: "10px" }} ref="newCookieValue" />
        <button
          styleName={"milligram.button-outline"}
          type="submit"
          onClick={x => this._setCookie(x)}
        >
          Update Cookie
        </button>
        <button
          styleName={"milligram.button-outline"}
          type="submit"
          onClick={() => this._refresh()}
        >
          Refresh Cookie
        </button>
        <button styleName={"milligram.button-outline"} onClick={() => this._expireCookie()}>
          Expire Cookie
        </button>
      </div>
    );
  }
}

//
// Must set this to get this.context.ssr in the component during server side rendering
// This is the legacy React context API.  https://reactjs.org/docs/legacy-context.html
//
DemoCookies.contextTypes = {
  ssr: PropTypes.object
};

export default DemoCookies;
