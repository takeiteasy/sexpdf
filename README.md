# lisd

> [!WARNING]
> Work in progress

`lisd` is a **LIS**p **D**ata format. It's inspired by [sexpline](https://github.com/snmsts/sexpline/), but for C/C++. This library is largely based off [JSMN](https://github.com/zserge/jsmn/tree/master), an embeddable descending parser for JSON by [zserge](https://github.com/zserge).

Define `LISD_IMPLEMENTATION` in *one* C/C++ file to create the implementation.

## Usage

```c
#define LISD_IMPLEMENTATION
#include "lisd.h"

// read_file, token_type_to_string, print_token implementations omitted for brevity. They can be found in test.c if needed.

int main(int argc, const char *argv[]) {
    struct ld_parser parser;
    struct ld_token tokens[256];
    int token_count;

    int input_len = 0;
    const char *input = read_file("test.lisd", (size_t*)&input_len);
    if (!input) {
        printf("Failed to read input file\n");
        return 1;
    }

    ld_init(&parser);
    if ((token_count = ld_parse(&parser, input, input_len, tokens, 256)) < 0) {
        printf("Parse error: %d\n", token_count);
        return 1;
    }
    
    printf("Successfully parsed %d tokens:\n\n", token_count);
    for (int i = 0; i < token_count; i++) {
        printf("Token %d: Type=%s, Start=%d, End=%d, Size=%d, Parent=%d\n", 
               i, 
               token_type_to_string(tokens[i].type),
               tokens[i].start,
               tokens[i].end,
               tokens[i].size,
               tokens[i].parent);
        printf("  Content: \"");
        print_token(input, &tokens[i]);
        printf("\"\n\n");
    }
    return 0;
}
```

## TODO

- [ ] Exporting (from C) API
- [ ] Convert lisd <-> JSON tool

## LICENSE
```
lisd, lisp data format

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
