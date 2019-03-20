const { setEvalAllowed } = require("../lib/index");
const expect = require("expect");
const sinon = require("sinon");

const errorRegex = /Code generation from strings disallowed for this context/;

function testEval(code, result) {
  const actual = eval(code);
  expect(actual).toEqual(result);
}

describe("setting boolean", () => {
  describe("true", () => {
    beforeEach(() => setEvalAllowed(true));
    it("should eval ok", () => {
      testEval("(() => 1234)()", 1234);
      testEval('(() => "1234")()', "1234");
    });
  });
  describe("false", () => {
    beforeEach(() => setEvalAllowed(false));
    it("should not eval", () => {
      expect(() => eval('(() => "1234")()')).toThrow(errorRegex);
      expect(() => eval("(() => 1234)()")).toThrow(errorRegex);
    });
  });
});

describe("with a callback", () => {
  let stub;

  beforeEach(() => {
    stub = sinon.stub();
    stub.returns(true);
    setEvalAllowed(stub);
  });

  it("eval true", () => {
    stub.returns(true);
    eval("(() => 12)()");
    sinon.assert.calledOnce(stub);
    sinon.assert.calledWith(stub, "(() => 12)()");
  });

  it("newFunction true", () => {
    stub.returns(true);
    const func = new Function("return 1");
    func();
    func();
    func();
    sinon.assert.calledOnce(stub);
    sinon.assert.calledWith(stub, `(function anonymous(\n) {\nreturn 1\n})`);
  });
});
