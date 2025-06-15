//==============================================================================
// COMPREHENSIVE BREAK KEYWORD TESTS
// This file tests break functionality in all loop types to ensure:
// 1. Break works correctly in each loop type
// 2. Break only affects the innermost loop in nested scenarios
// 3. Break works with conditional statements
//==============================================================================

printnl("=== STARTING COMPREHENSIVE BREAK TESTS ===");
printnl("");

//==============================================================================
// 1. WHILE LOOP BREAK TESTS
//==============================================================================

printnl("--- WHILE LOOP BREAK TESTS ---");

// Test 1.1: Simple while loop with break
printnl("Test 1.1: Simple while loop with break");
printnl("Expected: Should print i values 0,1,2,3,4 then break at 5");
int $i = 0;
while ($i < 10) {
    if ($i == 5) {
        printnl("Breaking at i = ", $i);
        break;
    }
    printnl("i = ", $i);
    $i++;
}
printnl("After while loop, i = ", $i, " (should be 5)");
printnl("");

// Test 1.2: While loop with break inside conditional
printnl("Test 1.2: While loop with break inside conditional");
printnl("Expected: Should print even numbers 0,2,4 then break at 6");
int $j = 0;
while ($j < 20) {
    if ($j % 2 == 0) {
        if ($j == 6) {
            printnl("Breaking at even number j = ", $j);
            break;
        }
        printnl("Even j = ", $j);
    }
    $j++;
}
printnl("After conditional while loop, j = ", $j, " (should be 6)");
printnl("");

// Test 1.3: Nested while loops with break (should only break innermost)
printnl("Test 1.3: Nested while loops with break");
printnl("Expected: Inner loop breaks at 3, outer continues");
int $outer = 0;
while ($outer < 3) {
    printnl("Outer loop: outer = ", $outer);
    int $inner = 0;
    while ($inner < 10) {
        if ($inner == 3) {
            printnl("  Breaking inner loop at inner = ", $inner);
            break;
        }
        printnl("  Inner loop: inner = ", $inner);
        $inner++;
    }
    printnl("  After inner loop, inner = ", $inner, " (should be 3)");
    $outer++;
}
printnl("After nested while loops, outer = ", $outer, " (should be 3)");
printnl("");

//==============================================================================
// 2. FOR-IN LOOP BREAK TESTS
//==============================================================================

printnl("--- FOR-IN LOOP BREAK TESTS ---");

// Test 2.1: Simple for-in loop with break
printnl("Test 2.1: Simple for-in loop with break");
printnl("Expected: Should process first 2 properties then break");
object $test_obj = {
    int $prop1 : 10,
    int $prop2 : 20,
    int $prop3 : 30,
    int $prop4 : 40,
    int $prop5 : 50
};

int $count = 0;
for (int $key, auto $value : $test_obj) {
    if ($count == 2) {
        printnl("Breaking for-in loop at count = ", $count);
        break;
    }
    printnl("Processing property: key = ", $key, ", value = ", $value);
    $count++;
}
printnl("After for-in loop, count = ", $count, " (should be 2)");
printnl("");

// Test 2.2: For-in loop with break inside conditional
printnl("Test 2.2: For-in loop with break inside conditional");
printnl("Expected: Should break when value >= 30");
for (int $key2, auto $value2 : $test_obj) {
    if ($value2 >= 30) {
        printnl("Breaking when value >= 30, value = ", $value2);
        break;
    }
    printnl("Processing: key = ", $key2, ", value = ", $value2);
}
printnl("");

// Test 2.3: Nested for-in loops with break
printnl("Test 2.3: Nested for-in loops with break");
printnl("Expected: Inner loop breaks, outer continues");
object $outer_obj = {
    int $a : 1,
    int $b : 2
};

for (int $outer_key, auto $outer_val : $outer_obj) {
    printnl("Outer for-in: key = ", $outer_key, ", value = ", $outer_val);
    int $inner_count = 0;
    for (int $inner_key, auto $inner_val : $test_obj) {
        if ($inner_count == 1) {
            printnl("  Breaking inner for-in at count = ", $inner_count);
            break;
        }
        printnl("  Inner for-in: key = ", $inner_key, ", value = ", $inner_val);
        $inner_count++;
    }
    printnl("  After inner for-in, inner_count = ", $inner_count, " (should be 1)");
}
printnl("");

//==============================================================================
// 3. C-STYLE FOR LOOP BREAK TESTS
//==============================================================================

printnl("--- C-STYLE FOR LOOP BREAK TESTS ---");

// Test 3.1: Simple C-style for loop with break
printnl("Test 3.1: Simple C-style for loop with break");
printnl("Expected: Should print k values 0,1,2,3,4 then break at 5");
for (int $k = 0; $k < 10; $k++) {
    if ($k == 5) {
        printnl("Breaking C-style for loop at k = ", $k);
        break;
    }
    printnl("k = ", $k);
}
printnl("");

// Test 3.2: C-style for loop with break inside conditional
printnl("Test 3.2: C-style for loop with break inside conditional");
printnl("Expected: Should print multiples of 3: 0,3,6 then break at 9");
for (int $m = 0; $m < 20; $m++) {
    if ($m % 3 == 0) {
        if ($m == 9) {
            printnl("Breaking at multiple of 3: m = ", $m);
            break;
        }
        printnl("Multiple of 3: m = ", $m);
    }
}
printnl("");

