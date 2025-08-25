/* sexpdf -- https://github.com/takeiteasy/sexpdf

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
 along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#ifndef SEXPDF_HEADER
#define SEXPDF_HEADER
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

struct sedf_parser {
    const char *source;
    size_t length;
    size_t position;
};

struct sedf_pair {
    const char *key;
    struct sedf_atom *value;
};

struct sedf_object {
    struct sedf_pair *pairs;
    size_t count;
};

struct sedf_array {
    struct sedf_atom **items;
    size_t count;
};

struct sedf_atom {
    enum {
        SEXPDF_ATOM_NULL,
        SEXPDF_ATOM_OBJECT,
        SEXPDF_ATOM_ARRAY,
        SEXPDF_ATOM_STRING,
        SEXPDF_ATOM_NUMBER,
        SEXPDF_ATOM_BOOLEAN,
        SEXPDF_ATOM_SYMBOL
    } type;
    union {
        struct sedf_object *object;
        struct sedf_array *array;
        char *string;
        double number;
        int boolean;
        char *symbol;
    } value;
};

enum sedf_error_type {
    SEXPDF_OK = 0,
    SEXPDF_OUT_OF_MEMORY = -1,
    SEXPDF_UNEXPECTED_CHAR = -2,
    SEXPDF_UNEXPECTED_EOF = -3,
    SEXPDF_UNEXPECTED_TOKEN = -4,
    SEXPDF_UNKNOWN_ERROR = -5
};

void sedf_init(struct sedf_parser *parser, const char *source, size_t length);
enum sedf_error_type sedf_parse(struct sedf_parser *parser, struct sedf_atom **out, size_t *count);
void sedf_free(struct sedf_atom *atom);

#if __cplusplus
}
#endif
#endif // SEXPDF_HEADER

#ifdef SEXPDF_IMPLEMENTATION
enum sedf_token_type {
    SEXPDF_TOKEN_SYMBOL = 0,
    SEXPDF_TOKEN_OBJECT = 1 << 0,
    SEXPDF_TOKEN_ARRAY = 1 << 1,
    SEXPDF_TOKEN_STRING = 1 << 2,
    SEXPDF_TOKEN_PRIMITIVE = 1 << 3,
    SEXPDF_TOKEN_OBJECT_CLOSE = 1 << 4,
};

struct sedf_token {
    enum sedf_token_type type;
    const char *start;
    size_t length;
};

static int is_eof(struct sedf_parser *parser) {
    return parser->position >= parser->length;
}

static void consume(struct sedf_parser *parser) {
    parser->position++;
}

static enum sedf_error_type skip_whitespace(struct sedf_parser *parser) {
    while (!is_eof(parser))
        switch (parser->source[parser->position]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                consume(parser);
                break;
            default:
                return SEXPDF_OK;
        }
    return SEXPDF_OK;
}

static char cursor(struct sedf_parser *parser) {
    return is_eof(parser) ? '\0' : parser->source[parser->position];
}

static char* cursor_ptr(struct sedf_parser *parser) {
    return is_eof(parser) ? NULL : (char*)&parser->source[parser->position];
}

static char peek(struct sedf_parser *parser) {
    return is_eof(parser) || parser->position + 1 >= parser->length ? '\0' : parser->source[parser->position + 1];
}

static int is_valid_lisp_symbol_char(char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        return 1;
    if (c >= '0' && c <= '9')
        return 1;
    switch (c) {
        case '-':
        case '+':
        case '*':
        case '/':
        case '@':
        case '$':
        case '%':
        case '&':
        case '=':
        case '<':
        case '>':
        case '.':
        case '_':
        case '~':
        case '?':
        case '!':
        case '[':
        case ']':
        case '{':
        case '}':
        case '^':
        case ':':
            return 1;
        default:
            return 0;
    }
}

void create_token(struct sedf_token *token, enum sedf_token_type type, const char *start, size_t length) {
    token->type = type;
    token->start = start;
    token->length = length;
}

enum sedf_error_type sedf_next(struct sedf_parser *parser, struct sedf_token *out) {
    enum sedf_error_type err = SEXPDF_OK;
    if ((err = skip_whitespace(parser)) != SEXPDF_OK)
        goto BAIL;
    char c = cursor(parser);
    switch (c) {
        case ';':
            if (peek(parser) == ';') {
                while (!is_eof(parser) && cursor(parser) != '\n')
                    consume(parser);
            } else {
                err = SEXPDF_UNEXPECTED_CHAR;
                goto BAIL;
            }
            break;
        case '(':
            create_token(out, SEXPDF_TOKEN_OBJECT, cursor_ptr(parser), 1);
            consume(parser);
            break;
        case ')':
            create_token(out, SEXPDF_TOKEN_OBJECT_CLOSE, cursor_ptr(parser), 1);
            consume(parser);
            break;
        case '"':
            create_token(out, SEXPDF_TOKEN_STRING, cursor_ptr(parser), 1);
            consume(parser);
            break;
        case '#':
            create_token(out, SEXPDF_TOKEN_ARRAY, cursor_ptr(parser), 1);
            consume(parser);
            break;
        default:;
            if (!is_valid_lisp_symbol_char(c)) {
                err = SEXPDF_UNEXPECTED_CHAR;
                goto BAIL;
            }

            enum sedf_token_type token_type = (c == ':') ? SEXPDF_TOKEN_SYMBOL : SEXPDF_TOKEN_PRIMITIVE;
            create_token(out, token_type, cursor_ptr(parser), 0);
            size_t start = parser->position;
            while (is_valid_lisp_symbol_char(c)) {
                consume(parser);
                if (is_eof(parser))
                    goto FOUND;
                else {
                    switch ((c = cursor(parser))) {
                        case ' ':
                        case '\t':
                        case '\n':
                        case '\r':
                        case ')':
                            goto FOUND;
                        default:
                            break;
                    }
                };
            }
        FOUND:
            if (start == parser->position) {
                err = SEXPDF_UNEXPECTED_EOF;
                goto BAIL;
            } else {
                out->start = parser->source + start;
                out->length = parser->position - start;
            }
            break;
    }
BAIL:
    return err;
}

static struct sedf_atom* sedf_create_atom(void) {
    struct sedf_atom *atom = (struct sedf_atom*)malloc(sizeof(struct sedf_atom));
    if (atom)
        memset(atom, 0, sizeof(struct sedf_atom));
    return atom;
}

static char* sedf_create_string(const char *src, size_t length) {
    char *str = (char*)malloc(length + 1);
    if (str) {
        memcpy(str, src, length);
        str[length] = '\0';
    }
    return str;
}

static enum sedf_error_type sedf_parse_atom(struct sedf_parser *parser, struct sedf_atom *atom);

static enum sedf_error_type sedf_parse_object(struct sedf_parser *parser, struct sedf_atom *atom) {
    atom->type = SEXPDF_ATOM_OBJECT;
    atom->value.object = (struct sedf_object*)malloc(sizeof(struct sedf_object));
    if (!atom->value.object)
        return SEXPDF_OUT_OF_MEMORY;
    atom->value.object->pairs = NULL;
    atom->value.object->count = 0;
    
    struct sedf_token token;
    enum sedf_error_type err = SEXPDF_OK;
    size_t capacity = 8;
    struct sedf_pair *pairs = (struct sedf_pair*)malloc(capacity * sizeof(struct sedf_pair));
    if (!pairs)
        return SEXPDF_OUT_OF_MEMORY;

    while (1) {
        if ((err = skip_whitespace(parser)) != SEXPDF_OK)
            goto BAIL;
        if (cursor(parser) == ')') {
            consume(parser);
            break;
        }
        if ((err = sedf_next(parser, &token)) != SEXPDF_OK)
            goto BAIL;
        if (token.type != SEXPDF_TOKEN_SYMBOL) {
            err = SEXPDF_UNEXPECTED_TOKEN;
            goto BAIL;
        }

        if (atom->value.object->count >= capacity) {
            capacity *= 2;
            struct sedf_pair *new_pairs = (struct sedf_pair*)realloc(pairs, capacity * sizeof(struct sedf_pair));
            if (!new_pairs) {
                err = SEXPDF_OUT_OF_MEMORY;
                goto BAIL;
            }
            pairs = new_pairs;
        }
        
        pairs[atom->value.object->count].key = sedf_create_string(token.start, token.length);
        if (!pairs[atom->value.object->count].key) {
            err = SEXPDF_OUT_OF_MEMORY;
            goto BAIL;
        }
        
        struct sedf_atom *value_atom = sedf_create_atom();
        if (!value_atom) {
            err = SEXPDF_OUT_OF_MEMORY;
            goto BAIL;
        }
        
        if ((err = sedf_parse_atom(parser, value_atom)) != SEXPDF_OK) {
            sedf_free(value_atom);
            goto BAIL;
        }
        
        pairs[atom->value.object->count].value = value_atom;
        atom->value.object->count++;
    }
    atom->value.object->pairs = pairs;

BAIL:
    if (pairs && err != SEXPDF_OK) {
        for (size_t i = 0; i < atom->value.object->count; i++) {
            free((void*)pairs[i].key);
            if (pairs[i].value)
                sedf_free(pairs[i].value);
        }
        free(pairs);
    }
    return err;
}

static enum sedf_error_type sedf_parse_array(struct sedf_parser *parser, struct sedf_atom *atom) {
    size_t capacity = 8;
    struct sedf_atom **items = NULL;
    enum sedf_error_type err = SEXPDF_OK;

    atom->type = SEXPDF_ATOM_ARRAY;
    atom->value.array = (struct sedf_array*)malloc(sizeof(struct sedf_array));
    if (!atom->value.array) {
        err = SEXPDF_OUT_OF_MEMORY;
        goto BAIL;
    }
    atom->value.array->items = NULL;
    atom->value.array->count = 0;
    
    struct sedf_token token;
    if ((err = sedf_next(parser, &token)) != SEXPDF_OK)
        goto BAIL;
    if (token.type != SEXPDF_TOKEN_OBJECT) {
        err = SEXPDF_UNEXPECTED_TOKEN;
        goto BAIL;
    }

    if (!(items = (struct sedf_atom**)malloc(capacity * sizeof(struct sedf_atom*)))) {
        err = SEXPDF_OUT_OF_MEMORY;
        goto BAIL;
    }
    
    while (1) {
        if ((err = skip_whitespace(parser)) != SEXPDF_OK)
            goto BAIL;
        
        if (cursor(parser) == ')') {
            consume(parser);
            break;
        }
        
        if (atom->value.array->count >= capacity) {
            capacity *= 2;
            struct sedf_atom **new_items = (struct sedf_atom**)realloc(items, capacity * sizeof(struct sedf_atom*));
            if (!new_items) {
                err = SEXPDF_OUT_OF_MEMORY;
                goto BAIL;
            }
            items = new_items;
        }
        
        struct sedf_atom *item_atom = sedf_create_atom();
        if (!item_atom) {
            err = SEXPDF_OUT_OF_MEMORY;
            goto BAIL;
        }
        
        if ((err = sedf_parse_atom(parser, item_atom)) != SEXPDF_OK) {
            sedf_free(item_atom);
            goto BAIL;
        }
        
        items[atom->value.array->count] = item_atom;
        atom->value.array->count++;
    }
    
    atom->value.array->items = items;
    return SEXPDF_OK;
    
BAIL:
    if (items) {
        for (size_t i = 0; i < atom->value.array->count; i++) {
            if (items[i])
                sedf_free(items[i]);
        }
        free(items);
    }
    return err;
}

static enum sedf_error_type sedf_parse_string(struct sedf_parser *parser, struct sedf_atom *atom, struct sedf_token *token) {
    atom->type = SEXPDF_ATOM_STRING;
    size_t start_pos = parser->position;
    while (!is_eof(parser) && cursor(parser) != '"')
        consume(parser);
    if (is_eof(parser))
        return SEXPDF_UNEXPECTED_EOF;

    size_t length = parser->position - start_pos;
    atom->value.string = sedf_create_string(parser->source + start_pos, length);
    if (!atom->value.string)
        return SEXPDF_OUT_OF_MEMORY;
    consume(parser);
    return SEXPDF_OK;
}

static enum sedf_error_type sedf_parse_primitive(struct sedf_parser *parser, struct sedf_atom *atom, struct sedf_token *token) {
    if (token->length == 1 && *token->start == 't') {
        atom->type = SEXPDF_ATOM_BOOLEAN;
        atom->value.boolean = 1;
        return SEXPDF_OK;
    }
    if (token->length == 3 && strncmp(token->start, "nil", 3) == 0) {
        atom->type = SEXPDF_ATOM_NULL;
        return SEXPDF_OK;
    }
    
    char *endptr;
    double num = strtod(token->start, &endptr);
    if (endptr == token->start + token->length) {
        atom->type = SEXPDF_ATOM_NUMBER;
        atom->value.number = num;
        return SEXPDF_OK;
    }
    
    atom->type = SEXPDF_ATOM_SYMBOL;
    atom->value.symbol = sedf_create_string(token->start, token->length);
    if (!atom->value.symbol)
        return SEXPDF_OUT_OF_MEMORY;

    return SEXPDF_OK;
}

static enum sedf_error_type sedf_parse_symbol(struct sedf_parser *parser, struct sedf_atom *atom, struct sedf_token *token) {
    atom->type = SEXPDF_ATOM_SYMBOL;
    atom->value.symbol = sedf_create_string(token->start, token->length);
    if (!atom->value.symbol)
        return SEXPDF_OUT_OF_MEMORY;
    return SEXPDF_OK;
}

static enum sedf_error_type sedf_parse_atom(struct sedf_parser *parser, struct sedf_atom *atom) {
    enum sedf_error_type err = SEXPDF_OK;
    struct sedf_token token;

    if ((err = sedf_next(parser, &token)) != SEXPDF_OK)
        return err;
    switch (token.type) {
        case SEXPDF_TOKEN_OBJECT:
            return sedf_parse_object(parser, atom);
        case SEXPDF_TOKEN_ARRAY:
            return sedf_parse_array(parser, atom);
        case SEXPDF_TOKEN_STRING:
            return sedf_parse_string(parser, atom, &token);
        case SEXPDF_TOKEN_PRIMITIVE:
            return sedf_parse_primitive(parser, atom, &token);
        case SEXPDF_TOKEN_SYMBOL:
            return sedf_parse_symbol(parser, atom, &token);
        default:
            return SEXPDF_UNEXPECTED_TOKEN;
    }
}

void sedf_init(struct sedf_parser *parser, const char *source, size_t length) {
    parser->source = source;
    parser->length = length;
    parser->position = 0;
};

enum sedf_error_type sedf_parse(struct sedf_parser *parser, struct sedf_atom **out, size_t *count) {
    enum sedf_error_type err = SEXPDF_OK;
    size_t capacity = 8, _count = 0;
    struct sedf_atom **atoms = (struct sedf_atom**)malloc(capacity * sizeof(struct sedf_atom*));
    if (!atoms) {
        err = SEXPDF_OUT_OF_MEMORY;
        goto BAIL;
    }

    for (;;) {
        if ((err = skip_whitespace(parser)) != SEXPDF_OK)
            goto BAIL;
        if (is_eof(parser))
            break;

        if (_count >= capacity) {
            capacity *= 2;
            struct sedf_atom **new_atoms = (struct sedf_atom**)realloc(atoms, capacity * sizeof(struct sedf_atom*));
            if (!new_atoms) {
                err = SEXPDF_OUT_OF_MEMORY;
                goto BAIL;
            }
            atoms = new_atoms;
        }

        struct sedf_atom *atom = sedf_create_atom();
        if (!atom) {
            err = SEXPDF_OUT_OF_MEMORY;
            goto BAIL;
        }

        if ((err = sedf_parse_atom(parser, atom)) != SEXPDF_OK) {
            sedf_free(atom);
            goto BAIL;
        }

        atoms[_count] = atom;
        _count++;
    }

BAIL:
    if (atoms && err != SEXPDF_OK) {
        for (size_t i = 0; i < _count; i++)
            sedf_free(atoms[i]);
        free(atoms);
    }
    if (out)
        *out = (struct sedf_atom*)atoms;
    if (count)
        *count = _count;
    return err;
}

void sedf_free(struct sedf_atom *atom) {
    if (!atom)
        return;
    switch (atom->type) {
        case SEXPDF_ATOM_STRING:
            free(atom->value.string);
            break;
        case SEXPDF_ATOM_SYMBOL:
            free(atom->value.symbol);
            break;
        case SEXPDF_ATOM_OBJECT:
            if (atom->value.object) {
                for (size_t i = 0; i < atom->value.object->count; i++) {
                    free((void*)atom->value.object->pairs[i].key);
                    sedf_free(atom->value.object->pairs[i].value);
                }
                free(atom->value.object->pairs);
                free(atom->value.object);
            }
            break;
        case SEXPDF_ATOM_ARRAY:
            if (atom->value.array) {
                for (size_t i = 0; i < atom->value.array->count; i++)
                    sedf_free(atom->value.array->items[i]);
                free(atom->value.array->items);
                free(atom->value.array);
            }
            break;
        default:
            break;
    }
    free(atom);
}
#endif // SEXPDF_IMPLEMENTATION
