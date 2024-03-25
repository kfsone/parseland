Kfsone's ParseLand -- Parsing Experiments
=========================================

🚧 WORK IN PROGRESS 🚧

*SEE [InProgress.md](InProgress.md) for details of which projects are (most) WIP*

ParseLand aims to be a combination of learnings and analyses of assorted parser tools
and libraries across different language ecosystems - primarily C++, C, Rust, Go, Python,
but if others prove interesting I'll look into them.

This project was inspired by the surprising-not-surprising[\*] scarceness of good parser
tools for Rust. 

(\*1: there are some good parser tools for Rust, but The Old Ways of writing parsers
and generating abstract syntax trees do not translate well to Rust)

I went thru so many of the parsers for Rust I started to realize that the latest one I
was trying to just a fork of an earlier one.

Which lead to the [blog post](...) that sparked this project.


# Background

## Who I am

I've been writing text-manipulation code since I was a kid in the 80s, perhaps the most
meta being my own MUD language. It's always been an axis of my professional work, too.

## This project

There are tutorials for each and every parser generator/tool out there, but they all
have their own distilled set of experiments that are often designed to avoid running
you into whatever pain point that toolset experiences.

So ParseLand presents a meaningful toy grammar - we should be able to use it to process
a suite of sample files and compare caveats: Writing a parser with _this_ gets the job
done, but if you have to write explicit grammar rules and code for every possible
error condition that might be a problem for you? If the only possible error you can
give the user is, defacto: "syntax error.\nsegmentation fault" you might not want it
in your prod environment?


# License

This code/project is licensed under MIT License terms -- see [LICENSE](LICENSE).


# Links

[cpp-peglib](https://github.com/yhirose/cpp-peglib): C++17 header-only PEG library.
[ParserTL](https://github.com/BenHanson/parsertl17): C++17 modular (combinator?) parser generator.



# The Grammar

## Credit: Tommy Krul (aka Scarfguy)
I have to give Tommy Krul of Super Evil MegaCorp credit for the original grammar this is based
on, which is used to define data structures. The grammar presented here is not that grammar,
the clean simplicity is Tommy's, the quirks are my exemplification of quirks we ran into making
tools to implement that grammar.


## Examples

I'll provide a number of examples in their own folder structure eventually.


### Trivial

This is a simple, should-parse OK example.

```
// ConnectionState provides a set of constants for the current state of a connectable.
enum ConnectionState
{
    DISCONNECTED,
    CONNECTED,
    ERROR
}

type Connection
{
    string              mName
    ConnectionState     mState = ConnectionState::DISCONNECTED
    User                mRecentUsers[] = {}
    float               mCostPerMinute
    int                 mKbPerSecond = 1000000  /* default to 1mb/s */
}
```

### Errors

These are cases that might look ok but should provide the user some kind of feedback
that they're done something wrong, and this is a big part of the rub - we want our users
to have a good experience when an error goes wrong, and be able to fix it quickly.

```
enum ConnectionState  /* Describe the state of a connection.
{
  DISCONNECTED,       /* The default case assumes no connection made.
  CONNECTED,
  ERROR,              /* Things aren't in an error state by default.
}

type Connection { }
```

In _this_ example none of the block-comments have been closed, and how you get the
grammar to handle this nicely can be a huge deal.

One parser tool I experimented with could only report the last, top-most production it
had tried to match, resulting in it saying:

```
1:1 | enum ConnectionState
    | ^-- expected 'type'
```

(Why is too involved for this readme)


## Details

The original grammar is used at SEMC to describe data types used in the asset conversion pipeline
of SEMC's proprietary and awesome game engine. Various things rely on it.

Features:
- `type` and `enum` definitions,
- C++-ish inheritance (`type X : Y`) for types,
- Dynamic-sized arrays of user-defined types (but not atoms),
- Optional defaults,
- Parse-time dependency resolution,
- C++ comments - line and nestable-block style


## The projects

I'll be trying to implement the grammar in various different parsing tools and for different
languages - C++ flex/bison with CMake, maybe a C++ peg grammar, versions in Rust, Golang and
Python.


# Credits/Thank you

Thanks to [mingodad](https://github.com/mingodad) for fixing up my pseudo-EBNF and pointing me
to the awesome [ParserTL Playground](https://mingodad.github.io/parsertl-playground/playground/)
(select 'Parseland parser' from the drop down) and introducing me to [ParserTL](https://github.com/BenHanson/parsertl17): C++17 modular (combinator?) parser generator.



