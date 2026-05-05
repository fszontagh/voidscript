function user_defined(object $ctx) {
    print("user_defined called\n");
}

print("function_exists(user_defined) = ");
print(function_exists("user_defined"));
print("\n");
print("function_exists(does_not_exist) = ");
print(function_exists("does_not_exist"));
print("\n");
