/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <stdio.h>
#include <stdlib.h>


#define LEVELS 10
#define HIGHEST_DIVISOR (1 << LEVELS)


typedef struct _Node Node;

struct _Node
{
  int divisor, dividend;
  Node *l, *r;
};


void subdivide(Node *p);

int num_printed = 0;


int main(int argc, char *argv[])
{
  Node *root, *n;
  unsigned int i, j, level, pass;

  /* Build the tree */
  
  root = malloc(sizeof(Node));
  root->dividend = 1;
  root->divisor = 2;
  subdivide(root);
  
  /* Traverse the tree, printing values */
  
  printf("int divide_seq[] = \n{\n");
  
  for (level = 0; level < LEVELS; level++)
  {
    printf("  /* Dividends for divisor of %d */\n\n  ", 1 << (level + 1));
    num_printed = 0;

    for (pass = 0; pass < (1 << level); pass++)
    {
      n = root;
      
      for (i = 0; i < level; i++)
      {
        if ((pass >> i) & 1) n = n->r;
        else n = n->l;
      }
      
      if (num_printed == 10)
      {
        printf(",\n  ");
        num_printed = 0;
      }

      printf("%s%3d", num_printed ? ", " : "", n->dividend);
      fflush(stdout);
      
      num_printed++;
    }
    
    printf("%s\n", level + 1 < LEVELS ? ",\n" : "");
  }
  
  printf("};\n");
  
  return(0);
}


void subdivide(Node *p)
{
  p->l = malloc(sizeof(Node));
  p->r = malloc(sizeof(Node));

  p->l->dividend = (p->dividend * 2) - 1;
  p->r->dividend = (p->dividend * 2) + 1;

  p->r->divisor = p->l->divisor = p->divisor * 2;
  
  if (p->l->divisor >= HIGHEST_DIVISOR)
  {
    p->l->l = p->l->r = NULL;
    p->r->l = p->r->r = NULL;
    return;
  }

  subdivide(p->l);
  subdivide(p->r);
}
