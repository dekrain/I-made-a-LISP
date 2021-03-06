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

A compound `S-expression` is either a list expression, a vector expression, or a hash-map.
Example of a list expression: `(123 456 789)`, empty list `()`;
Example of a vector expression: `[123 456 789]`, empty vector `[]`;
Example of a hash-map: `{:a 123 :b 456 "c" (+ 1 (* 3 4))}`.
A hash-map is transformed by the reader into following form:
`{args...} -> (hash-map args...)`, case for last example: `(hash-map :a 123 :b 456 "c" (+ 1 (* 3 4)))`.

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

# Advanced
This section describes features of the language for advanced users.

## Metadata
The MAL language allows you to add metadata for every value in the interpreter!
This is accomplished by putting an additional field to the `MalValue` structure.
A "metadatum" of a value is an optional value that is fully transparent to most of the execution environment.
Metadata of values can be accessed with `meta` function, which takes 1 argument: target object.
A value with specified metadata can be created with `(with-meta value metavalue)` function.
It can also be called through a special reader macro like this: `^metavalue expression`.

Metadata can (not currently implemented) be used by the interpreter in the following scenarios:
-   By the reader (code values) - to track filenames and line numbers of occuring expressions.
    This can be useful for debugging (error/stack trace), but it comes with higher memory usage penalty.
    It will be possible to toggle this feature.
-   By the interpreter (code values) - to store some runtime profiling informations. For example it might
    be possible to replace constant expressions with their values to speed-up execution and to mark functions
    as `pure`, so they may also be considered when unfolding constants.

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