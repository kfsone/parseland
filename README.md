Kfsone's ParseLand -- Parsing Experiments
=========================================

This is a collection of evaluation-experiments trying out various different parsing tools/libs
for a small DSL. Hopefully it might become a sort of Rosetta-Stone-meets-Recipebook.

# License
---------

This code/project is licensed under MIT License terms -- see [LICENSE](LICENSE).


# The Grammar
-------------

## Credit: Tommy Krul (aka Scarfguy)
I have to give Tommy Krul of Super Evil MegaCorp credit for the original grammar this is based
on, which is used to define data structures. The grammar presented here is not that grammar,
the clean simplicity is Tommy's, the quirks are my exemplification of quirks we ran into making
tools to implement that grammar.


## Examples

### Trivial

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

