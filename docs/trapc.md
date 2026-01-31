# itrapc.md
31 January 2026
Robin.Rowe@trapc.org

TrapC is an safe-by-design extension to the C programming language proposed by Robin Rowe in February 2025 as ISO C Committeee whitepaper N3423. TrapC has memory-safe pointers, is typesafe even when using void pointers, and has error handling mechanism called trap that is much more lightweight and foolproof than C++ exceptions. TrapC has a few features borrowed from C++ that are helpful when building safety-critical systems, such as member functions and constructors and destructors. TrapC is very lightweight, is a safety extension of C, not a subset of C++. 

TrapC is implemented as an interpreter and as a compiler. The TrapC interpreter itrapc forked from the picoc C interpreter. The TrapC compiler trapc forked from the tinycc C compiler.

## Interpreter itrapc Files

clibrary.c
clibrary.h
debug.c
expression.c
expression.h
heap.c
heap.h
include.c
include.h
interpreter.h
itrapc.h
lex.c
lex.h
license.h
parse.c
parse.h
platform.c
platform.h
platform.h
table.c
table.h
type.c
type.h
variable.c
variable.h

# Architecture of itrapc 

# itrapc Architecture Documentation

## Overview

itrapc is a portable C interpreter designed to execute C source code directly without requiring compilation. It implements a complete C runtime environment with support for variables, functions, types, expressions, and memory management.

## Core Architecture Components

### 1. **Engine Structure (`struct Engine`)**

The central state container that holds all interpreter components:

* **Global Tables**: For variables, string literals, and reserved words
* **Memory Management**: Heap and stack allocation systems
* **Type System**: Built-in type definitions and type management
* **Parser State**: Current parsing context and source position
* **Debugger Support**: Breakpoint management and debugging state
* **C Library Integration**: Standard library function mappings

### 2. **Type System**

itrapc implements a comprehensive type system with the following features:

* **Base Types**: All C primitive types (int, char, float, double, void, etc.)
* **Derived Types**: Pointers, arrays, functions, structs, unions, enums
* **Type Metadata**: Size, alignment, and derived type relationships
* **Member Functions**: Support for struct member functions (with mangled names)
* **Type Tables**: Hash tables for efficient type lookup and management

### 3. **Variable Management**

* **Scope Management**: Local and global variable scoping with ScopeID tracking
* **Allocation Strategies**: Heap vs. stack allocation based on lifetime
* **LValue/RValue Tracking**: Distinguishes between modifiable and temporary values
* **String Literal Pooling**: Dedicated table for string constant management

### 4. **Memory Management**

* **Stack Frames**: Function call stack with local variable tables
* **Heap Allocation**: Freelist-based memory allocation with bucket optimization
* **Memory Alignment**: Platform-specific alignment using `ALIGN_TYPE`
* **Cleanup Tracking**: Automatic cleanup of allocated resources

### 5. **Parser and Lexer**

* **Token-Based Parsing**: Comprehensive lexical token definitions (72+ tokens)
* **Source Position Tracking**: File, line, and column information
* **Macro Processing**: Preprocessor directive handling
* **Interactive Mode**: REPL-style execution with line buffering

### 6. **Expression Evaluation**

* **Operator Precedence**: Full C operator precedence implementation
* **Expression Stack**: Stack-based expression evaluation
* **Type Coercion**: Automatic numeric and pointer type conversions
* **Function/Macro Calls**: Parameter passing and evaluation

### 7. **Hash Table System**

* **Multiple Table Types**: Global, local, string, type, and breakpoint tables
* **Prime Number Sizes**: Optimized hash distribution using prime numbers
* **Chained Hashing**: Collision resolution via linked lists
* **String Interning**: Shared string table for identifier storage

### 8. **Platform Abstraction**

* **Cross-Platform Support**: UNIX\_HOST and WIN32 configurations
* **I/O Abstraction**: Platform-independent input/output functions
* **Memory Alignment**: Architecture-specific alignment handling
* **Build Configuration**: Conditional compilation for features

