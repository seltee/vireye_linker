clang++ --target=thumb -mthumb -mfloat-abi=soft -fdeclspec -c -m32 -Os test.cpp test_helper.cpp
linker test.o test_helper.o test.sce
pause