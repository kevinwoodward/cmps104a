// Jose Sepulveda, joasepul@ucsc.edu
// Kevin Woodward, keawoodw@ucsc.edu

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>

#include "symtable.h"


/*
Here we need code to do a traversal
through the astree and set the different attributes
this is called after the parser.y returns the tree
to main, then we call this with the tree and outfile
  


*/
