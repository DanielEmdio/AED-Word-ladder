//
// AED, November 2022 (Tomás Oliveira e Silva)
//
// Second practical assignement (speed run)
//
// Place your student numbers and names here
//   N.Mec. XXXXXX  Name: XXXXXXX
//
// Do as much as you can
//   1) MANDATORY: complete the hash table code
//      *) hash_table_create
//      *) hash_table_grow
//      *) hash_table_free
//      *) find_word
//      +) add code to get some statistical data about the hash table
//   2) HIGHLY RECOMMENDED: build the graph (including union-find data) -- use the similar_words function...
//      *) define_representative
//      *) add_edge
//   3) RECOMMENDED: implement breadth-first search in the graph
//      *) breadh_first_search
//   4) RECOMMENDED: list all words belonging to a connected component
//      *) list_connected_component
//   5) RECOMMENDED: find the shortest path between to words
//      *) breadh_first_search
//      *) path_finder
//      *) test the smallest path from bem to mal
//         [ 0] bem
//         [ 1] tem
//         [ 2] teu
//         [ 3] meu
//         [ 4] mau
//         [ 5] mal
//      *) find other interesting word ladders
//   6) OPTIONAL: compute the diameter of a connected component and list the longest word chain
//      *) breadh_first_search
//      *) connected_component_diameter
//   7) OPTIONAL: print some statistics about the graph
//      *) graph_info
//   8) OPTIONAL: test for memory leaks
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//
// static configuration
//

#define _max_word_size_ 32

//
// data structures (SUGGESTION --- you may do it in a different way)
//

typedef struct adjacency_node_s adjacency_node_t;
typedef struct hash_table_node_s hash_table_node_t;
typedef struct hash_table_s hash_table_t;

struct adjacency_node_s
{
  adjacency_node_t *next;    // link to the next adjacency list node
  hash_table_node_t *vertex; // the other vertex
};

struct hash_table_node_s
{
  // the hash table data
  char word[_max_word_size_]; // the word
  hash_table_node_t *next;    // next hash table linked list node
  // the vertex data
  adjacency_node_t *head;      // head of the linked list of adjancency edges
  int visited;                 // visited status (while not in use, keep it at 0)
  hash_table_node_t *previous; // breadth-first search parent
  // the union find data
  hash_table_node_t *representative; // the representative of the connected component this vertex belongs to
  int number_of_vertices;            // number of vertices of the conected component (only correct for the representative of each connected component)
  int number_of_edges;               // number of edges of the conected component (only correct for the representative of each connected component)
};

struct hash_table_s
{
  unsigned int hash_table_size;   // the size of the hash table array
  unsigned int number_of_entries; // the number of entries in the hash table
  unsigned int number_of_edges;   // number of edges (for information purposes only)
  hash_table_node_t **heads;      // the heads of the linked lists
};

//
// allocation and deallocation of linked list nodes (done)
//

