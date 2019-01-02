Vireye linker gets object files and transfomes it into .sce file, that can be loaded by vireye core (currently in dev)

### Basic usage:
```
clang++ --target=thumb -mthumb -mfloat-abi=soft -fdeclspec -c -m32 -Os test.cpp test_helper.cpp
linker test.o test_helper.o test.sce
```

Test program may be found in Debug folder
