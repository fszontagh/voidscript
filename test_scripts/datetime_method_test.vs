print("=== DateTime Method Test ===");

DateTime $dt = new DateTime();
print("DateTime object created");

// Try calling year method
print("Attempting to call year()...");
int $year_val = $dt->year();
print("Year: " + number_to_string($year_val));

print("End of method test");