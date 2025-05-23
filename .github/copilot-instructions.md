# Copilot Instructions
This file contains instructions for Copilot to help it understand the context of the project.
The project is a C++ application that uses CMake for building and includes a set of test scripts.
The project is designed to be built and run using CMake, and it includes a set of test scripts for testing the functionality of the application.
The test scripts are located in the `test_scripts` directory and are written in a custom scripting language.
The project is structured to allow for easy building and testing, with a clear separation between the source code and the test scripts.
The main source code is located in the `src` directory, and the CMake configuration files are located in the root directory.
The project is designed to be cross-platform and should work on any system that supports CMake.

Always try to build and run the project using the following commands:

- **To build the project:**  
  `cmake --build build`

- **To run a test script:**  
Check the current working directory. To run the binary, you need to be in the build directory.
`./voidscript --debug ../test_scripts/SCRIPT_NAME.vs`  
  (Replace `SCRIPT_NAME` with the name of a script from the test_scripts folder.)

Use these commands whenever you need to build or test the project.

With ValuePtr you can use:
``` 
Symbols::ValuePtr ptr = "Something";
std::string something = ptr;
```
