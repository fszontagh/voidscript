// Test script to verify enum implementation

// Define a simple enum
enum Color {
    RED,
    GREEN = 5,
    BLUE
};

// Test enum values in expressions
int red_value = Color.RED;
int green_value = Color.GREEN;
int blue_value = Color.BLUE;

print("Red value: ");
print(red_value);
print("\n");

print("Green value: ");
print(green_value);
print("\n");

print("Blue value: ");
print(blue_value);
print("\n");

// Test enum values in switch statement
switch (Color.RED) {
    case Color.RED:
        print("Matched RED\n");
        break;
    case Color.GREEN:
        print("Matched GREEN\n");
        break;
    case Color.BLUE:
        print("Matched BLUE\n");
        break;
    default:
        print("No match\n");
        break;
}

// Test with variable
int current_color = Color.GREEN;
switch (current_color) {
    case Color.RED:
        print("Current color is RED\n");
        break;
    case Color.GREEN:
        print("Current color is GREEN\n");
        break;
    case Color.BLUE:
        print("Current color is BLUE\n");
        break;
    default:
        print("Unknown color\n");
        break;
}