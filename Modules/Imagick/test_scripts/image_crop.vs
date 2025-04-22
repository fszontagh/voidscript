Imagick $img = new Imagick();

if ($argc != 3) {
    string $error = sformat("Usage: {} <path_to_the_image> <path_to_cropped_image>", $argv[0]);
    throw_error($error);
}

$img->read($argv[1]);
$img->crop(256,256,0,0);
$img->write($argv[2]);