// Test 3.3: Nested C-style for loops with break
printnl("Test 3.3: Nested C-style for loops with break");
printnl("Expected: Inner loop breaks at 2, outer continues through all iterations");
for (int $outer_c = 0; $outer_c < 3; $outer_c++) {
    printnl("Outer C-style: outer_c = ", $outer_c);
    for (int $inner_c = 0; $inner_c < 5; $inner_c++) {
        if ($inner_c == 2) {
            printnl("  Breaking inner C-style at inner_c = ", $inner_c);
            break;
        }
        printnl("  Inner C-style: inner_c = ", $inner_c);
    }
}
printnl("");

//==============================================================================
// 4. MIXED NESTED LOOP TESTS
//==============================================================================

printnl("--- MIXED NESTED LOOP TESTS ---");

// Test 4.1: While loop containing C-style for loop with break
printnl("Test 4.1: While containing C-style for with break");
printnl("Expected: Inner for breaks at 2, while continues");
int $while_counter = 0;
while ($while_counter < 2) {
    printnl("While iteration: while_counter = ", $while_counter);
    for (int $for_in_while = 0; $for_in_while < 5; $for_in_while++) {
        if ($for_in_while == 2) {
            printnl("  Breaking inner for loop at for_in_while = ", $for_in_while);
            break;
        }
        printnl("  For in while: for_in_while = ", $for_in_while);
    }
    $while_counter++;
}
printnl("");

// Test 4.2: C-style for loop containing while loop with break
printnl("Test 4.2: C-style for containing while with break");
printnl("Expected: Inner while breaks at 2, for continues");
for (int $for_outer = 0; $for_outer < 2; $for_outer++) {
    printnl("For iteration: for_outer = ", $for_outer);
    int $while_in_for = 0;
    while ($while_in_for < 5) {
        if ($while_in_for == 2) {
            printnl("  Breaking inner while at while_in_for = ", $while_in_for);
            break;
        }
        printnl("  While in for: while_in_for = ", $while_in_for);
        $while_in_for++;
    }
}
printnl("");

// Test 4.3: For-in containing C-style for with break
printnl("Test 4.3: For-in containing C-style for with break");
printnl("Expected: Inner C-style for breaks at 1, for-in continues");
object $mixed_obj = {
    int $x : 100,
    int $y : 200
};

for (int $forin_key, auto $forin_val : $mixed_obj) {
    printnl("For-in iteration: key = ", $forin_key, ", value = ", $forin_val);
    for (int $cstyle_in_forin = 0; $cstyle_in_forin < 3; $cstyle_in_forin++) {
        if ($cstyle_in_forin == 1) {
            printnl("  Breaking inner C-style for at cstyle_in_forin = ", $cstyle_in_forin);
            break;
        }
        printnl("  C-style in for-in: cstyle_in_forin = ", $cstyle_in_forin);
    }
}
printnl("");

//==============================================================================
// 5. EDGE CASE TESTS
//==============================================================================

printnl("--- EDGE CASE TESTS ---");

// Test 5.1: Break as first statement in loop
printnl("Test 5.1: Break as first statement in loop");
printnl("Expected: Loop should not execute any iterations");
int $edge_counter = 0;
while ($edge_counter < 10) {
    break;
    printnl("This should never print");
    $edge_counter++;
}
printnl("After immediate break, edge_counter = ", $edge_counter, " (should be 0)");
printnl("");

// Test 5.2: Multiple break conditions
printnl("Test 5.2: Multiple break conditions");
printnl("Expected: Should break at first condition met (p = 3)");
for (int $p = 0; $p < 10; $p++) {
    if ($p == 3) {
        printnl("Breaking at first condition: p = ", $p);
        break;
    }
    if ($p == 7) {
        printnl("This break should never execute");
        break;
    }
    printnl("p = ", $p);
}
printnl("");

// Test 5.3: Break in deeply nested loops
printnl("Test 5.3: Break in deeply nested loops (3 levels)");
printnl("Expected: Innermost loop breaks, middle and outer continue");
for (int $level1 = 0; $level1 < 2; $level1++) {
    printnl("Level 1: level1 = ", $level1);
    for (int $level2 = 0; $level2 < 2; $level2++) {
        printnl("  Level 2: level2 = ", $level2);
        for (int $level3 = 0; $level3 < 5; $level3++) {
            if ($level3 == 1) {
                printnl("    Breaking level 3 at level3 = ", $level3);
                break;
            }
            printnl("    Level 3: level3 = ", $level3);
        }
        printnl("  Completed level 3 iteration");
    }
    printnl("Completed level 2 iteration");
}
printnl("");

printnl("=== ALL BREAK TESTS COMPLETED ===");
printnl("");
printnl("SUMMARY:");
printnl("- All tests verify that break only affects the innermost loop");
printnl("- Break works correctly in while, for-in, and C-style for loops");
printnl("- Break works with conditional statements");
printnl("- Break works as the first statement in a loop");
printnl("- Break works in deeply nested loop structures");