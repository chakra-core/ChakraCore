import React from "react";
import PropTypes from "prop-types";
import { connect } from "react-redux";
import { toggleCheck, incNumber, decNumber } from "../actions";
import "../styles/custom.css";

class DemoStates extends React.Component {
  render() {
    const { checked, value, dispatch } = this.props;
    return (
      <div>
        <h6 styleName={"docs-header"}>Demo Managing States with Redux</h6>
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
  }
}

DemoStates.propTypes = {
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
)(DemoStates);
