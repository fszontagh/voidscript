class Simple {
    // No constructor
    public: string $name = "Default";
    
    function getName() string {
        return $this->name;
    }
}

Simple $s = new Simple();
printnl($s->getName()); 