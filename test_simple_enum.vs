// Simple enum test
enum Status {
    PENDING,
    RUNNING = 10,
    DONE
};

Status $state = Status.PENDING;

print("Testing enum declaration completed\n");
