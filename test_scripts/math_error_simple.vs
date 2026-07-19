// Simple error test for Math module

printnl("=== Math Module Error Test ===");
printnl("");

printnl("Testing sqrt(-1) - should cause error:");
try {
    double $result = sqrt(-1.0);
    printnl("FAIL: sqrt(-1) should not have returned: ", $result);
} catch (string $e) {
    printnl("OK: sqrt(-1) rejected");
}