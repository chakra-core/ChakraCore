import React, { Component } from "react";
import PropTypes from "prop-types";
import { connect } from "react-redux";
import { Nav } from "./nav";
import { inputName, inputTextarea, selectOption } from "../actions";
import custom from "../styles/custom.css"; // eslint-disable-line no-unused-vars
import demoStyle from "../styles/demo1.css"; // eslint-disable-line no-unused-vars

class Demo1 extends Component {
  constructor(props) {
    super(props);

    this.state = {
      username: { value: "" },
      textarea: { value: "" },
      selectedOption: { value: "0-13" }
    };
  }

  render() {
    const { dispatch } = this.props;
    return (
      <div styleName={"custom.container"}>
        <Nav {...this.props} />
        <div styleName={"demoStyle.container"}>
          <h2>Forms Demo</h2>
          <form>
            <fieldset>
              <label htmlFor="nameField">Name</label>
              <input
                type="text"
                placeholder="Electrode User"
                id="nameField"
                value={this.props.username}
                onChange={event => {
                  dispatch(inputName(event.target.value));
                }}
              />
              <label htmlFor="ageRangeField">Experience with Electrode</label>
              <select
                id="ageRangeField"
                onChange={event => {
                  dispatch(selectOption(event.target.value));
                }}
                value={this.props.selectedOption}
              >
                <option value="0-13">0-13 month</option>
                <option value="14-17">14-17 month</option>
                <option value="18-23">18-23 month</option>
                <option value="24+">24+ month</option>
              </select>
              <label htmlFor="commentField">Comment</label>
              <textarea
                placeholder="Leave feedback for electrode..."
                id="commentField"
                value={this.props.textarea}
                onChange={event => dispatch(inputTextarea(event.target.value))}
              />
              <input type="submit" value="Send" />
            </fieldset>
          </form>
        </div>
      </div>
    );
  }
}

Demo1.propTypes = {
  username: PropTypes.string,
  textarea: PropTypes.string,
  selectedOption: PropTypes.string,
  dispatch: PropTypes.func.isRequired
};

const mapStateToProps = state => {
  return {
    username: state.username.value,
    textarea: state.textarea.value,
    selectedOption: state.selectedOption.value
  };
};

export default connect(
  mapStateToProps,
  dispatch => ({ dispatch })
)(Demo1);
