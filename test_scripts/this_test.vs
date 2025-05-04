class Test {
    // Variable declaration 
    private: string $name = "Test";
    
    // Simple getter method
    function getName() string {
        // Use $this->name syntax
        return $this->name;
    }
}

// Create an instance
Test $test = new Test();

// Print the name
printnl($test->getName()); 