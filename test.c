#define SEXPDF_IMPLEMENTATION
#include "sexpdf.h"
#include <stdio.h>

static char *read_file(const char *path, size_t *size) {
    char *result = NULL;
    FILE *file = fopen(path, "rb");
    if (!file)
        goto BAIL;
    fseek(file, 0, SEEK_END);
    size_t _size = ftell(file);
    if (size)
        *size = _size;
    fseek(file, 0, SEEK_SET);
    if (!(result = malloc(_size + 1)))
        goto BAIL;
    fread(result, 1, _size, file);
    result[_size] = '\0';
BAIL:
    if (file)
        fclose(file);
    return result;
}

void sedf_print_atom(struct sedf_atom *atom, int indent) {
    switch (atom->type) {
        case SEXPDF_ATOM_NULL:
            printf("%*snil\n", indent, "");
            break;
        case SEXPDF_ATOM_BOOLEAN:
            printf("%*s%s\n", indent, "", atom->value.boolean ? "true" : "false");
            break;
        case SEXPDF_ATOM_NUMBER:
            printf("%*s%.6g\n", indent, "", atom->value.number);
            break;
        case SEXPDF_ATOM_STRING:
            printf("%*s\"%s\"\n", indent, "", atom->value.string);
            break;
        case SEXPDF_ATOM_SYMBOL:
            printf("%*s%s\n", indent, "", atom->value.symbol);
            break;
        case SEXPDF_ATOM_OBJECT:
            printf("%*s{\n", indent, "");
            for (size_t i = 0; i < atom->value.object->count; i++) {
                printf("%*s  %s: ", indent, "", atom->value.object->pairs[i].key);
                sedf_print_atom(atom->value.object->pairs[i].value, indent + 1);
            }
            printf("%*s}\n", indent, "");
            break;
        case SEXPDF_ATOM_ARRAY:
            printf("%*s[\n", indent, "");
            for (size_t i = 0; i < atom->value.array->count; i++)
                sedf_print_atom(atom->value.array->items[i], indent + 2);
            printf("%*s]\n", indent, "");
            break;
    }
}

int main(int argc, char **argv) {
    size_t size;
    int result = 0;
    size_t count = 0;
    struct sedf_atom **atoms = NULL;
    const char *file = read_file("test.sedf", &size);
    if (!file) {
        result = 1;
        goto BAIL;
    }

    struct sedf_parser parser;
    sedf_init(&parser, file, size);
    enum sedf_error_type err = sedf_parse(&parser, (struct sedf_atom**)&atoms, &count);
    if (err != SEXPDF_OK) {
        printf("Parse error: %d\n", err);
        result = 2;
        goto BAIL;
    }
    
    printf("Successfully parsed %zu atoms\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("=== ATOM #%zu ===\n", i + 1);
        sedf_print_atom(atoms[i], 0);
        printf("\n");
    }

BAIL:
    if (atoms) {
        for (size_t i = 0; i < count; i++)
            sedf_free(atoms[i]);
        free(atoms);
    }
    free((void*)file);
    return result;
}
