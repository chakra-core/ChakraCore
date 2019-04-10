import React, { Component } from "react";
import PropTypes from "prop-types";
import { Link } from "react-router-dom";
import styles from "../styles/nav.css"; // eslint-disable-line no-unused-vars

export class Nav extends Component {
  constructor(props) {
    super(props);
  }

  render() {
    const navs = [["Home", ""], ["Demo1", "demo1"], ["Demo2", "demo2"]];
    const currentTab = this.props.location.pathname.replace("/", "");
    return (
      <ul styleName={"styles.bar"}>
        {navs.map((x, i) => {
          const s = `styles.base${currentTab === x[1] ? " styles.active" : ""}`;
          return (
            <li key={i} styleName={s}>
              <Link to={`/${x[1]}`}>{x[0]}</Link>
            </li>
          );
        })}
      </ul>
    );
  }
}

Nav.propTypes = {
  location: PropTypes.shape({
    pathname: PropTypes.string
  })
};

Nav.defaultProps = {
  location: {
    pathname: ""
  }
};
