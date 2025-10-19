#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

typedef enum { TYPE_NULL, TYPE_BUTTON, TYPE_SLIDER, TYPE_LABEL } ComponentType;

typedef struct {
  char *command;
  char *data;
} ButtonArgs;

typedef struct {
  char *command;
  int max_value;
  int min_value;
} SliderArgs;

typedef struct {
  char *data;
} LabelArgs;

typedef struct Component {
  ComponentType type;
  void *args;
  struct Component **children;
  int children_count;
} Component;

typedef struct {
  char *background;
  char *normal_color;
  char *normal_back;
  char *hover_color;
  char *hover_back;
  char *font;
  struct {
    int x;
    int y;
  } offset;
  struct {
    int width;
    int height;
  } size;
} General;

typedef struct {
  General general;
  Component *components;
  int components_count;
} Config;

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

  size_t read_len = fread(content, 1, length, file);
  content[read_len] = '\0';
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

/* --- General parser --- */
int parse_general_node(yaml_document_t *doc, yaml_node_t *general_node,
                       General *general) {
  if (!general_node || general_node->type != YAML_MAPPING_NODE)
    return 0;

  *general = (General){0};

  general->background =
      get_scalar_strdup(get_mapping_value(doc, general_node, "background"));
  general->normal_color =
      get_scalar_strdup(get_mapping_value(doc, general_node, "normal_color"));
  general->normal_back =
      get_scalar_strdup(get_mapping_value(doc, general_node, "normal_back"));
  general->hover_color =
      get_scalar_strdup(get_mapping_value(doc, general_node, "hover_color"));
  general->hover_back =
      get_scalar_strdup(get_mapping_value(doc, general_node, "hover_back"));
  general->font =
      get_scalar_strdup(get_mapping_value(doc, general_node, "font"));

  // Parse offset array [x, y]
  yaml_node_t *offset_node = get_mapping_value(doc, general_node, "offset");
  if (offset_node && offset_node->type == YAML_SEQUENCE_NODE) {
    yaml_node_item_t *items = offset_node->data.sequence.items.start;
    if (items < offset_node->data.sequence.items.top) {
      yaml_node_t *x_node = yaml_document_get_node(doc, *items);
      get_scalar_int(x_node, &general->offset.x);
      items++;
    }
    if (items < offset_node->data.sequence.items.top) {
      yaml_node_t *y_node = yaml_document_get_node(doc, *items);
      get_scalar_int(y_node, &general->offset.y);
    }
  }

  // Parse size array [width, height]
  yaml_node_t *size_node = get_mapping_value(doc, general_node, "size");
  if (size_node && size_node->type == YAML_SEQUENCE_NODE) {
    yaml_node_item_t *items = size_node->data.sequence.items.start;
    if (items < size_node->data.sequence.items.top) {
      yaml_node_t *width_node = yaml_document_get_node(doc, *items);
      get_scalar_int(width_node, &general->size.width);
      items++;
    }
    if (items < size_node->data.sequence.items.top) {
      yaml_node_t *height_node = yaml_document_get_node(doc, *items);
      get_scalar_int(height_node, &general->size.height);
    }
  }

  return 1;
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
  *(parent->children[parent->children_count]) = *child;
  parent->children_count++;
  return 1;
}

/* parse a single component mapping node into Component (returns 1 on success)
 */
int parse_component_node(yaml_document_t *doc, yaml_node_t *comp_node,
                         Component *out) {
  if (!comp_node || comp_node->type != YAML_MAPPING_NODE)
    return 0;
  *out = (Component){0};

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
      return 1;
    for (yaml_node_item_t *it = args_node->data.sequence.items.start;
         it < args_node->data.sequence.items.top; ++it) {
      yaml_node_t *child_node = yaml_document_get_node(doc, *it);
      Component temp_child;
      if (!parse_component_node(doc, child_node, &temp_child))
        continue;
      if (!append_child(out, &temp_child)) {
        return 0;
      }
    }
    return 1;
  } else if (strcmp(type_str, "button") == 0) {
    out->type = TYPE_BUTTON;
    ButtonArgs *b = malloc(sizeof(ButtonArgs));
    *b = (ButtonArgs){0};
    if (args_node && args_node->type == YAML_MAPPING_NODE) {
      b->command =
          get_scalar_strdup(get_mapping_value(doc, args_node, "command"));
      b->data = get_scalar_strdup(get_mapping_value(doc, args_node, "data"));
    }
    out->args = b;
    return 1;
  } else if (strcmp(type_str, "slider") == 0) {
    out->type = TYPE_SLIDER;
    SliderArgs *s = malloc(sizeof(SliderArgs));
    *s = (SliderArgs){0};
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
    *l = (LabelArgs){0};
    if (args_node && args_node->type == YAML_MAPPING_NODE) {
      l->data = get_scalar_strdup(get_mapping_value(doc, args_node, "data"));
    }
    out->args = l;
    return 1;
  }

  return 0;
}

static void cleanup_parser(yaml_parser_t *parser, yaml_document_t *document) {
  if (document)
    yaml_document_delete(document);
  if (parser)
    yaml_parser_delete(parser);
}

