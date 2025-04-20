class Foo {
  public: int $a = 1;
  public: int $b = 2;

  function test() {
    printnl("Test method called");
  }
}
Foo $f = new Foo();
$f->test();