static adjacency_node_t *allocate_adjacency_node(void)
{
  adjacency_node_t *node;

  node = (adjacency_node_t *)malloc(sizeof(adjacency_node_t));
  if (node == NULL)
  {
    fprintf(stderr, "allocate_adjacency_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_adjacency_node(adjacency_node_t *node)
{
  free(node);
}

static hash_table_node_t *allocate_hash_table_node(void)
{
  hash_table_node_t *node;

  node = (hash_table_node_t *)malloc(sizeof(hash_table_node_t));
  if (node == NULL)
  {
    fprintf(stderr, "allocate_hash_table_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}

//
// hash table stuff (mostly to be done)
//

unsigned int crc32(const char *str)
{
  static unsigned int table[256];
  unsigned int crc;

  if (table[1] == 0u) // do we need to initialize the table[] array?
  {
    unsigned int i, j;

    for (i = 0u; i < 256u; i++)
      for (table[i] = i, j = 0u; j < 8u; j++)
        if (table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  }
  crc = 0xAED02022u; // initial value (chosen arbitrarily)
  while (*str != '\0')
    crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
  return crc;
}

static hash_table_t *hash_table_create(unsigned int hash_table_size)
{
  hash_table_t *hash_table;
  unsigned int i;

  // allocate memory for the hash_table 'header'
  hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
  if (hash_table == NULL)
  {
    fprintf(stderr, "create_hash_table: out of memory\n");
    exit(1);
  }

  hash_table->hash_table_size = hash_table_size;
  hash_table->number_of_edges = hash_table->number_of_entries = 0;

  // allocate space for the hash_table array (array of pointers to the linked lists)
  hash_table->heads = (hash_table_node_t **)malloc((size_t)hash_table_size * sizeof(hash_table_node_t *));
  for (i = 0u; i < hash_table->hash_table_size; i++)
    hash_table->heads[i] = NULL;

  printf("hash table created (size: %d)\n", hash_table_size);

  return hash_table;
}

static void hash_table_free(hash_table_t *hash_table)
{
  // loop over all of the hash_table entries
  for (unsigned int i = 0u; i < hash_table->hash_table_size; i++)
  {
    hash_table_node_t *node = hash_table->heads[i];
    hash_table_node_t *next_node;

    adjacency_node_t *adj_node;
    adjacency_node_t *next_adj_node;

    // for eacch entrie, loop over the linked list and free every node
    while (node != NULL)
    {
      next_node = node->next;

      // also clean up the adjacency nodes
      adj_node = node->head;
      while (adj_node != NULL)
      {
        next_adj_node = adj_node->next;
        free_adjacency_node(adj_node);
        adj_node = next_adj_node;
      }

      free_hash_table_node(node);
      node = next_node;
    }
  }

  // final clean up
  free(hash_table->heads);
  free(hash_table);
}

static void hash_table_grow(hash_table_t *hash_table)
{
  unsigned int new_hash_table_size = hash_table->hash_table_size * 2;

  hash_table_node_t **new_hash_table_heads = (hash_table_node_t **)malloc((size_t)new_hash_table_size * sizeof(hash_table_node_t *));
  for (unsigned int i = 0u; i < hash_table->hash_table_size; i++)
    new_hash_table_heads[i] = NULL;

  // loop over the old hash_table
  for (unsigned int i = 0u; i < hash_table->hash_table_size; i++)
  {
    hash_table_node_t *node = hash_table->heads[i];
    hash_table_node_t *next_node_saved_pointer;

    while (node != NULL)
    {
      next_node_saved_pointer = node->next; // save the pointer to the next hash_table node from the current hash_table
      node->next = NULL;                    // set the pointer of the current node to null (it will be placed in the end of a linked list)

      unsigned int new_idx = crc32(node->word) % new_hash_table_size; // calculate new index with the new hash table size
      hash_table_node_t *new_hash_table_node_pointer = new_hash_table_heads[new_idx];

      // assign the current node to the new hash_table
      new_hash_table_heads[new_idx] = node;
      node->next = new_hash_table_node_pointer;

      node = next_node_saved_pointer;
    }
  }

  // clean up
  free(hash_table->heads);

  hash_table->heads = new_hash_table_heads;
  hash_table->hash_table_size = new_hash_table_size;

  printf("hash table resized (new size: %d, entrie: %d)\n", new_hash_table_size, hash_table->number_of_entries);
}

static hash_table_node_t *find_word(hash_table_t *hash_table, const char *word, int insert_if_not_found)
{
  // resize hash_table if needed
  if (hash_table->number_of_entries > hash_table->hash_table_size)
    hash_table_grow(hash_table);

  unsigned i = crc32(word) % hash_table->hash_table_size;
  hash_table_node_t *node = NULL;
  bool node_found = false;

  // loop over the linked list in the current hash_table position to 'look' for the node
  node = hash_table->heads[i];
  while (node != NULL && !node_found)
  {
    // check if node word is equal to the word we are looking for
    node_found = !strcmp(node->word, word);

    if (node_found) break;

    node = node->next;
  }

  if (!node_found && insert_if_not_found)
  {
    // fill in all of the values on the new node
    node = allocate_hash_table_node();
    node->next = node->previous = NULL;
    node->head = NULL;
    node->number_of_edges = node->number_of_vertices = node->visited = 0;
    node->representative = node;
    node->visited = 0;
    strcpy(node->word, word);

    // insert the new node (beginning of the linked list)
    hash_table_node_t *next_node = hash_table->heads[i];
    hash_table->heads[i] = node;
    node->next = next_node;

    hash_table->number_of_entries++;
  }

  return node;
}

//
// add edges to the word ladder graph (mostly do be done)
//

static void define_representative(hash_table_t *hash_table, hash_table_node_t *to, hash_table_node_t *from)
{
  // 'fix' the representative
  hash_table_node_t *from_representative = from->representative;
  hash_table_node_t *to_representative = to->representative;

  // reset the number of vertices
  from->representative->number_of_vertices = 0;
  for (unsigned int i = 0u; i < hash_table->hash_table_size; i++)
  {
    hash_table_node_t *hash_table_node = hash_table->heads[i];
    while (hash_table_node != NULL)
    {
      if (hash_table_node->representative == to_representative)
      {
        // set the representative of all nodes that belong to same connected component to the new one
        hash_table_node->representative = from_representative;
        from->representative->number_of_vertices++;
      }
      hash_table_node = hash_table_node->next;
    }
  }
}

static void add_edge(hash_table_t *hash_table, hash_table_node_t *from, const char *word)
{
  hash_table_node_t *to = find_word(hash_table, word, 0);
  if (to != NULL)
  {
    // connect 'from' to 'to'
    adjacency_node_t *from_adjacency_node = from->head;
    from->head = allocate_adjacency_node();
    from->head->next = from_adjacency_node;
    from->head->vertex = to;

    // connect 'to' to 'from'
    adjacency_node_t *to_adjacency_node = to->head;
    to->head = allocate_adjacency_node();
    to->head->next = to_adjacency_node;
    to->head->vertex = from;

    // count up the number of edges in all the graphs
    hash_table->number_of_edges++;

    // representative
    define_representative(hash_table, to, from);
  }
}

//
// generates a list of similar words and calls the function add_edge for each one (done)
//
// man utf8 for details on the uft8 encoding
//

static void break_utf8_string(const char *word, int *individual_characters)
{
  int byte0, byte1;

  while (*word != '\0')
  {
    byte0 = (int)(*(word++)) & 0xFF;
    if (byte0 < 0x80)
      *(individual_characters++) = byte0; // plain ASCII character
    else
    {
      byte1 = (int)(*(word++)) & 0xFF;
      if ((byte0 & 0b11100000) != 0b11000000 || (byte1 & 0b11000000) != 0b10000000)
      {
        fprintf(stderr, "break_utf8_string: unexpected UFT-8 character\n");
        exit(1);
      }
      *(individual_characters++) = ((byte0 & 0b00011111) << 6) | (byte1 & 0b00111111); // utf8 -> unicode
    }
  }
  *individual_characters = 0; // mark the end!
}

static void make_utf8_string(const int *individual_characters, char word[_max_word_size_])
{
  int code;

  while (*individual_characters != 0)
  {
    code = *(individual_characters++);
    if (code < 0x80)
      *(word++) = (char)code;
    else if (code < (1 << 11))
    { // unicode -> utf8
      *(word++) = 0b11000000 | (code >> 6);
      *(word++) = 0b10000000 | (code & 0b00111111);
    }
    else
    {
      fprintf(stderr, "make_utf8_string: unexpected UFT-8 character\n");
      exit(1);
    }
  }
  *word = '\0'; // mark the end
}

static void similar_words(hash_table_t *hash_table, hash_table_node_t *from)
{
  static const int valid_characters[] =
      {                                                                                          // unicode!
       0x2D,                                                                                     // -
       0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D,             // A B C D E F G H I J K L M
       0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,             // N O P Q R S T U V W X Y Z
       0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D,             // a b c d e f g h i j k l m
       0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,             // n o p q r s t u v w x y z
       0xC1, 0xC2, 0xC9, 0xCD, 0xD3, 0xDA,                                                       // Á Â É Í Ó Ú
       0xE0, 0xE1, 0xE2, 0xE3, 0xE7, 0xE8, 0xE9, 0xEA, 0xED, 0xEE, 0xF3, 0xF4, 0xF5, 0xFA, 0xFC, // à á â ã ç è é ê í î ó ô õ ú ü
       0};
  int i, j, k, individual_characters[_max_word_size_];
  char new_word[2 * _max_word_size_];

  break_utf8_string(from->word, individual_characters);
  for (i = 0; individual_characters[i] != 0; i++)
  {
    k = individual_characters[i];
    for (j = 0; valid_characters[j] != 0; j++)
    {
      individual_characters[i] = valid_characters[j];
      make_utf8_string(individual_characters, new_word);
      // avoid duplicate cases
      if (strcmp(new_word, from->word) > 0)
        add_edge(hash_table, from, new_word);
    }
    individual_characters[i] = k;
  }
}

//
// breadth-first search (to be done)
//
// returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
//

static int breadh_first_search(hash_table_node_t *origin, hash_table_node_t *goal, bool diameter_calculation)
{
  unsigned int number_of_used_vertices = 1;
  hash_table_node_t *previous = goal->previous;

  // follow the 'previous pointer' until the beginning to count (and print) the words
  if (!diameter_calculation) printf("path from %s to %s\n  [ 0] %s\n", origin->word, goal->word, goal->word);
  while (previous != NULL)
  {
    if (!diameter_calculation) printf("  [%2d] %s\n", number_of_used_vertices, previous->word);
    number_of_used_vertices++;
    previous = previous->previous;
  }

  return number_of_used_vertices;
}

//
// list all vertices belonging to a connected component (complete this)
//

static void list_connected_component(hash_table_t *hash_table, const char *word)
{
  hash_table_node_t *node = find_word(hash_table, word, 0);
  if (node == NULL) return; // word might not exist

  // loop over the hash_table
  for (unsigned int i = 0u; i < hash_table->hash_table_size; i++)
  {
    hash_table_node_t *hash_table_node = hash_table->heads[i];

    // if the representative of the word is the same as the representative of the current hash_table node, they belong to the same connected component
    while (hash_table_node != NULL)
    {
      if (hash_table_node->representative == node->representative)
        printf("  %s\n", hash_table_node->word);

      hash_table_node = hash_table_node->next;
    }
  }
}

//
// compute the diameter of a connected component (optional)
//

static int largest_diameter = -1;
static void path_finder(hash_table_t *hash_table, const char *from_word, const char *to_word, bool diameter_calculation);

static int connected_component_diameter(hash_table_t *hash_table, hash_table_node_t *node)
{
  if (node == NULL) return -1; // invalid node

  unsigned int diameter = 0u;
  unsigned int j = 0u;
  unsigned int i = 0u;

  // amount of vertices that the connected component has
  unsigned int number_of_nodes = node->representative->number_of_vertices;
  hash_table_node_t **connected_component_nodes = (hash_table_node_t **)malloc(sizeof(hash_table_node_t *) * number_of_nodes);

  // fill in the cconnected_component_nodes array
  for (; i < hash_table->hash_table_size; i++)
  {
    hash_table_node_t *hash_table_node = hash_table->heads[i];

    while (hash_table_node != NULL)
    {
      if (hash_table_node->representative == node->representative) {
        connected_component_nodes[j] = hash_table_node;
        j++;
      }

      hash_table_node = hash_table_node->next;
    }
  }

  // loop over all of the words in the connected component to find the largest of the smallest paths
  for (i = 0u, j = 0u; i < number_of_nodes; i++)
  {
    // no need to check for duplicate pairs
    for (j = i + 1; j < number_of_nodes; j++)
    {
      path_finder(hash_table, connected_component_nodes[i]->word, connected_component_nodes[j]->word, true);
    }
  }
  printf("\n");

  // clean up
  free(connected_component_nodes);
  largest_diameter = -1;

  return diameter;
}

//
// find the shortest path from a given word to another given word (to be done)
//

static void path_finder(hash_table_t *hash_table, const char *from_word, const char *to_word, bool diameter_calculation)
{
  // change the 'from' and 'to' so that, when printing the path we get it in the correct order
  hash_table_node_t *to = find_word(hash_table, from_word, 0);
  hash_table_node_t *from = find_word(hash_table, to_word, 0);

  if (to == NULL || from == NULL) return; // one or both of the words might not exist
  if (to->representative != from->representative) return; // make sure both words belong to same connected componen

  // allocate memory for the queue
  hash_table_node_t **queue = (hash_table_node_t **)malloc((size_t)to->representative->number_of_vertices * sizeof(hash_table_node_t *));

  unsigned int r = 0;
  unsigned int w = 1;

  queue[0] = from;
  queue[0]->visited = 1;

  bool node_found = false;
  while (r != w && !node_found)
  {
    for (adjacency_node_t *adj_node = queue[r]->head; adj_node != NULL && !node_found; adj_node = adj_node->next)
    {
      if (!adj_node->vertex->visited) {
        adj_node->vertex->visited = 1;

        adj_node->vertex->previous = queue[r];
        queue[w++] = adj_node->vertex;

        if (!strcmp(adj_node->vertex->word, to->word))
          node_found = true;
      }
    }
    r++;
  }

  // print the path
  int path_size = breadh_first_search(from, to, diameter_calculation);

  if (diameter_calculation && path_size > largest_diameter)
  {
    largest_diameter = path_size;
    hash_table_node_t *previous = to->previous;

    printf("  Biggest Path[%d] -> %s ", path_size, to->word);
    // follow the 'previous pointer' until the beginning to count how many words do we need (in reverse order)
    while (previous != NULL)
    {
      printf("%s ", previous->word);
      previous = previous->previous;
    }
    printf("\r");
  }

  // reset the visited status on all of the node inside the queue
  unsigned int i = 0;
  for (hash_table_node_t *node = queue[i]; node != NULL && i < (unsigned int) to->representative->number_of_vertices; node = queue[++i]) {
    node->visited = 0;
    node->previous = NULL;
  }

  free(queue); // clean up
}

//
// some graph information (optional)
//

static void graph_info(hash_table_t *hash_table)
{
  // print the number of vertices
  printf("%d vertices\n", hash_table->number_of_entries);

  // print the number of connected components
  unsigned int number_of_connected_components = 0;
  for (unsigned int i = 0u; i < hash_table->hash_table_size; i++)
  {
    hash_table_node_t *hash_table_node = hash_table->heads[i];

    while (hash_table_node != NULL)
    {
      // if the representative of a node is itself then we have a connected component
      if (hash_table_node->representative == hash_table_node)
        number_of_connected_components++;

      hash_table_node = hash_table_node->next;
    }
  }

  printf("%d edges\n", hash_table->number_of_edges);
  printf("%d connected components\n", number_of_connected_components);
}

//
// main program
// 

int main(int argc, char **argv)
{
  char word[100], from[100], to[100];
  hash_table_t *hash_table;
  hash_table_node_t *node;
  unsigned int i;
  int command;
  FILE *fp;

  // initialize hash table
  hash_table = hash_table_create(100); // 100 is the initial hash table size
  // read words
  fp = fopen((argc < 2) ? "wordlist-four-letters.txt" : argv[1], "rb");
  if (fp == NULL)
  {
    fprintf(stderr, "main: unable to open the words file\n");
    exit(1);
  }
  while (fscanf(fp, "%99s", word) == 1)
  {
    (void)find_word(hash_table, word, 1);
  }
  fclose(fp);

  // find all similar words
  for (i = 0u; i < hash_table->hash_table_size; i++)
    for (node = hash_table->heads[i]; node != NULL; node = node->next)
      similar_words(hash_table, node);
  graph_info(hash_table);

  // ask what to do
  for (;;)
  {
    fprintf(stderr, "Your wish is my command:\n");
    fprintf(stderr, "  1 WORD       (list the connected component WORD belongs to)\n");
    fprintf(stderr, "  2 FROM TO    (list the shortest path from FROM to TO)\n");
    fprintf(stderr, "  3 WORD       (diameter - determine the biggest of the shortests paths in word graph)\n");
    fprintf(stderr, "  4            (terminate)\n");
    fprintf(stderr, "> ");
    if (scanf("%99s", word) != 1)
      break;
    command = atoi(word);
    if (command == 1)
    {
      if (scanf("%99s", word) != 1)
        break;
      list_connected_component(hash_table, word);
    }
    else if (command == 2)
    {
      if (scanf("%99s", from) != 1)
        break;
      if (scanf("%99s", to) != 1)
        break;
      path_finder(hash_table, from, to, false);
    }
    else if (command == 3) {
      if (scanf("%99s", from) != 1)
        break;
      connected_component_diameter(hash_table, find_word(hash_table, from, 0));
    }
    else if (command == 4)
      break;
  }

  // clean up
  hash_table_free(hash_table);
  return 0;
}