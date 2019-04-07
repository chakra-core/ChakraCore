import { combineReducers } from "redux";

const number = store => {
  return store || {};
};

const checkBox = store => {
  return store || {};
};

export default combineReducers({
  number,
  checkBox
});
