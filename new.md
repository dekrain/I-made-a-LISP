This document describes some additional features that might be implemented to MAL language (DK MAL) in the future.

# Universal trampoline
Currently, Tail-calls are implemped by including a trampoline inside
`mal.Interpreter/EvaluateExpression()`, which repetidely calls `/Apply()` (or stops with a `/EvalAst()`)
and checks the TC mark returned by `/EvaluateExpression()` if it should continue execution.

This reduces native stack usage, but there are still calls that are not in tail position, or cannot be
handled by `/EvaluateExpression()`. This includes builtin-function calls, macro-expansion calls (but
only the macro call, return expression is TCO'd), non-last branch of `do`, test branch of `if`, etc.

The plan is to create an universal trampoline function, which takes current frame of new explicit
call stack and calls the appropiate handler. This would make it easier to inspect the call stack of
the interpreter during runtime and to more precisely control stack overflows (currently this is done
inside `/EvaluateExpression()` which includes a guard that tracks total ammount of calls to this function
in one instance of the interpreter).

# Memory manager
Currently, every instance of `MalValue` exists in context-dependent place and can hold references to resources,
automatically managed by a reference-counter. This may compicate tracking of memory usage and potential memory leaks.

# Constant caching/tracking
Currently, if an expression is evaluated, which includes runtime evaluation of a constant, for example:
```clojure
(if (< arg
    (* 50 4) ; A constant!
    ) arg nil)
```
the constant is evaluated every time the expression is executed.

A potential solution may be to track when a constant is produced, and when a constant is calculated based on
other constants. When the outermost constant expression is detected, it may be replaced with the calculated constant
(e.g. via code metadata; note: ADD METADATA!).

This can be further extended to more complex construct, such as in this `if` expression:
```clojure
(if (> (* 5 10) (* 7 7)) br-t br-f)
```
it can be detected that the test-expression `(> (* 5 10) (* 7 7))` is constant, and may be replaced by a value (
also possibly implicitly-converted to a boolean because this parameter is a truth value):
```clojure
(if true br-t br-f)
```
This can be further reduced to just `br-t`, which becomes the final form of the whole `if` expression.

This concept may be further extended to detect `pure` functions, that is functions without side-effects.
The resulting `pure` functions may be then considered by constant-unfolding mechanism.