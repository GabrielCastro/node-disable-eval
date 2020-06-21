const bindings = require("bindings")("evalmanager.node");

module.exports = {
  setEvalAllowed: bindings.setEvalAllowed.bind(undefined),
};
