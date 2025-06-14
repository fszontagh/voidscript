// Check command line arguments
if ($argc < 3 || $argc > 4) {
    error("Usage: ", $argv[0], " <image> <width> [height]");
    exit(1);
}

const string $image = $argv[1];
int $width = string_to_number($argv[2]);
int $height = 0;

// Validate input file exists
if (!file_exists($image)) {
    error("Image file not found: ", $image);
    exit(1);
}

printnl("Resizing image: ", $image);

// Create Imagick instance and read the image
Imagick $imagick = new Imagick();
$imagick->read($image);

// Calculate height if not provided (maintain aspect ratio)
if ($argc == 3) {
    int $origWidth = $imagick->getWidth();
    int $origHeight = $imagick->getHeight();
    $height = ($origHeight * $width / $origWidth);
    printnl("Maintaining aspect ratio - calculated height: ", $height);
} else {
    $height = $argv[3];
}

printnl("Resizing to: ", $width, "x", $height);

// Resize the image
$imagick->resize($width, $height);

// Generate output filename with "_resized" suffix
// Find the last dot position for file extension
int $dotPos = -1;
int $len = string_length($image);
for (int $i = $len - 1; $i >= 0; $i--) {
    string $char = string_substr($image, $i, 1);
    if ($char == ".") {
        $dotPos = $i;
        // Continue searching for compatibility - avoid break statement
        $i = -1; // This will exit the loop
    }
}

string $output = NULL;
if ($dotPos > 0) {
    string $namepart = string_substr($image, 0, $dotPos);
    string $extension = string_substr($image, $dotPos + 1, $len - $dotPos - 1);
    $output = format("{}_resized.{}", $namepart, $extension);
} else {
    // No extension found, just append _resized
    $output = format("{}_resized", $image);
}

// Write the resized image
$imagick->write($output);
printnl("Resized image saved as: ", $output);