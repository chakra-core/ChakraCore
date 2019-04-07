export const toggleCheck = () => {
  return {
    type: "TOGGLE_CHECK"
  };
};

export const incNumber = () => {
  return {
    type: "INC_NUMBER"
  };
};

export const decNumber = () => {
  return {
    type: "DEC_NUMBER"
  };
};

export const inputName = value => {
  return {
    type: "INPUT_NAME",
    value
  };
};

export const inputTextarea = value => {
  return {
    type: "INPUT_TEXT_AREA",
    value
  };
};

export const selectOption = value => {
  return {
    type: "SELECT_OPTION",
    value
  };
};

export const setShowFakeComp = value => {
  return {
    type: "SHOW_FAKE_COMP",
    value
  };
};
