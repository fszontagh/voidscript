// Test to debug variable name resolution

function testVarName(string $param) {
    printnl("Function param type: ", typeof($param));
    printnl("Function param value: ", $param);
}

testVarName("testValue");
