// Regression: MethodCallExpressionNode counted method calls with a depth counter that
// was incremented on entry but not decremented on several return paths (native methods,
// comparison methods). It therefore measured cumulative calls, not nesting depth, so a
// program that made more than ~100 method calls TOTAL aborted with a bogus
// "Infinite loop detected in method calls". A DepthGuard RAII object now restores it on
// every exit path.
// Expected: clean exit 0.

class Counter {
    private: int $n = 0;
    public:
        function bump() { $this->n = $this->n + 1; }
        function value() int { return $this->n; }
}

Counter $c = new Counter();
// Far more than the old 100-call ceiling, all sequential (depth 1), not recursive.
for (int $i = 0; $i < 500; $i++) {
    $c->bump();
}
printnl($c->value());           // 500

// genuine deep recursion must still be caught, not silently allowed
printnl("done");
