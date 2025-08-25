# sexpdf

> [!WARNING]
> Work in progress

**S**-**EXP**ession **D**ata **F**ormat, inspired by [sexpline](https://github.com/snmsts/sexpline/) but for C/C++. See [test.sedf](https://github.com/takeiteasy/sexpdf/blob/master/test.sedf) for a sample.

There are two different implementations: see [sexpdf.h](https://github.com/takeiteasy/sexpdf/blob/master/sexpdf.h) and [test.c](https://github.com/takeiteasy/sexpdf/blob/master/test.c) for a more comprehensive example that uses heap allocation and the standard library. Or see [sexpdf_zero.h](https://github.com/takeiteasy/sexpdf/blob/master/sexpdf_zero.h) and [zero.c](https://github.com/takeiteasy/sexpdf/blob/master/zero.c) for a c89 version that has no dependencies and doesn't allocate anthing.

They are both single header implementations so just `#define SEXPDF_IMPLEMENTATION` before including. The zero allocation version is largely based off [JSMN](https://github.com/zserge/jsmn/tree/master), an embeddable descending parser for JSON by [zserge](https://github.com/zserge).

## TODO

- [ ] Exporting (from C) API
- [ ] Convert SEDF <-> JSON tool

## LICENSE
```
sexpdf

Copyright (C) 2025 George Watson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
