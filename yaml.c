#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

typedef enum { TYPE_NULL, TYPE_BUTTON, TYPE_SLIDER, TYPE_LABEL } ComponentType;

typedef struct {
  char *command;
  char *valid_data;
  char *invalid_data;
} ButtonArgs;

typedef struct {
  char *command;
  int max_value;
  int min_value;
} SliderArgs;

typedef struct {
  char *valid_data;
  char *invalid_data;
} LabelArgs;

typedef struct Component {
  ComponentType type;
  void *args;
  struct Component **children;
  int children_count;
} Component;

typedef struct {
  int 
} General;
char *strdup_or_null(const char *s) {
  if (!s)
    return NULL;
  char *r = malloc(strlen(s) + 1);
  if (r)
    strcpy(r, s);
  return r;
}

char *read_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file)
    return NULL;
  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *content = malloc(length + 1);
  if (!content) {
    fclose(file);
    return NULL;
  }
  fread(content, 1, length, file);
  content[length] = '\0';
  fclose(file);
  return content;
}

yaml_node_t *get_mapping_value(yaml_document_t *doc, yaml_node_t *mapping,
                               const char *key) {
  if (!mapping || mapping->type != YAML_MAPPING_NODE)
    return NULL;
  for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start;
       pair < mapping->data.mapping.pairs.top; ++pair) {
    yaml_node_t *key_node = yaml_document_get_node(doc, pair->key);
    if (key_node && key_node->type == YAML_SCALAR_NODE &&
        strcmp((char *)key_node->data.scalar.value, key) == 0) {
      return yaml_document_get_node(doc, pair->value);
    }
  }
  return NULL;
}

int get_scalar_int(yaml_node_t *node, int *out) {
  if (!node || node->type != YAML_SCALAR_NODE)
    return 0;
  char *endptr;
  long v = strtol((char *)node->data.scalar.value, &endptr, 10);
  if (endptr == (char *)node->data.scalar.value)
    return 0;
  *out = (int)v;
  return 1;
}

char *get_scalar_strdup(yaml_node_t *node) {
  if (!node || node->type != YAML_SCALAR_NODE)
    return NULL;
  return strdup_or_null((char *)node->data.scalar.value);
}

/* allocate and append a child component to parent */
int append_child(Component *parent, Component *child) {
  Component **newc = realloc(
      parent->children, sizeof(Component *) * (parent->children_count + 1));
  if (!newc)
    return 0;
  parent->children = newc;
  parent->children[parent->children_count] = malloc(sizeof(Component));
  if (!parent->children[parent->children_count])
    return 0;
  *(parent->children[parent->children_count]) = *child; // copy
  parent->children_count++;
  return 1;
}

/* parse a single component mapping node into Component (returns 1 on success)
 */
int parse_component_node(yaml_document_t *doc, yaml_node_t *comp_node,
                         Component *out) {
  if (!comp_node || comp_node->type != YAML_MAPPING_NODE)
    return 0;
  memset(out, 0, sizeof(Component));

  yaml_node_t *type_node = get_mapping_value(doc, comp_node, "type");
  yaml_node_t *args_node = get_mapping_value(doc, comp_node, "args");
  if (!type_node || type_node->type != YAML_SCALAR_NODE)
    return 0;
  const char *type_str = (char *)type_node->data.scalar.value;

  if (strcmp(type_str, "null") == 0) {
    out->type = TYPE_NULL;
    out->children = NULL;
    out->children_count = 0;
    if (!args_node || args_node->type != YAML_SEQUENCE_NODE)
      return 1; // empty null is allowed
    for (yaml_node_item_t *it = args_node->data.sequence.items.start;
         it < args_node->data.sequence.items.top; ++it) {
      yaml_node_t *child_node = yaml_document_get_node(doc, *it);
      Component temp_child;
      if (!parse_component_node(doc, child_node, &temp_child))
        continue;
      if (!append_child(out, &temp_child)) {
        // cleanup temp_child allocations
        // free allocated args inside temp_child
        // We'll let caller free everything on error for simplicity
        return 0;
      }
    }
    return 1;
  } else if (strcmp(type_str, "button") == 0) {
    out->type = TYPE_BUTTON;
    ButtonArgs *b = malloc(sizeof(ButtonArgs));
    b->command = NULL;
    b->valid_data = NULL;
    b->invalid_data = NULL;
    if (args_node && args_node->type == YAML_MAPPING_NODE) {
      b->command =
          get_scalar_strdup(get_mapping_value(doc, args_node, "command"));
      b->valid_data =
          get_scalar_strdup(get_mapping_value(doc, args_node, "valid_data"));
      b->invalid_data =
          get_scalar_strdup(get_mapping_value(doc, args_node, "invalid_data"));
    }
    out->args = b;
    return 1;
  } else if (strcmp(type_str, "slider") == 0) {
    out->type = TYPE_SLIDER;
    SliderArgs *s = malloc(sizeof(SliderArgs));
    s->command = NULL;
    s->max_value = 0;
    s->min_value = 0;
    if (args_node && args_node->type == YAML_MAPPING_NODE) {
      s->command =
          get_scalar_strdup(get_mapping_value(doc, args_node, "command"));
      get_scalar_int(get_mapping_value(doc, args_node, "max_value"),
                     &s->max_value);
      get_scalar_int(get_mapping_value(doc, args_node, "min_value"),
                     &s->min_value);
    }
    out->args = s;
    return 1;
  } else if (strcmp(type_str, "label") == 0) {
    out->type = TYPE_LABEL;
    LabelArgs *l = malloc(sizeof(LabelArgs));
    l->valid_data =
        get_scalar_strdup(get_mapping_value(doc, args_node, "valid_data"));
    l->invalid_data =
        get_scalar_strdup(get_mapping_value(doc, args_node, "invalid_data"));
    out->args = l;
    return 1;
  }

  return 0;
}

