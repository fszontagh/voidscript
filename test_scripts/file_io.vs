# File I/O Feature Test
string $f = "test_file_io.txt";
print("file_exists before create: ", file_exists($f), "\n");
file_put_contents($f, "Hello from VoidScript!", false);
print("file_exists after create: ", file_exists($f), "\n");
print("file_get_contents: ", file_get_contents($f), "\n");
# Overwrite with permission
file_put_contents($f, "Overwritten content", true);
print("file_get_contents after overwrite: ", file_get_contents($f), "\n");