struct Node {
  int value;
  struct Node *next;
};

int init_node(void) {
  struct Node *node = (struct Node *)1024;
  node->value = 7;
  node->next = (struct Node *)1024;
  return node->next != 0;
}
