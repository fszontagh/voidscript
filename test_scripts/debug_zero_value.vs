class TestInteger {
    int $value = 0;
    
    getValueDirect() {
        return this->$value;
    }
    
    printDebugInfo() {
        try {
            print("Value is: " + this->$value);
        } catch {
            print("Failed to access value");
        }
    }
}

// Test creating instance and accessing zero value
$obj = new TestInteger();
$obj->printDebugInfo();

print("Trying direct access:");
try {
    $result = $obj->getValueDirect();
    print("Got result: " + $result);
} catch {
    print("Failed to get result");
}

// Test with non-zero value
class TestIntegerNonZero {
    int $value = 42;
    
    printDebugInfo() {
        print("Non-zero value is: " + this->$value);
    }
}

$obj2 = new TestIntegerNonZero();
$obj2->printDebugInfo();
