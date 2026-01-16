# Verbose Debug Mode for Member Functions

## Enabling Verbose Output

To enable detailed debug output for member function debugging, define `VERBOSE` during compilation.

### For CMake builds:

Add to your CMakeLists.txt or use:
```bash
cmake -DCMAKE_C_FLAGS="-DVERBOSE" ..
```

### For Visual Studio:

1. Right-click project → Properties
2. C/C++ → Preprocessor → Preprocessor Definitions
3. Add `VERBOSE` to the list

### For command-line builds:

```bash
gcc -DVERBOSE -c expression.c
```

## Debug Output

When `VERBOSE` is defined, you'll see output like:

```
DEBUG: TokenIdentifier seen: 'foo'
DEBUG: It's a variable
DEBUG: Parser->Mode = 0, RunModeRun = 0
DEBUG: Got variable, Typ->Base = 14
DEBUG: Pushing variable 'foo' onto stack
DEBUG: After push, StackTop->Val->Typ->Base = 14
DEBUG: Before handling dot/arrow
DEBUG: Parser->Mode = 0, RunModeRun = 0
DEBUG: StackTop->Val->Typ->Base = 14
DEBUG: Calling ExpressionMemberFunctionCall (run mode)
DEBUG: ExpressionMemberFunctionCall called
DEBUG: StackTop->Val->Typ->Base = 14
DEBUG: Token = 41 (Dot=41, Arrow=42)
DEBUG: TypeStruct = 14
```

## Disabling Debug Output

Simply rebuild without the `-DVERBOSE` flag for clean output.
