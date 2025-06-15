// Test enum value access
enum Status {
    PENDING,
    RUNNING,
    DONE
};

int value = Status.PENDING;
print("Enum value access works!\n");
print(value);
print("\n");