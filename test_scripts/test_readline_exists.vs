print("Testing if getline exists and can be called...");

# Try to call getline and see what happens
try {
    string result = getline();
    print("getline worked! Result: " + result);
} catch {
    print("getline function not found or failed");
}

print("Test completed");