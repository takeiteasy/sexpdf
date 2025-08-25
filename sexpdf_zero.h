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

struct sedf_parser {
    unsigned int pos;     /* offset in the SEDF string */
    unsigned int toknext; /* next token to allocate */
    int toksuper;         /* superior token node, e.g. parent object or array */
};

enum sedf_token_type {
    SEXPDF_UNDEFINED = 0,
    SEXPDF_SEXP = 1 << 0,     /* S-expression: (...) */
    SEXPDF_ARRAY = 1 << 1,    /* Array: #(...) */
    SEXPDF_STRING = 1 << 2,   /* String: "..." */
    SEXPDF_PRIMITIVE = 1 << 3 /* Primitive: :keyword, number, symbol */
};

enum sedf_err {
    /* Not enough tokens were provided */
    SEXPDF_ERROR_NOMEM = -1,
    /* Invalid character */
    SEXPDF_ERROR_INVAL = -2,
    /* The data is not complete, more bytes expected */
    SEXPDF_ERROR_PART = -3
};

struct sedf_token {
    enum sedf_token_type type;
    int start;
    int end;
    int size;
    int parent;
};

void sedf_init(struct sedf_parser *parser);
int sedf_parse(struct sedf_parser *parser, const char *input, const int input_length, struct sedf_token *tokens, const unsigned int max_tokens);

#if __cplusplus
}
#endif
#endif // SEXPDF_HEADER

#ifdef SEXPDF_IMPLEMENTATION
#ifndef NULL
#define NULL (void*)0
#endif

/* Allocates a fresh unused token from the token pool. */
static struct sedf_token *sedf_alloc_token(struct sedf_parser *parser, 
                                           struct sedf_token *tokens,
                                           const int num_tokens) {
    struct sedf_token *tok;
    if (parser->toknext >= num_tokens)
        return NULL;
    tok = &tokens[parser->toknext++];
    tok->start = tok->end = -1;
    tok->size = 0;
    tok->parent = -1;
    return tok;
}

