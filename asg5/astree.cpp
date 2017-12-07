// Jose Sepulveda, joasepul@ucsc.edu
// Kevin Woodward, keawoodw@ucsc.edu

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "string_set.h"
#include "lyutils.h"

astree::astree (int symbol_, const location& lloc_, const char* info) {
   symbol = symbol_;
   lloc = lloc_;
   lexinfo = string_set::intern (info);
   // vector defaults to empty -- no children
}

astree::~astree() {
   while (not children.empty()) {
      astree* child = children.back();
      children.pop_back();
      delete child;
   }
   if (yydebug) {
      fprintf (stderr, "Deleting astree (");
      astree::dump (stderr, this);
      fprintf (stderr, ")\n");
   }
}

char* get_attributes (attr_bitset attributes) {
    string str = "";

    if (attributes.test (ATTR_void))        str += "void ";
    if (attributes.test (ATTR_int))         str += "int ";
    if (attributes.test (ATTR_null))        str += "null ";
    if (attributes.test (ATTR_string))      str += "string ";
    if (attributes.test (ATTR_struct))      str += "struct ";
    if (attributes.test (ATTR_array))       str += "array ";
    if (attributes.test (ATTR_function))    str += "function ";
    if (attributes.test (ATTR_variable))    str += "variable ";
    if (attributes.test (ATTR_typeid))      str += "typeid ";
    if (attributes.test (ATTR_param))       str += "param ";
    if (attributes.test (ATTR_lval))        str += "lval ";
    if (attributes.test (ATTR_const))       str += "const ";
    if (attributes.test (ATTR_vreg))        str += "vreg ";
    if (attributes.test (ATTR_vaddr))       str += "vaddr ";

    return strdup(str.c_str());
}

astree* astree::adopt (astree* child1, astree* child2) {
   //Note to self: child2 may leak memory
   if (child1 != nullptr)
   {
       children.push_back (child1);
   }
   if (child2 != nullptr)
   {
       children.push_back (child2);
   }
   return this;
}

astree* astree::adopt_ifelse (astree* predicate,
                              astree* then_stmt,
                              astree* else_stmt) {

    if (predicate != nullptr)
    {
       children.push_back (predicate);
    }
   if (then_stmt != nullptr)
   {
       children.push_back (then_stmt);
   }
   if (else_stmt != nullptr)
   {
       children.push_back (else_stmt);
   }
   return this;
}

astree* astree::adopt_sym (int symbol_,
                           astree* child1,
                           astree* child2){
   symbol = symbol_;
   return adopt (child1, child2);
}

astree* astree::adopt_with_sym (int symbol_,
                                astree* child1,
                                astree* child2){
   child1->symbol = symbol_;
   return adopt (child1, child2);
}

astree* astree::synthesize_root(astree* new_root){
    new_root = new astree(TOK_ROOT, {0,0,0}, "");
    return new_root;
}

astree* astree::synthesize_function (astree* identdecl,
                                    astree* func_params,
                                    astree* block){
    astree* func_ast;
    func_ast = new astree (TOK_PROTOTYPE, lexer::lloc, "");
    func_ast->adopt(identdecl);
    func_params->symbol = TOK_PARAMLIST;
    func_ast->adopt(func_params);

    if(block->symbol == ';'){
        destroy(block);
    }
    else
    {
        func_ast->symbol = TOK_FUNCTION;
        func_ast->adopt(block);
    }
    return func_ast;

}



void astree::dump_node (FILE* outfile) {
   fprintf (outfile, "%p->{%s %zd.%zd.%zd \"%s\":",
            this, parser::get_tname (symbol),
            lloc.filenr, lloc.linenr, lloc.offset,
            lexinfo->c_str());
   for (size_t child = 0; child < children.size(); ++child) {
      fprintf (outfile, " %p", children.at(child));
   }
}

void astree::dump_tree (FILE* outfile, int depth) {
   fprintf (outfile, "%*s", depth * 3, "");
   dump_node (outfile);
   fprintf (outfile, "\n");
   for (astree* child: children) child->dump_tree (outfile, depth + 1);
   fflush (nullptr);
}

void astree::dump (FILE* outfile, astree* tree) {
   if (tree == nullptr) fprintf (outfile, "nullptr");
                   else tree->dump_node (outfile);
}

void astree::print (FILE* outfile, astree* tree, int depth) {
   //fprintf (outfile, "; %*s", depth * 3, "");
   for(int i=0; i<depth; i++)
   {
       fprintf(outfile, "|  ");
   }
   if (tree == NULL)
   {
       exec::exit_status = 1;
       return;
   }
   char *tname = const_cast<char*>(parser::get_tname (tree->symbol));
   if(strstr (tname, "TOK_") == tname) tname += 4;
   fprintf (outfile, "%s \"%s\" %zd.%zd.%zd %s\n",
            tname, tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset,
            get_attributes(tree->attributes));

   for (astree* child: tree->children) {
      astree::print (outfile, child, depth + 1);
   }
}

void destroy (astree* tree1, astree* tree2, astree* tree3) {
   if (tree1 != nullptr) delete tree1;
   if (tree2 != nullptr) delete tree2;
   if (tree3 != nullptr) delete tree3;
}

void errllocprintf (const location& lloc, const char* format,
                    const char* arg) {
   static char buffer[0x1000];
   assert (sizeof buffer > strlen (format) + strlen (arg));
   snprintf (buffer, sizeof buffer, format, arg);
   errprintf ("%s:%zd.%zd: %s",
              (*lexer::filename (lloc.filenr)).c_str(),
              lloc.linenr,
              lloc.offset,
              buffer);
}
