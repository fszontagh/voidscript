enum Status {
    PENDING,
    RUNNING = 10,
    DONE
};

Status $state = Status.PENDING;  // This should work
Status $other;                   // This should work (uninitialized)

print("Target functionality test passed!");
print("Status $state = Status.PENDING; works correctly");
printnl("Status $other; works correctly");

printnl("$state: ", $state);
printnl("$other: ", $other);

if ($state == Status.DONE) {
    printnl("Pending");
}