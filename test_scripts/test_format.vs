printnl("=== TESTING FORMAT MODULE ===");

// Test the format function
string $formatted = format("Hello {}, you have {} messages!", "World", "5");
printnl("Formatted string: ", $formatted);

// Test format_print function
printnl("Direct format_print test:");
format_print("User: {} | Score: {} | Status: {}", "Alice", "1250", "Active");
