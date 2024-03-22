# Contributing to Subconverter

Welcome, and thank you for your interest in contributing to subconverter!

## Reporting Issues

- Check if the issue already exists.
- Use the issue template to create a new issue.
- Be as detailed as possible.

## Feature Requests

- Check the feature doesn't already exist.
- Use the feature request template to submit a new request.

## Pull Requests

- Fork the repository.
- Create a new branch for your changes.
- Follow the coding style and guidelines.
- Test the features as much as you can.
- Update the documentation as needed.
- Submit a pull request with a detailed description of your changes.

## Setting Up Your Development Environment

To contribute to Subconverter, you'll need to set up your development environment. Here's a brief overview:

### Windows Build Instructions

1. install the prerequisites:
for example if you are using msys2 and mingw-w64 for compilation:
```shell
pacman -S base-devel git mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-pcre2 patch python
```

2. run script to initialize the project for development:
```shell
sh scripts/dev.windows.release.sh
```

3. debug the project with GDB:
```shell
gdb ./subconverter/subconverter.exe
```
or use VSCode with the following configuration:
```json
{
    "name": "C/C++: g++.exe build and debug active file",
    "type": "cppdbg",
    "request": "launch",
    "program": "${workspaceFolder}/subconverter/subconverter.exe",
    "args": [],
    "stopAtEntry": false,
    "cwd": "${workspaceFolder}/subconverter/",
    "environment": [],
    "externalConsole": true,
    "MIMode": "gdb",
    "miDebuggerPath": "C:\\msys64\\mingw64\\bin\\gdb.exe",
    "setupCommands": [
        {
        "description": "Enable pretty-printing for gdb",
        "text": "-enable-pretty-printing",
        "ignoreFailures": true
        },
        {
        "description": "Set Disassembly Flavor to Intel",
        "text": "-gdb-set disassembly-flavor intel",
        "ignoreFailures": true
        }
    ],
}
```

4. increamental build during development:

```shell
make -j4 && cp subconverter.exe subconverter/subconverter.exe
```

### Linux / macOS

1. install the prerequisites:

You should install all the following prequisites via your package manager.
- cmake
- pcre2
- patch
- python

2. run the script to initialize the project for development:

```shell
sh scripts/build.macos.release.sh
```

3. run the executable for test:
```shell
./subconverter
```

Thank you for contributing to subconverter!
