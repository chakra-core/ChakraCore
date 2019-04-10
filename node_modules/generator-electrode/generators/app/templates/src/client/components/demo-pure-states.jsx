/*
 * This is the exact same component as in demo-states.jsx except implemented
 * with a pure functional component.
 */

import React from "react";
import PropTypes from "prop-types";
import { connect } from "react-redux";
import { toggleCheck, incNumber, decNumber } from "../actions";
import "../styles/custom.css";

const DemoPureStates = props => {
  const { checked, value, dispatch } = props;
  return (
    <div>
      <h6 styleName={"docs-header"}>Demo Managing States in Pure Functional Component</h6>
      <label
        styleName={"checkbox-label"}
        onChange={() => dispatch(toggleCheck())}
        checked={checked}
      >
        <input type="checkbox" checked={checked} onChange={() => null} />
        <span> checkbox </span>
      </label>
      <div styleName={"checkbox-label-width10rem"}>{checked ? "checked" : "unchecked"}</div>
      <div>
        <button onClick={() => dispatch(decNumber())}>&#8810;</button>
        <div styleName={"checkbox-label-width6rem"}>{value}</div>
        <button onClick={() => dispatch(incNumber())}>&#8811;</button>
      </div>
    </div>
  );
};

DemoPureStates.propTypes = {
  checked: PropTypes.bool,
  value: PropTypes.number.isRequired,
  dispatch: PropTypes.func.isRequired
};

const mapStateToProps = state => {
  return {
    checked: state.checkBox.checked,
    value: state.number.value
  };
};

export default connect(
  mapStateToProps,
  dispatch => ({ dispatch })
)(DemoPureStates);
