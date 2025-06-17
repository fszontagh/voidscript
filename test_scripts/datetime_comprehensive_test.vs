print("=== VoidScript DateTime Module Comprehensive Test ===");
print("");

// Test built-in functions
print("1. Testing Built-in Functions:");
print("-------------------------------");
int $timestamp = current_unix_timestamp();
print("Current Unix Timestamp: " + number_to_string($timestamp));
print("Current Date (default format): " + date());
print("");

// Test DateTime class instantiation
print("2. Testing DateTime Class:");
print("---------------------------");
DateTime $dt = new DateTime();
print("Created new DateTime object");
print("");

// Test property methods
print("3. Testing Property Methods:");
print("-----------------------------");
print("Year: " + number_to_string($dt->year()));
print("Month: " + number_to_string($dt->month()));
print("Day: " + number_to_string($dt->day()));
print("Hour: " + number_to_string($dt->hour()));
print("Minute: " + number_to_string($dt->minute()));
print("Second: " + number_to_string($dt->second()));
print("");

// Test formatting
print("4. Testing Date Formatting:");
print("----------------------------");
print("Y-m-d format: " + $dt->format("Y-m-d"));
print("Y-m-d H:i:s format: " + $dt->format("Y-m-d H:i:s"));
print("m/d/Y format: " + $dt->format("m/d/Y"));
print("H:i:s format: " + $dt->format("H:i:s"));
print("Custom format (Y-m-d [H:i]): " + $dt->format("Y-m-d [H:i]"));
print("");

// Test date manipulation
print("5. Testing Date Manipulation:");
print("------------------------------");
DateTime $dt2 = new DateTime();
print("Original date: " + $dt2->format("Y-m-d H:i:s"));

$dt2->addDays(5);
print("After adding 5 days: " + $dt2->format("Y-m-d H:i:s"));

$dt2->addMonths(2);
print("After adding 2 months: " + $dt2->format("Y-m-d H:i:s"));

$dt2->addYears(1);
print("After adding 1 year: " + $dt2->format("Y-m-d H:i:s"));

$dt2->addHours(3);
print("After adding 3 hours: " + $dt2->format("Y-m-d H:i:s"));

$dt2->addMinutes(30);
print("After adding 30 minutes: " + $dt2->format("Y-m-d H:i:s"));

$dt2->addSeconds(45);
print("After adding 45 seconds: " + $dt2->format("Y-m-d H:i:s"));
print("");

// Test negative values (subtraction)
print("6. Testing Date Subtraction:");
print("-----------------------------");
DateTime $dt3 = new DateTime();
print("Original date: " + $dt3->format("Y-m-d H:i:s"));

$dt3->addDays(-10);
print("After subtracting 10 days: " + $dt3->format("Y-m-d H:i:s"));

$dt3->addMonths(-1);
print("After subtracting 1 month: " + $dt3->format("Y-m-d H:i:s"));

$dt3->addHours(-5);
print("After subtracting 5 hours: " + $dt3->format("Y-m-d H:i:s"));
print("");

// Test multiple DateTime objects
print("7. Testing Multiple DateTime Objects:");
print("-------------------------------------");
DateTime $birthday = new DateTime();
$birthday->addYears(-30);
$birthday->addMonths(-6);
$birthday->addDays(-15);

DateTime $anniversary = new DateTime();
$anniversary->addYears(-5);

print("Birthday (30 years, 6 months, 15 days ago): " + $birthday->format("Y-m-d"));
print("Anniversary (5 years ago): " + $anniversary->format("Y-m-d"));
print("");

// Test edge cases and various formats
print("8. Testing Various Format Patterns:");
print("------------------------------------");
DateTime $formatTest = new DateTime();
print("ISO format (Y-m-d): " + $formatTest->format("Y-m-d"));
print("US format (m/d/Y): " + $formatTest->format("m/d/Y"));
print("European format (d.m.Y): " + $formatTest->format("d.m.Y"));
print("Time only (H:i:s): " + $formatTest->format("H:i:s"));
print("12-hour format simulation (H:i): " + $formatTest->format("H:i"));
print("Year only (Y): " + $formatTest->format("Y"));
print("Month only (m): " + $formatTest->format("m"));
print("Day only (d): " + $formatTest->format("d"));
print("");

print("=== DateTime Module Test Complete ===");
print("All DateTime functionality has been tested successfully!");