/* --- main parse_config --- */
/* components_out is an allocated array pointer and count_out is set to number
 * of components */
int parse_config(const char *yaml_data, Component **components_out,
                 int *count_out) {
  yaml_parser_t parser;
  yaml_document_t document;

  *components_out = NULL;
  *count_out = 0;

  if (!yaml_parser_initialize(&parser))
    return 0;
  yaml_parser_set_input_string(&parser, (const unsigned char *)yaml_data,
                               strlen(yaml_data));
  if (!yaml_parser_load(&parser, &document)) {
    yaml_parser_delete(&parser);
    return 0;
  }

  yaml_node_t *root = yaml_document_get_root_node(&document);
  if (!root || root->type != YAML_MAPPING_NODE) {
    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    return 0;
  }

  yaml_node_t *components_node =
      get_mapping_value(&document, root, "components");
  if (!components_node || components_node->type != YAML_SEQUENCE_NODE) {
    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    return 0;
  }

  for (yaml_node_item_t *it = components_node->data.sequence.items.start;
       it < components_node->data.sequence.items.top; ++it) {
    yaml_node_t *comp = yaml_document_get_node(&document, *it);
    Component temp;
    if (!parse_component_node(&document, comp, &temp))
      continue;
    Component *newarr =
        realloc(*components_out, sizeof(Component) * (*count_out + 1));
    if (!newarr) {
      // allocation failed; should free previously allocated components
      // simple cleanup: free doc and parser and return 0 (caller gets no
      // components)
      yaml_document_delete(&document);
      yaml_parser_delete(&parser);
      return 0;
    }
    *components_out = newarr;
    (*components_out)[*count_out] = temp; // copy
    (*count_out)++;
  }

  yaml_document_delete(&document);
  yaml_parser_delete(&parser);
  return 1;
}

/* free helpers */
void free_component(Component *c) {
  if (!c)
    return;
  if (c->type == TYPE_NULL) {
    for (int i = 0; i < c->children_count; ++i) {
      free_component(c->children[i]);
      free(c->children[i]);
    }
    free(c->children);
  } else if (c->type == TYPE_BUTTON) {
    ButtonArgs *b = c->args;
    if (b) {
      free(b->command);
      free(b->valid_data);
      free(b->invalid_data);
      free(b);
    }
  } else if (c->type == TYPE_SLIDER) {
    SliderArgs *s = c->args;
    if (s) {
      free(s->command);
      free(s);
    }
  } else if (c->type == TYPE_LABEL) {
    LabelArgs *l = c->args;
    if (l) {
      free(l->valid_data);
      free(l->invalid_data);
      free(l);
    }
  }
}

/* debug print */
void print_component(Component *c, int indent) {
  for (int i = 0; i < indent; ++i)
    putchar(' ');
  if (c->type == TYPE_NULL) {
    printf("- null (children: %d)\n", c->children_count);
    for (int i = 0; i < c->children_count; ++i)
      print_component(c->children[i], indent + 2);
  } else if (c->type == TYPE_BUTTON) {
    ButtonArgs *b = c->args;
    printf("- button command=%s valid=%s invalid=%s\n",
           b->command ? b->command : "(null)",
           b->valid_data ? b->valid_data : "(null)",
           b->invalid_data ? b->invalid_data : "(null)");
  } else if (c->type == TYPE_SLIDER) {
    SliderArgs *s = c->args;
    printf("- slider command=%s min=%d max=%d\n",
           s->command ? s->command : "(null)", s->min_value, s->max_value);
  } else if (c->type == TYPE_LABEL) {
    LabelArgs *l = c->args;
    printf("- label valid=%s invalid=%s\n",
           l->valid_data ? l->valid_data : "(null)",
           l->invalid_data ? l->invalid_data : "(null)");
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <config.yaml>\n", argv[0]);
    return 1;
  }
  char *yaml = read_file(argv[1]);
  if (!yaml) {
    fprintf(stderr, "read failed\n");
    return 1;
  }

  Component *components = NULL;
  int count = 0;
  if (!parse_config(yaml, &components, &count)) {
    fprintf(stderr, "parse_config failed\n");
    free(yaml);
    return 1;
  }

  printf("Parsed %d components:\n", count);
  for (int i = 0; i < count; ++i)
    print_component(&components[i], 0);

  /* cleanup */
  for (int i = 0; i < count; ++i) {
    free_component(&components[i]);
  }
  free(components);
  free(yaml);
  return 0;
}
