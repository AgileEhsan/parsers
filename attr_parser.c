#define _GNU_SOURCE

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

struct pair_t
{
    char* name;
    char* val;
    struct pair_t* next;
};

struct tag_node_t
{
    struct tag_t* tag;
    struct tag_node_t* next;
    struct tag_node_t* prev;
};

struct tag_list_t
{
    struct tag_node_t* head;
    struct tag_node_t* tail;
};

struct tag_t
{
    char* name;
    struct tag_t* parent;
    struct tag_list_t* children;
    struct pair_t* attributes;
};

typedef struct tag_t tag_t;
typedef tag_t* ptag_t;

char s_not_found[] = "Not Found!";
char* text;
int pos;
int len;
ptag_t root_tag, current_tag;

ptag_t init_tag(ptag_t parent, char* name)
{
    ptag_t new_tag = calloc(sizeof(tag_t), 1);
    (new_tag)->parent = parent;
    (new_tag)->name = name;
    (new_tag)->children = calloc(sizeof(struct tag_list_t), 1);
    return new_tag;
}

ptag_t find_child_tag(ptag_t parent, char* tag_name)
{
    struct tag_node_t* iter = parent->children->head;
    while (iter)
    {
        if (0 == strcmp(tag_name, iter->tag->name))
            return iter->tag;
        iter = iter->next;
    }
    return NULL;
}

char* find_attr(ptag_t parent, char* attr_name)
{
    struct pair_t* attr = parent->attributes;
    while (attr)
    {
        if (0 == strcmp(attr_name, attr->name))
            return attr->val;
        attr = attr->next;
    }
    return s_not_found;
}

void add_child(ptag_t root, ptag_t child)
{
    child->parent = root;
    struct tag_node_t* node = calloc(sizeof(struct tag_node_t), 1);
    node->tag = child;
    if (!root->children->head)
        root->children->head = root->children->tail = node;
    else {
        node->prev = root->children->tail;
        root->children->tail->next = node;
        root->children->tail = node;
    }
}

void add_attr(ptag_t root, char* name, char* val)
{
    struct pair_t* node = calloc(sizeof(struct pair_t), 1);
    node->name = name;
    node->val = val;
    node->next = root->attributes;
    root->attributes = node;
}

void error()
{
    fprintf(stdout, "!! Syntax Error near %d !!\n%s\n", pos, text+pos);
    fflush(stdout);
    exit(1);
}

void discard()
{
    for (; pos < len && isspace(text[pos]); ++pos);
}

void match(char expect)
{
    discard();
    if (text[pos] == expect)
        ++pos;
    else
        error();
}

void read_id(int* start, int* end)
{
    discard();
    *start = pos;
    while (isalnum(text[pos]) || text[pos] == '_')
        ++pos;
    *end = pos-1;
}

void read_quote(int* start, int* end)
{
    match('"');
    *start = pos;
    while ('"' != text[pos])
        ++pos;
    *end = pos-1;
    match('"');
}

void parse_attribute()
{
    int id_start, id_end;
    int val_start, val_end;
    char *id = 0, *val = 0;

    read_id(&id_start, &id_end);
    match('=');
    read_quote(&val_start, &val_end);

    asprintf(&id, "%.*s", id_end-id_start+1, text+id_start);
    asprintf(&val, "%.*s", val_end-val_start+1, text+val_start);
    add_attr(current_tag, id, val);
}

void parse_tags()
{
    int id_start, id_end;

    match('<');
    read_id(&id_start, &id_end);

    char* tag_name = 0;
    asprintf(&tag_name, "%.*s", id_end-id_start+1, text+id_start);

    ptag_t old = current_tag;
    current_tag = init_tag(current_tag, tag_name);
    /* printf("[ Enter ] - %s\n", tag_name); */

    if (old)
        add_child(old, current_tag);

    while (text[pos] != '>')
    {
        parse_attribute();
    }

    match('>');
    while (pos+1 < len && text[pos+1] != '/')
    {
        parse_tags();
    }

    match('<');
    match('/');
    read_id(&id_start, &id_end);

    if (strncmp(tag_name, text+id_start, id_end-id_start+1))
    {
        error();
    }

    /* printf("[ Exit ] - %s\n", tag_name); */
    match('>');
    current_tag = current_tag->parent;
}

char* resolve(char const* attr_path)
{
    char* temp = strdup(attr_path);
    char* ptr = strchr(temp, '~');
    char* attr_name = ptr+1;
    *ptr = 0;
    char* dir = strtok(temp, ".");
    ptag_t iter_tag = root_tag;
    do
    {
        iter_tag = find_child_tag(iter_tag, dir);
        dir = strtok(NULL, ".");
    } while (dir && iter_tag);

    char* ans = s_not_found;
    if (iter_tag)
    {
        ans = find_attr(iter_tag, attr_name);
    }
    free(temp);
    return ans;
}

void parse()
{
    root_tag = init_tag(NULL, "/");
    current_tag = root_tag;
    while (pos < len)
    {
        parse_tags();
    }
}

void traverse(ptag_t tag)
{
    if (! tag)
        return;
    printf("[ Enter ] %s\n", tag->name);
    struct tag_node_t* p = tag->children->head;
    while (p)
    {
        traverse(p->tag);
        p = p->next;
    }
    printf("[ Exit ] %s\n", tag->name);
}

int main()
{
    int n_lines, n_queries;
    size_t size;
    int n_bytes;
    char* line;

    scanf("%d %d\n", &n_lines, &n_queries);
    for (int i = 0; i < n_lines; ++i)
    {
        line = 0;
        size = 0;
        n_bytes = getline(&line, &size, stdin);
        line[n_bytes-1] = 0;
        if (text)
            asprintf(&text, "%s%.*s", text, n_bytes-1, line);
        else
            asprintf(&text, "%s", line);
        free(line);
    }

    len = strlen(text);
    parse();

    do
    {
        line = 0;
        size_t size = 0;
        size = getline(&line, &size, stdin);
        if (line[size-1] == '\n')
            line[size-1] = 0;

        puts(resolve(line));
        free(line);
    } while (--n_queries);
}
