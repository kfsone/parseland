naive_cpp parser implementation
-------------------------------

This is the first reference implementation of the ParseLand grammar, and it's naive in the sense
that it was written ad-hoc without significant design or emphasis on performance: that is.


## naive: design

A bottom-up approach, without worrying about how it might influence the bigger picture.

For example, lack of line/column tracking: I have opinions on where and how to do this, but it
would have several significant design impacts; error handling, the current implementation is
essentially single-error-per-parse, there's no real opportunity or option for multiple errors
or error recovery.


## naive: performance

On the one hand, several obvious big-impact performance considerations got made (using stringview
instead of string), and the occasional minor performance tweak slips past my fingertips, I'm
trying to generally eschew implementation decisions driven by performance concerns. There will
be aspects that perform horribly in some circumstances - e.g. no line/column tracking.


## structure

This implementation is split into a library based on scanner.cpp with the core scanner, and an app
which demonstrates using the scanner and implements an example AST builder.


## concepts

Since I'm limiting myself to somewhere between C++17 and C++20 I didn't have access to std::expected,
so I chose to revisit my own value-or-err return type with learnings from time using Go and Rust. I'm
not entirely crazy about how it translates into C++, especially when moving errors between different
value-type returns, and for the verbosity in some places - but I'm putting this down to the whole
"naive" business.

Within Scanner I most commonly use a Result<Token> which I alias as TResult. In the app I most commonly
use a Result<OwningPointer<ASTNode>> which I aliased PResult (pointer).