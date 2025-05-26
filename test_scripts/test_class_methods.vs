class TestClass {
    private:
        string $name;
        int $value;
    
    public:
        function setValue(int $val) {
            $this->value = $val;
        }
        
        function getValue() int {
            return $this->value;
        }
        
        function setName(string $n) {
            $this->name = $n;
        }
        
        function getName() string {
            return $this->name;
        }
        
        function construct(string $initialName, int $initialValue) {
            $this->name = $initialName;
            $this->value = $initialValue;
        }
}

TestClass $obj = new TestClass("Test", 42);
print("Name: " + $obj->getName());
print("Value: " + $obj->getValue());

$obj->setName("Updated");
$obj->setValue(100);

print("Updated Name: " + $obj->getName());
print("Updated Value: " + $obj->getValue());
