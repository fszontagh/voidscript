class Test {
    private:
    int $age = 25;
}

Test $obj = new Test();
print("About to access private property");
print($obj->age);
print("This should not print");