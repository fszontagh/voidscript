// Test enum dot notation after lexer fix
enum Color {
    RED,
    GREEN,
    BLUE
};

// Test basic dot notation access
int redValue = Color.RED;
int greenValue = Color.GREEN;
int blueValue = Color.BLUE;

// Print the results to verify they work
print("Testing enum dot notation:\n");
print("Color.RED = ");
print(redValue);
print("\n");
print("Color.GREEN = ");
print(greenValue);
print("\n");
print("Color.BLUE = ");
print(blueValue);
print("\n");

// Test in variable assignment
int selectedColor = Color.RED;
print("Selected color: ");
print(selectedColor);
print("\n");

// Test in conditional
if (selectedColor == Color.RED) {
    print("Selected color is RED\n");
}

print("Enum dot notation test completed successfully!\n");