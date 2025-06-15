enum Status {
    PENDING,
    RUNNING = 10,
    DONE
};

print("Enum declared");
Status $state = Status.PENDING;
print("Enum assigned");