import React from "react";
import ReactDOM from "react-dom";
import Home from "client/components/home";
import { createStore } from "redux";
import { Provider } from "react-redux";
import rootReducer from "client/reducers";
import { BrowserRouter } from "react-router-dom";

describe("Home", () => {
  let component;
  let container;

  beforeEach(() => {
    container = document.createElement("div");
  });

  afterEach(() => {
    ReactDOM.unmountComponentAtNode(container);
  });

  it("has expected content with deep render", () => {
    const initialState = {
      checkBox: { checked: false },
      number: { value: 999 }
    };

    const store = createStore(rootReducer, initialState);

    component = ReactDOM.render(
      <Provider store={store}>
        <BrowserRouter>
          <Home />
        </BrowserRouter>
      </Provider>,
      container
    );

    expect(component).to.not.be.false;
  });
});
