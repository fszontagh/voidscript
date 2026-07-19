# File I/O Feature Test
# Writes into the system temp dir and removes the file afterwards, so the test is
# idempotent and leaves no untracked artefact in the repo. It used to write
# test_file_io.txt next to the script and fail on every run after the first with
# "File already exists".
string $f = "/tmp/voidscript_file_io_test.txt";

# Start from a known state in case a previous run was interrupted.
if (file_exists($f)) {
    file_unlink($f);
}

print("file_exists before create: ", file_exists($f), "\n");
file_put_contents($f, "Hello from VoidScript!", false);
print("file_exists after create: ", file_exists($f), "\n");
print("file_get_contents: ", file_get_contents($f), "\n");
# Overwrite with permission
file_put_contents($f, "Overwritten content", true);
print("file_get_contents after overwrite: ", file_get_contents($f), "\n");

file_unlink($f);
print("file_exists after cleanup: ", file_exists($f), "\n");
