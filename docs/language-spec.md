# MiniC Language Specification

## 1. Overview
MiniC is a strictly-typed, procedural subset of C designed for compilation to GPU architectures via NVIDIA Parallel Thread Execution (PTX). It eliminates high-level complexities such as pointer arithmetic, heap allocation, union types, and dynamic dispatch while retaining core C features necessary for numerical and compute-kernel algorithms.

## 2. Lexical Structure

### 2.1 Identifiers
- An identifier begins with an ASCII letter (`a`-`z`, `A`-`Z`) or underscore (`_`), followed by zero or more letters, digits (`0`-`9`), or underscores.
- Regex: `[a-zA-Z_][a-zA-Z0-9_]*`
- Case-sensitive.

### 2.2 Keywords
The following tokens are reserved keywords and cannot be used as identifiers:
- Types: `int`, `void`
- Control Flow: `if`, `else`, `while`, `return`
- Kernel Attributes (optional / target): `__global__`, `__device__`

### 2.3 Literals
- Integer literals: Decimal numbers consisting of one or more digits `[0-9]+`.
- MiniC integers are signed 32-bit two's complement integers (`int`), mapping to PTX `.s32` / `.b32`.

### 2.4 Operators and Delimiters
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Relational: `<`, `<=`, `>`, `>=`
- Equality: `==`, `!=`
- Logical: `&&`, `||`, `!`
- Assignment: `=`
- Punctuation: `;`, `,`, `(`, `)`, `{`, `}`

### 2.5 Comments and Whitespace
- Single-line comments: `// ...` to the end of the line.
- Multi-line comments: `/* ... */` (non-nesting).
- Whitespace: Spaces, tabs (`\t`), carriage returns (`\r`), and newlines (`\n`) serve as token separators and are otherwise ignored.

---

## 3. Formal Grammar (EBNF)

```ebnf
Program         ::= FunctionDecl*

FunctionDecl    ::= Type Identifier "(" ParamList? ")" CompoundStmt
ParamList       ::= Param ("," Param)*
Param           ::= Type Identifier
Type            ::= "int" | "void"

CompoundStmt    ::= "{" Stmt* "}"
Stmt            ::= VarDeclStmt
                  | ExprStmt
                  | IfStmt
                  | WhileStmt
                  | ReturnStmt
                  | CompoundStmt

VarDeclStmt     ::= Type Identifier ("=" Expr)? ";"
ExprStmt        ::= Expr? ";"

IfStmt          ::= "if" "(" Expr ")" Stmt ("else" Stmt)?
WhileStmt       ::= "while" "(" Expr ")" Stmt
ReturnStmt      ::= "return" Expr? ";"

Expr            ::= AssignmentExpr
AssignmentExpr  ::= Identifier "=" AssignmentExpr
                  | LogicalOrExpr

LogicalOrExpr   ::= LogicalAndExpr ( "||" LogicalAndExpr )*
LogicalAndExpr  ::= EqualityExpr ( "&&" EqualityExpr )*
EqualityExpr    ::= RelationalExpr ( ("==" | "!=") RelationalExpr )*
RelationalExpr  ::= AdditiveExpr ( ("<" | "<=" | ">" | ">=") AdditiveExpr )*
AdditiveExpr    ::= MultiplicativeExpr ( ("+" | "-") MultiplicativeExpr )*
MultiplicativeExpr ::= UnaryExpr ( ("*" | "/" | "%") UnaryExpr )*
UnaryExpr       ::= ("-" | "+" | "!") UnaryExpr
                  | PrimaryExpr

PrimaryExpr     ::= Identifier
                  | IntegerLiteral
                  | Identifier "(" ArgList? ")"
                  | "(" Expr ")"

ArgList         ::= Expr ("," Expr)*
```

---

## 4. Operator Precedence and Associativity

| Level | Operator | Description | Associativity |
|:-----:|:--------:|:------------|:-------------:|
| 1 | `( )`, `func()` | Parentheses, Function call | Left-to-Right |
| 2 | `+`, `-`, `!` | Unary plus, negation, logical NOT | Right-to-Left |
| 3 | `*`, `/`, `%` | Multiplicative | Left-to-Right |
| 4 | `+`, `-` | Additive | Left-to-Right |
| 5 | `<`, `<=`, `>`, `>=` | Relational | Left-to-Right |
| 6 | `==`, `!=` | Equality | Left-to-Right |
| 7 | `&&` | Logical AND | Left-to-Right |
| 8 | `\|\|` | Logical OR | Left-to-Right |
| 9 | `=` | Assignment | Right-to-Left |

---

## 5. Type System and Semantic Rules

1. **Primitive Types**:
   - `int`: Signed 32-bit two's complement integer.
   - `void`: Used solely as a return type for functions that do not return a value.
2. **Scoping**:
   - Block scoping with standard lexical nesting.
   - Variable shadowing in inner scopes is permitted, but duplicate declarations within the same immediate scope are rejected as semantic errors.
3. **Variable Lifecycle**:
   - All variables must be declared before use.
   - Use of undeclared variables produces an error with precise source coordinates.
4. **Function Calls**:
   - Functions must be declared prior to invocation.
   - Argument counts and types must match the parameter list.
5. **Constant Evaluation & Static Errors**:
   - Static division by zero (e.g. `x / 0` or `x % 0` where the divisor evaluates to constant `0`) is caught at compile-time as a diagnostic error.
6. **Return Correctness**:
   - Non-void functions must return an expression of type `int`.
   - Void functions must not return a value expression.
