# DDS_Core System Documentation
DDS_Core is a .NET class library that provides a high-level API for using DDS.

## Usages

DDS_Core can be used in .NET applications with .NET 8+ to interact with the DDS.

A simple example of using DDS_Core to solve a board is shown below:
```c#
using DDS_Core;
...

var cfg = new SolverConfig()
             {
               TTKind          = TTKind.Large
             , DefaultMemoryMB = 256
             , MaximumMemoryMB = 1024
             };

using (var ctx = new SolverContext(cfg))
{
    var rc = ctx.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);
}
```


## API Reference