/* Fills token type and boundaries. */
static void sedf_fill_token(struct sedf_token *token, const enum sedf_token_type type,
                            const int start, const int end) {
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

/* Fills next available token with primitive. */
static int sedf_parse_primitive(struct sedf_parser *parser, const char *input,
                                const int len, struct sedf_token *tokens,
                                const int num_tokens) {
    struct sedf_token *token;
    int start = parser->pos;
    for (; parser->pos < len && input[parser->pos] != '\0'; parser->pos++) {
        switch (input[parser->pos]) {
        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case '(':
        case ')':
            goto found;
        default:
            break;
        }
        if (input[parser->pos] < 32 || input[parser->pos] >= 127) {
            parser->pos = start;
            return SEXPDF_ERROR_INVAL;
        }
    }

found:
    if (tokens == NULL) {
        parser->pos--;
        return 0;
    }
    if ((token = sedf_alloc_token(parser, tokens, num_tokens)) == NULL) {
        parser->pos = start;
        return SEXPDF_ERROR_NOMEM;
    }
    sedf_fill_token(token, SEXPDF_PRIMITIVE, start, parser->pos);
    token->parent = parser->toksuper;
    parser->pos--;
    return 0;
}

/* Fills next token with SEDF string. */
static int sedf_parse_string(struct sedf_parser *parser, const char *input,
                             const int len, struct sedf_token *tokens,
                             const int num_tokens) {
    struct sedf_token *token;
    /* Skip starting quote */
    int start = parser->pos++;

    for (; parser->pos < len && input[parser->pos] != '\0'; parser->pos++) {
        char c = input[parser->pos];

        /* end of string */
        if (c == '\"') {
            if (tokens == NULL)
                return 0;
            token = sedf_alloc_token(parser, tokens, num_tokens);
            if (token == NULL) {
                parser->pos = start;
                return SEXPDF_ERROR_NOMEM;
            }
            sedf_fill_token(token, SEXPDF_STRING, start + 1, parser->pos);
            token->parent = parser->toksuper;
            return 0;
        }

        if (c == '\\' && parser->pos + 1 < len) {
            parser->pos++;
            switch (input[parser->pos]) {
            case '\"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
                break;
            /* escaped symbol \xXX */
            case 'x':
                parser->pos++;
                for (int i = 0; i < 2 && parser->pos < len && input[parser->pos] != '\0'; i++) {
                    /* If it isn't a hex character we have an error */
                    if (!((input[parser->pos] >= 48 && input[parser->pos] <= 57) ||   /* 0-9 */
                          (input[parser->pos] >= 65 && input[parser->pos] <= 70) ||   /* A-F */
                          (input[parser->pos] >= 97 && input[parser->pos] <= 102))) { /* a-f */
                        parser->pos = start;
                        return SEXPDF_ERROR_INVAL;
                    }
                    parser->pos++;
                }
                parser->pos--;
                break;
            /* escaped symbol \uXXXX */
            case 'u':
                parser->pos++;
                for (int i = 0; i < 4 && parser->pos < len && input[parser->pos] != '\0'; i++) {
                    /* If it isn't a hex character we have an error */
                    if (!((input[parser->pos] >= 48 && input[parser->pos] <= 57) ||   /* 0-9 */
                          (input[parser->pos] >= 65 && input[parser->pos] <= 70) ||   /* A-F */
                          (input[parser->pos] >= 97 && input[parser->pos] <= 102))) { /* a-f */
                        parser->pos = start;
                        return SEXPDF_ERROR_INVAL;
                    }
                    parser->pos++;
                }
                parser->pos--;
                break;
            /* Unexpected symbol */
            default:
                parser->pos = start;
                return SEXPDF_ERROR_INVAL;
            }
        }
    }
    parser->pos = start;
    return SEXPDF_ERROR_PART;
}

void sedf_init(struct sedf_parser *parser) {
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

int sedf_parse(struct sedf_parser *parser, const char *input, const int input_length, 
               struct sedf_token *tokens, const unsigned int max_tokens) {
    int r, i;
    struct sedf_token *token;
    int count = parser->toknext;

    for (; parser->pos < input_length && input[parser->pos] != '\0'; parser->pos++) {
        char c;
        enum sedf_token_type type;
        switch ((c = input[parser->pos])) {
        case '(':
            count++;
            if (tokens == NULL)
                break;
            if ((token = sedf_alloc_token(parser, tokens, max_tokens)) == NULL)
                return SEXPDF_ERROR_NOMEM;
            if (parser->toksuper != -1) {
                struct sedf_token *t = &tokens[parser->toksuper];
                t->size++;
                token->parent = parser->toksuper;
            }
            token->type = SEXPDF_SEXP;
            token->start = parser->pos;
            parser->toksuper = parser->toknext - 1;
            break;
        case ')':
            if (tokens == NULL)
                break;
            for (i = parser->toknext - 1; i >= 0; i--) {
                token = &tokens[i];
                if (token->start != -1 && token->end == -1) {
                    if (token->type != SEXPDF_SEXP && token->type != SEXPDF_ARRAY) {
                        return SEXPDF_ERROR_INVAL;
                    }
                    parser->toksuper = -1;
                    token->end = parser->pos + 1;
                    break;
                }
            }
            /* Error if unmatched closing bracket */
            if (i == -1)
                return SEXPDF_ERROR_INVAL;
            for (; i >= 0; i--) {
                token = &tokens[i];
                if (token->start != -1 && token->end == -1) {
                    parser->toksuper = i;
                    break;
                }
            }
            break;
        case '#':
            /* Look ahead for array syntax #(...) */
            if (parser->pos + 1 < input_length && input[parser->pos + 1] == '(') {
                count++;
                if (tokens == NULL) {
                    parser->pos++; /* Skip the '(' */
                    break;
                }
                if ((token = sedf_alloc_token(parser, tokens, max_tokens)) == NULL) 
                    return SEXPDF_ERROR_NOMEM;
                
                if (parser->toksuper != -1) {
                    struct sedf_token *t = &tokens[parser->toksuper];
                    t->size++;
                    token->parent = parser->toksuper;
                }
                token->type = SEXPDF_ARRAY;
                token->start = parser->pos; /* Start at '#' */
                parser->toksuper = parser->toknext - 1;
                parser->pos++; /* Skip the '(' - the main loop will increment pos again */
            } else {
                /* Treat as primitive if not followed by '(' */
                if ((r = sedf_parse_primitive(parser, input, input_length, tokens, max_tokens)) < 0)
                    return r;
                count++;
                if (parser->toksuper != -1 && tokens != NULL)
                    tokens[parser->toksuper].size++;
            }
            break;
        case '\"':
            if ((r = sedf_parse_string(parser, input, input_length, tokens, max_tokens)) < 0)
                return r;
            count++;
            if (parser->toksuper != -1 && tokens != NULL)
                tokens[parser->toksuper].size++;
            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        /* Everything else is a primitive (keywords, numbers, symbols) */
        default:
            if ((r = sedf_parse_primitive(parser, input, input_length, tokens, max_tokens)) < 0)
                return r;
            count++;
            if (parser->toksuper != -1 && tokens != NULL)
                tokens[parser->toksuper].size++;
            break;
        }
    }

    /* Unmatched opened object or array */
    if (tokens != NULL)
        for (i = parser->toknext - 1; i >= 0; i--)
            if (tokens[i].start != -1 && tokens[i].end == -1)
                return SEXPDF_ERROR_PART;
    return count;
}
#endif // SEXPDF_IMPLEMENTATION
