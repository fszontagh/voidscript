// Test to validate object state preservation issues
printnl("=== Testing Object State Preservation ===");

// Create Imagick instance
Imagick $imagick = new Imagick();
printnl("Step 1: Created Imagick instance");

// Call read method
printnl("Step 2: Calling read method...");
$imagick->read("../Képek/12103909.jpg");
printnl("Step 3: Read method completed successfully");

// Try to call getWidth
printnl("Step 4: Attempting to call getWidth...");
int $width = $imagick->getWidth();
printnl("Step 5: getWidth returned: ", $width);

printnl("=== Test Complete ===");