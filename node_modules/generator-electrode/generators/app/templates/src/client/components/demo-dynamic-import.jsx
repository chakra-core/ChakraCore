import React from "react";
import loadable from "@loadable/component";
import PropTypes from "prop-types";
import { connect } from "react-redux";
import { setShowFakeComp } from "../actions";
import Promise from "bluebird";
import demoStyle from "../styles/demo2.css"; // eslint-disable-line no-unused-vars
import custom from "../styles/custom.css"; // eslint-disable-line no-unused-vars

const Fallback = () => (
  <div styleName={"custom.dynamic-demo-fallback"}>
    <strong>Dynamic Imported Component is loading ...</strong>
  </div>
);

let Demo = loadable(() => import("./demo-loadable"), {
  fallback: <Fallback />
});

const timeout = 1000;
const load = dispatch => {
  dispatch(setShowFakeComp(false));
  Promise.try(() => loadable(() => import(/* webpackChunkName: "loadable" */ "./demo-loadable")))
    .delay(timeout)
    .then(x => (Demo = x))
    .then(() => {
      dispatch(setShowFakeComp(true));
    });
};

const DynamicImportDemo = ({ showFakeComp, dispatch }) => {
  return (
    <div>
      <h6 styleName={"custom.docs-header"}>
        Demo Dynamic Import with&nbsp;
        <a href="https://www.smooth-code.com/open-source/loadable-components/">
          Loadable Components
        </a>
      </h6>
      <div styleName={"custom.dynamic-demo-box"}>
        {showFakeComp.value ? <Demo /> : <Fallback />}
      </div>
      <div>
        <button onClick={() => load(dispatch)}>Refresh Comp</button>
      </div>
    </div>
  );
};
DynamicImportDemo.propTypes = {
  showFakeComp: PropTypes.object,
  dispatch: PropTypes.func
};
export default connect(
  state => state,
  dispatch => ({ dispatch })
)(DynamicImportDemo);
