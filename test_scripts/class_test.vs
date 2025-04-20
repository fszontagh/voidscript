// Define a simple class with defaults and two public properties
        class Foo {
          public: int $a = 1;
          public: int $b = 2;

          function test() {
                printnl("Test method called");
          }
          function incrementA() int {
            return this->a = this->a + 1;
          }
        }

        // Instantiate with one constructor arg (overrides $a, leaves $b at default)
        Foo $f = new Foo(10);

        // Should print “10”
        printnl("$f->a = ", $f->a);
        printnl("$f->b = ", $f->b);

        // Should print “2”
        printnl("$f->b + $f->a = ",$f->b + $f->a);

        // typeof returns a string “object”
        printnl(typeof($f));

        //$f->test();
