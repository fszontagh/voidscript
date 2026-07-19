// With no arguments, fall back to the bundled fixture so the script is runnable
// unattended as part of the test suite. Explicit arguments still behave as a CLI tool.
string $image = "";
int $width = 0;
boolean $usingFixture = false;

if ($argc == 1) {
    $image = path_join(path_dirname($argv[0]), "fixtures/sample.png");
    $width = 8;
    $usingFixture = true;
    printnl("No arguments given - using bundled fixture ", $image);
} else if ($argc < 3 || $argc > 4) {
    error("Usage: ", $argv[0], " <image> <width> [height]");
    exit(1);
} else {
    $image = $argv[1];
    $width = string_to_number($argv[2]);
}

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
if ($usingFixture || $argc == 3) {
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
        break;
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

if ($usingFixture) {
    file_unlink($output);
    printnl("Removed fixture output ", $output);
}