## Key Data Structures

### `struct Value`

The fundamental unit representing any value in itrapc:

c

```
struct Value {
    struct ValueType *Typ;      // Type information
    union AnyValue *Val;        // Actual data
    struct Value *LValueFrom;   // Containing value for LValues
    char ValOnHeap;            // Allocation location flags
    char IsLValue;             // Modifiability flag
    int ScopeID;               // Scope tracking
    // ... additional metadata
};
```

### `struct ValueType`

Type definition structure:

c

```
struct ValueType {
    enum BaseType Base;        // Fundamental type category
    int ArraySize;            // For array types
    int Sizeof;               // Size in bytes
    int AlignBytes;           // Alignment requirements
    const char *Identifier;   // Type name
    struct Table *Members;    // Struct/union members
    // ... type relationships
};
```

### `struct Table`

Generic hash table implementation:

c

```
struct Table {
    short Size;               // Table size (prime number)
    short OnHeap;             // Allocation flag
    struct TableEntry **HashTable;  // Bucket array
};
```

## Execution Flow

1. **Initialization** (`EngineInitialize`)

   * Memory allocation

   * Type system setup

   * Global table initialization

2. **Parsing** (`EngineParse`)

   * Lexical analysis

   * Syntax tree construction

   * Symbol table population

3. **Execution**

   * Statement-by-statement evaluation

   * Expression computation

   * Function calls and returns

4. **Cleanup** (`EngineCleanup`)

   * Memory deallocation

   * Resource release

   * State reset

## Platform Support Features

### Memory Alignment

c

```
#ifdef __hppa__ || __sparc__
    #define ALIGN_TYPE double
#else
    #define ALIGN_TYPE void*
#endif
```

### Hash Table Sizing

* **Small**: 53, 97, 193
* **Medium**: 389, 769, 1543
* **Large**: 3079, 6151, 12289

Prime numbers are used for optimal hash distribution and reduced collisions.

## Debugging Support

* **Breakpoint Management**: Hash table for breakpoint storage
* **Source Position Tracking**: File, line, column for error reporting
* **Debug Mode**: Conditional compilation for debugging features
* **Manual Breakpoints**: Programmatic breakpoint setting

## Library Integration

* **Standard C Library**: stdio, stdlib, string, math, time, etc.
* **Dynamic Registration**: Library functions can be added at runtime
* **Intrinsic Functions**: Built-in optimized implementations

## Configuration Options

### Compile-Time Defines:

* `DEBUGGER`: Enable debugging features
* `USE_READLINE`: Interactive line editing
* `DEBUG_HEAP`: Heap allocation debugging
* Various debug flags for specific components

### Runtime Configuration:

* Stack size specification
* Interactive vs. batch mode
* Debugger enable/disable

## Error Handling

* **Structured Error Reporting**: Source position context
* **Type Mismatch Detection**: Assignment and parameter validation
* **Memory Access Validation**: Pointer dereference safety
* **Scope Violation Detection**: Variable access rules

## Performance Optimizations

* **Freelist Buckets**: Reduced fragmentation for common sizes
* **String Interning**: Shared string storage
* **Type Caching**: Frequently used type reuse
* **Expression Stack**: Efficient evaluation without recursion

## Extensibility

* **Library Registration**: `IncludeRegister()` for new libraries
* **Platform Functions**: Platform-specific function overrides
* **Type System Extension**: Custom type creation and manipulation
* **Variable Tracking**: Additional metadata through variable-type tables

## Security Considerations

* **Memory Boundaries**: Stack and heap separation
* **Pointer Validation**: Safe dereferencing
* **Type Safety**: Runtime type checking
* **Scope Enforcement**: Variable access restrictions

This architecture enables itrapc to provide a complete, portable C execution environment while maintaining efficiency and extensibility across different platforms and use cases.
