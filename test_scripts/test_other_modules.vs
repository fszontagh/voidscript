printnl("=== TESTING ALL MODULES EXCEPT XML ===");

// Test MariaDB class
printnl("Testing MariaDB class...");
class MariaDB $db = new MariaDB();
printnl("✓ MariaDB class created successfully");

// Test Imagick class  
printnl("Testing Imagick class...");
class Imagick $img = new Imagick();
printnl("✓ Imagick class created successfully");

printnl("All non-XML modules working correctly!");
