import React, { Component } from "react";
import { Nav } from "./nav";
import { connect } from "react-redux";
import custom from "../styles/custom.css"; // eslint-disable-line no-unused-vars
import demoStyle from "../styles/demo2.css"; // eslint-disable-line no-unused-vars

class Demo2 extends Component {
  constructor(props) {
    super(props);
  }

  render() {
    return (
      <div styleName={"custom.container"}>
        <Nav {...this.props} />
        <section styleName={"custom.header"}>
          <h2>Buttons Demo</h2>
          <div styleName="demoStyle.main">
            <div styleName="demoStyle.sub-main">
              <button styleName="demoStyle.button-one">Tap Me</button>
            </div>
            <div styleName="demoStyle.sub-main">
              <button styleName="demoStyle.button-two">
                <span>Hover Me</span>
              </button>
            </div>
            <div styleName="demoStyle.sub-main">
              <button styleName="demoStyle.button-three">Click Me</button>
            </div>
          </div>
        </section>
      </div>
    );
  }
}

const mapStateToProps = () => {
  return {};
};

export default connect(
  mapStateToProps,
  dispatch => ({ dispatch })
)(Demo2);
