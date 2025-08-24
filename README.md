# sexpdf

**S**-**EXP**ession **D**ata **F**ormat, inspired by [sexpline](https://github.com/snmsts/sexpline/) but for C/C++. Zero dependencies (if you don't count the standard library). 

```c
#define SEXPDF_IMPLEMENTATION
#include "sexpdf.h"

int main(int argc, char **argv) {
    size_t size;
    const char *source = read_file("test.sedf", &size);
    if (!source)
        return 1;

    size_t count = 0;
    struct sedf_atom **atoms = NULL;
    struct sedf_parser parser;
    sedf_init(&parser, source, size);
    enum sedf_error_type err = sedf_parse(&parser, (struct sedf_atom**)&atoms, &count);
    if (err != SEXPDF_OK)
        return 1;

    if (atoms) {
        for (size_t i = 0; i < count; i++)
            sedf_free(atoms[i]);
        free(atoms);
    }
    if (source)
        free(source);
    return 0;
}
```

See [test.c](https://github.com/takeiteasy/sexpdf/blob/master/test.c) for a more comprehensive example.

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
