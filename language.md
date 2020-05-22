This is a language strongly based on @kanaka's Make-A-Lisp project, which is an interpreted, dynamic LISP-based language.

# The basics
The MAL language is based on `S-expression`s.
An `S-expression` is either a literal value (basic type-expression) or a compound expression.

The basic types in MAL are currently:
-   A number, e.g. `123`, `-123`, `1_000_000`=`1000000`
-   A nil `nil`, true `true` or false `false`
-   A text type:
-   -   A symbol, e.g. `example`
-   -   A string, e.g. `"asdf"`
-   -   A keyword, e.g. `:abcd`

Additional basic types are:
-   A function, e.g. builtin function `list`, a user-defined function returned by `fn` or `macro` forms
-   An atom, created with `atom` builtin function

A compound `S-expression` is either a list expression or a vector expression (in future: map expression).
Example of a list expression: `(123 456 789)`, empty list `()`
Example of a vector expression: `[123 456 789]`, empty vector `[]`

A non-empty `list` expression is also called a `call` expression.

A `call` expression consists of 2 components:
-   The callee expression - first element of the list, e.g. `+` in the `(+ 1 (* 2 3))` expression
-   The arguments - rest of the list, e.g. `1`, `2`, and `3` in `(* 1 2 3)` expression

An expression in the source code can be either a literal expression or a compound expression.
Additionally comments are supported.
A comment beggins with the `;` token and lasts to the end of the line.
```clojure
(+ 1 2 3 4 5) ; I'm a comment
(* 2 4 ; I'm also a comment
    6 8)
```

# Evaluation
Every MAL program is a sequence of `S-expression`s and is evaluated in linear order.
An `S-expression` `expr` is evaluated as follows:

If `expr` is a literal:
-   if `expr` is a `symbol`: lookup the `symbol` in the current environment (a.k.a. scope) and return found value.
-   otherwise, return the `expr`

If `expr` is a compound expression:
-   if `expr` is a `vector`: evaluate every element of the vector in order, then return the new vector.
-   if `expr` is an empty `list`: return the list.
-   if `expr` is a `call`: see below.

The evaluation of `call` expression is as follows:
1. Take the `callee` expression. If `callee` is a symbol, and if the symbol has a special value, stop here and evaluate a special form (see: Special forms)
2. Otherwise, evaluate the `callee` expression.
3. If result of the `callee` expression is a `macro`, call the macro with `arguments`, and evaluate the result from the beginning.
4. Otherwise (`callee` result must be a function), evaluate all `arguments`, then call the function with the evaluated `arguments`, return the result.

# Special forms
In MAL, there exist special expressions, which look like this:

`(form-symbol arguments...)`

Special forms are evaluated seperately by their own evaluation rules.
An example of such a special form would be `(def sym-name value)`.
This form takes 2 arguments: a symbol `sym-name` and an expression `value`.
The `value` is evaluated and is bound to the current environment with the name `sym-name`.

There exist various special forms in MAL, full index below.

# Special form index
This section lacks descriptions; for descriptions, see: `src/interpreter.cpp:Interpreter/Apply()`
-   `(def name value)`
-   `(let* (name1 val1 name2 val2 ...) expr)`
-   `(do expr1 expr2 ... exprLast)`
-   `(if test case-true <optional>case-false-or-empty)`
-   `(fn params body)`
-   `(macro params body)`
-   `(quote expr)`
-   `(quasiquote expr)`
-   `(macroexpand expr)`
-   `(try* body err-name err-body)`

The following forms are availible inside a `quasiquote` expression:
-   `(unquote exor)`
-   `(splice-unquote expr-list)`

# Standard library
see: `src/core_lib.cpp`;
see: `bootstrap.mal`