static int parse_general_section(yaml_document_t *document, yaml_node_t *root,
                                 Config *config) {
  yaml_node_t *general_node = get_mapping_value(document, root, "general");
  if (general_node) {
    return parse_general_node(document, general_node, &config->general);
  }
  return 1;
}

static int parse_components_section(yaml_document_t *document,
                                    yaml_node_t *root, Config *config) {
  yaml_node_t *components_node =
      get_mapping_value(document, root, "components");

  if (!components_node || components_node->type != YAML_SEQUENCE_NODE) {
    return 0;
  }

  int count = 0;
  Component *components = NULL;

  for (yaml_node_item_t *it = components_node->data.sequence.items.start;
       it < components_node->data.sequence.items.top; ++it) {
    yaml_node_t *comp = yaml_document_get_node(document, *it);
    Component temp;

    if (!parse_component_node(document, comp, &temp))
      continue;

    Component *newarr = realloc(components, sizeof(Component) * (count + 1));
    if (!newarr) {
      free(components);
      return 0;
    }

    components = newarr;
    components[count] = temp;
    count++;
  }

  config->components = components;
  config->components_count = count;
  return 1;
}

/* --- main parse_config --- */
int parse_config(const char *yaml_data, Config *config) {
  yaml_parser_t parser;
  yaml_document_t document;
  int result = 1;

  *config = (Config){0};

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
    cleanup_parser(&parser, &document);
    return 0;
  }

  result &= parse_general_section(&document, root, config);
  result &= parse_components_section(&document, root, config);

  cleanup_parser(&parser, &document);
  return result;
}

/* free helpers */
void free_general(General *general) {
  if (!general)
    return;
  free(general->background);
  free(general->normal_color);
  free(general->normal_back);
  free(general->hover_color);
  free(general->hover_back);
  free(general->font);
}

void free_component(Component *c) {
  if (!c)
    return;

  switch (c->type) {
  case TYPE_NULL: {
    for (int i = 0; i < c->children_count; ++i) {
      free_component(c->children[i]);
      free(c->children[i]);
    }
    free(c->children);
    break;
  }
  case TYPE_BUTTON: {
    ButtonArgs *b = c->args;
    if (b) {
      free(b->command);
      free(b->data);
      free(b);
    }
    break;
  }
  case TYPE_SLIDER: {
    SliderArgs *s = c->args;
    if (s) {
      free(s->command);
      free(s);
    }
    break;
  }
  case TYPE_LABEL: {
    LabelArgs *l = c->args;
    if (l) {
      free(l->data);
      free(l);
    }
    break;
  }
  }
}

void free_config(Config *config) {
  if (!config)
    return;

  free_general(&config->general);

  for (int i = 0; i < config->components_count; ++i) {
    free_component(&config->components[i]);
  }
  free(config->components);

  free(config);
}

/* debug print */
void print_general(General *general) {
  if (!general)
    return;

  printf("General settings:\n");
  printf("  background: %s\n",
         general->background ? general->background : "(null)");
  printf("  normal_color: %s\n",
         general->normal_color ? general->normal_color : "(null)");
  printf("  normal_back: %s\n",
         general->normal_back ? general->normal_back : "(null)");
  printf("  hover_color: %s\n",
         general->hover_color ? general->hover_color : "(null)");
  printf("  hover_back: %s\n",
         general->hover_back ? general->hover_back : "(null)");
  printf("  font: %s\n", general->font ? general->font : "(null)");
  printf("  offset: [%d, %d]\n", general->offset.x, general->offset.y);
  printf("  size: [%d, %d]\n", general->size.width, general->size.height);
}

void print_component(Component *c, int indent) {
  for (int i = 0; i < indent; ++i)
    putchar(' ');

  if (c->type == TYPE_NULL) {
    printf("- null (children: %d)\n", c->children_count);
    for (int i = 0; i < c->children_count; ++i)
      print_component(c->children[i], indent + 2);
  } else if (c->type == TYPE_BUTTON) {
    ButtonArgs *b = c->args;
    printf("- button command=%s text=%s\n", b->command ? b->command : "(null)",
           b->data ? b->data : "(null)");
  } else if (c->type == TYPE_SLIDER) {
    SliderArgs *s = c->args;
    printf("- slider command=%s min=%d max=%d\n",
           s->command ? s->command : "(null)", s->min_value, s->max_value);
  } else if (c->type == TYPE_LABEL) {
    LabelArgs *l = c->args;
    printf("- label text=%s\n", l->data ? l->data : "(null)");
  }
}

void print_config(Config *config) {
  printf("=== CONFIG PARSED ===\n");
  print_general(&config->general);

  printf("\nParsed %d components:\n", config->components_count);
  for (int i = 0; i < config->components_count; ++i) {
    print_component(&config->components[i], 0);
    printf("\n");
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

  Config *config = calloc(1, sizeof(Config));

  if (!parse_config(yaml, config)) {
    fprintf(stderr, "parse_config failed\n");
    free(yaml);
    return 1;
  }

  print_config(config);
  free_config(config);
  free(yaml);

  return 0;
}
