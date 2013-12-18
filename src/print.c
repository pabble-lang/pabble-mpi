#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <sesstype/st_node.h>
#ifdef __DEBUG__
#include <sesstype/st_node_print.h>
#endif
#include <scribble/print.h>

#include "scribble/mpi_print.h"


string_list labels = {.strings = NULL, .count = 0};
unsigned int mpi_primitive_count = 0;


void mpi_find_labels(st_node *node)
{
  int found = 0;
  if (node->type == ST_NODE_SEND || node->type == ST_NODE_RECV) {
    // If message label is not MPI_*
    if (strlen(node->interaction->msgsig.op) > 0 && strncmp(node->interaction->msgsig.op, "MPI_", 4) != 0) {
      for (int label=0; label<labels.count && !found; label++) {
        if (strcmp(labels.strings[label], node->interaction->msgsig.op) == 0) {
          found = 1;
        }
      }
      if (!found) {
        if (labels.count == 0) {
          labels.strings = (char **)malloc(sizeof(char *));
        } else {
          labels.strings = (char **)realloc(labels.strings, sizeof(char *) * (labels.count+1));
        }

        labels.strings[labels.count++] = strdup(node->interaction->msgsig.op);
      }
    }
  }

  for (int child=0; child<node->nchild; child++) {
    mpi_find_labels(node->children[child]);
  }
}


void mpi_fprint_choice(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_CHOICE);
  mpi_fprintf(stream, "%*s/* Choice \n", indent, SPACE);
  scribble_fprint_node(stream, node, indent+1);
  mpi_fprintf(stream, "%*s*/\n", indent, SPACE);


  // This is choice role check, not conditional
  mpi_fprint_msg_cond(stream, tree, node->choice->at, indent);

  indent++;

  mpi_fprintf(stream, "%*sif (0/* condition */) { // branch decision\n", indent, SPACE);

  indent++;

  // Representing all branches
  for (int child=0; child<node->nchild; child++) {

    indent++;
    mpi_fprint_node(pre_stream, stream, post_stream, tree, node->children[child], indent);
    indent--;

    if (child != node->nchild-1) {
      mpi_fprintf(stream, "%*s} else if (0/* condition */) { // branch decision\n", indent, SPACE);
    }
  }

  indent--;

  mpi_fprintf(stream, "%*s} // branch decision\n", indent, SPACE);

  indent--;

  mpi_fprintf(stream, "%*s} else {\n", indent, SPACE);

  indent++;
  mpi_fprintf(stream, "%*sMPI_Status status;\n", indent, SPACE);

  mpi_fprintf(stream, "%*sMPI_Probe(%d, MPI_ANY_TAG, MPI_COMM_WORLD, &status);\n",
      indent, SPACE,
      node->choice->at->param[0]->num);

  mpi_fprintf(stream, "%*sswitch (status.MPI_TAG) {", indent, SPACE);

  for (int child=0; child<node->nchild; child++) {
    if (node->children[child]->type == ST_NODE_ROOT) {

      // choice at X { U from X; U to Y; /* Multiple lines */ }
      indent++;
      mpi_fprintf(stream, "%*scase %s:\n", indent, SPACE, node->children[child]->children[0]->interaction->msgsig.op);
      mpi_fprint_node(pre_stream, stream, post_stream, tree, node->children[child], indent);
      mpi_fprintf(stream, "%*sbreak;\n", indent, SPACE);
      indent--;

    } else if (node->children[child]->type == ST_NODE_SEND || node->children[child]->type == ST_NODE_RECV) {

      // choice at X { U from X; }
      indent++;
      mpi_fprintf(stream, "%*scase %s:\n", indent, SPACE, node->children[child]->interaction->msgsig.op);
      mpi_fprint_node(pre_stream, stream, post_stream, tree, node->children[child], indent);
      mpi_fprintf(stream, "%*sbreak;\n", indent, SPACE);
      indent--;

    }
  }

  mpi_fprintf(stream, "%*s} // switch\n", indent, SPACE);

  indent--;

  mpi_fprintf(stream, "%*s} // rank selection\n", indent, SPACE);
}


void mpi_fprint_node(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL);
#ifdef __DEBUG__
  st_node_fprint(stderr, node, 0);
  fprintf(stderr, "\n");
#endif

  switch (node->type) {
    case ST_NODE_ROOT:      mpi_fprint_root(pre_stream, stream, post_stream, tree, node, indent);      break;
    case ST_NODE_SEND:      mpi_fprint_send(pre_stream, stream, post_stream, tree, node, indent);      break;
    case ST_NODE_RECV:      mpi_fprint_recv(pre_stream, stream, post_stream, tree, node, indent);      break;
    case ST_NODE_CHOICE:    mpi_fprint_choice(pre_stream, stream, post_stream, tree, node, indent);    break;
    case ST_NODE_PARALLEL:  mpi_fprint_parallel(pre_stream, stream, post_stream, tree, node, indent);  break;
    case ST_NODE_RECUR:     mpi_fprint_recur(pre_stream, stream, post_stream, tree, node, indent);     break;
    case ST_NODE_CONTINUE:  mpi_fprint_continue(pre_stream, stream, post_stream, tree, node, indent);  break;
    case ST_NODE_FOR:       mpi_fprint_for(pre_stream, stream, post_stream, tree, node, indent);       break;
    case ST_NODE_ALLREDUCE: mpi_fprint_allreduce(pre_stream, stream, post_stream, tree, node, indent); break;
    case ST_NODE_IFBLK:     mpi_fprint_ifblk(pre_stream, stream, post_stream, tree, node, indent);     break;
    case ST_NODE_ONEOF:     mpi_fprint_oneof(pre_stream, stream, post_stream, tree, node, indent);     break;

    case ST_NODE_SENDRECV:  assert(0/* Global protocol not possible */); break;
    default:                fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
  }
}


void mpi_fprint_parallel(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_PARALLEL);

  mpi_fprintf(stream, "%*s#pragma omp sections\n", indent, SPACE);

  mpi_fprintf(stream, "%*s{\n", indent, SPACE);

  indent++;

  for (int child=0; child<node->nchild; ++child) {
    mpi_fprintf(stream, "%*s#pragma omp section\n", indent, SPACE);

    mpi_fprintf(stream, "%*s{\n", indent, SPACE);
    mpi_fprint_node(pre_stream, stream, post_stream, tree, node->children[child], indent);
    mpi_fprintf(stream, "%*s}\n", indent, SPACE);
  }

  indent--;
  mpi_fprintf(stream, "%*s}\n", indent, SPACE);
}


void mpi_fprint_recur(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_RECUR);

  mpi_fprintf(stream, "%*s/* Recur\n", indent, SPACE);
  scribble_fprint_node(stream, node, indent+1);
  mpi_fprintf(stream, "%*s*/\n", indent, SPACE);

  mpi_fprintf(stream, "#pragma pabble loop\n");
  mpi_fprintf(stream, "%*swhile (1/* CHANGEME */) {\n", indent, SPACE);

  mpi_fprint_children(pre_stream, stream, post_stream, tree, node, indent);

  indent++;
  mpi_fprintf(stream, "%*sbreak;\n", indent, SPACE);

  indent--;
  mpi_fprintf(stream, "%*s}\n", indent, SPACE);
}


void mpi_fprint_continue(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_CONTINUE);

  mpi_fprintf(stream, "%*scontinue;\n", indent, SPACE);
}

/**
 * \brief For loop.
 *
 * @param[out] pre_stream  Stream in set up section.
 * @param[out] stream      Stream in body section.
 * @param[out] post_stream Stream in tear down section.
 * @param[in]  tree        Session Tree to print.
 * @param[in]  node        Session Type node to print.
 * @param[in]  indent      Indention amount.
 */
void mpi_fprint_for(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_FOR);

  mpi_fprintf(stream, "%*sfor (int %s = ", indent, SPACE, node->forloop->range->bindvar);
  mpi_fprint_expr(stream, node->forloop->range->from);
  mpi_fprintf(stream, "; %s <= ", node->forloop->range->bindvar);
  mpi_fprint_expr(stream, node->forloop->range->to);
  mpi_fprintf(stream, "; %s++) {\n", node->forloop->range->bindvar);
  indent++;
  if (node->forloop->except != NULL) {
    mpi_fprintf(stream, "%*sif (%s!=%s) continue; // SKIP\n", indent, SPACE, node->forloop->range->bindvar, node->forloop->except);
  }
  for (int child=0; child<node->nchild; ++child) {
    mpi_fprint_node(pre_stream, stream, post_stream, tree, node->children[child], indent);
  }
  indent--;
  mpi_fprintf(stream, "%*s}\n", indent, SPACE);
}


void mpi_fprint_root(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree* tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_ROOT);

  mpi_fprintf(stream, "\n%*s{ /* --- BEGIN Protocol body --- */\n\n", indent, SPACE);

  mpi_fprint_children(pre_stream, stream, post_stream, tree, node, indent);

  mpi_fprintf(stream, "\n%*s} /* --- END Protocol body --- */ \n\n", indent, SPACE);
}


/**
 * \brief Entry point to MPI code generator.
 *
 *
 */
void mpi_fprint(FILE *stream, st_tree *tree)
{
  char body_tmpfile[31];
  strncpy(body_tmpfile, "/tmp/scribble_mpi.body.c-XXXXXX", 31);
  FILE *body_fp = fdopen(mkstemp(body_tmpfile), "w+");
  unlink(body_tmpfile);

  char tail_tmpfile[31];
  strncpy(tail_tmpfile, "/tmp/scribble_mpi.tail.c-XXXXXX", 31);
  FILE *tail_fp = fdopen(mkstemp(tail_tmpfile), "w+");
  unlink(tail_tmpfile);

#ifdef __DEBUG__
  fprintf(stderr, "DEBUG/%s:%d headers..\n", __FUNCTION__, __LINE__);
#endif

  /**
   * Includes.
   */
  mpi_fprintf(stream, "/*** Generated by scribble-to-mpi generator ***/\n");
  mpi_fprintf(stream, "#include <stdio.h>\n");
  mpi_fprintf(stream, "#include <stdlib.h>\n");
  mpi_fprintf(stream, "#include <string.h>\n");
  mpi_fprintf(stream, "#include <mpi.h>\n\n");

#ifdef __DEBUG__
  fprintf(stderr, "DEBUG/%s:%d auto generate label/tags..\n", __FUNCTION__, __LINE__);
#endif

  /**
   * Define MPI tags / Pabble message labels as constants.
   * (1) Walk whole st_tree->root to find unique labels
   * (2) Write a #define for each of them (use arary index)
   */
  labels.count = 0;
  mpi_find_labels(tree->root);
  mpi_fprintf(stream, "/*** MPI tags / Scribble labels ***/\n");
  if (labels.count > 0) {
    for (int label=0; label<labels.count; label++) {
      mpi_fprintf(stream, "#define %s %d\n", labels.strings[label], label);
    }
  }

#ifdef __DEBUG__
  fprintf(stderr, "DEBUG/%s:%d rank conversion macros..\n", __FUNCTION__, __LINE__);
#endif

  /**
   * Define macros to convert multi-dimension roles to MPI ranks.
   * (1) Go through roles in st_tree
   * (2) Role definitions are only defined by numeric constants or declared constants
   *     => numeric constants (eg. 3) = use value
   *     => declared constants (eg. N) = find value (if defined), use as is otherwise
   * (3) Derive and bind values of constants from SIZE_VARIABLE
   */
  mpi_fprintf(stream, "\n/*** Scribble parameterised role to MPI rank conversion macros ***/\n");
  int mpi_nroles = 0;
  for (int role=0; role<tree->info->nrole; role++) {
    switch (tree->info->roles[role]->dimen) {
      case 0: // Ordinary role, use defined rank value
#ifdef __DEBUG__
  fprintf(stderr, "DEBUG/%s:%d rank conversion macro for %s..\n", __FUNCTION__, __LINE__, tree->info->roles[role]->name);
#endif
        mpi_fprintf(stream, "#define %s_RANK %d\n\n", tree->info->roles[role]->name, mpi_nroles);
        mpi_nroles++;
        break;
      case 1:
      case 2:
#ifdef __DEBUG__
  fprintf(stderr, "DEBUG/%s:%d rank conversion macro for %s..\n", __FUNCTION__, __LINE__, tree->info->roles[role]->name);
#endif
        assert(tree->info->roles[role]->dimen <= 2 /* Won't work with > 2 dimen for now */);

        mpi_fprintf(stream, "#define %s_RANK(", tree->info->roles[role]->name);
        for (int dimen=0; dimen<tree->info->roles[role]->dimen; dimen++) {
          mpi_fprintf(stream, "%s%c", dimen==0?"":",", 'x'+dimen);
        }

        mpi_fprintf(stream, ") (%d", mpi_nroles); // Starting parenthesis + Rank Offset

        for (int dimen=0; dimen<tree->info->roles[role]->dimen; dimen++) {
          assert(tree->info->roles[role]->param[dimen]->type == ST_EXPR_TYPE_RNG);

          if (dimen > 0) { // Prepend 'row-offset', *(x_max-x_min)
            mpi_fprintf(stream, "*((");
            mpi_fprint_const_or_var(stream, tree, tree->info->roles[role]->param[dimen-1]->rng->to); // x_max
            mpi_fprintf(stream, ")-(");
            mpi_fprint_const_or_var(stream, tree, tree->info->roles[role]->param[dimen-1]->rng->from); // x_min
            mpi_fprintf(stream, "))");
          }

          // For dimen > 2 we need to add (ndimen - current dimen) opening brackets

          mpi_fprintf(stream, "+((%c)-(", 'x'+dimen); // y
          mpi_fprint_const_or_var(stream, tree, tree->info->roles[role]->param[dimen]->rng->from); // y_min
          mpi_fprintf(stream, "))");
        }
        mpi_fprintf(stream, ")"); // Ending parenthesis
        break;
    }
  }

#ifdef __DEBUG__
  fprintf(stderr, "DEBUG/%s:%d generate main/declarations..\n", __FUNCTION__, __LINE__);
#endif

  mpi_fprintf(stream, "\n\nint main(int argc, char *argv[])\n");
  mpi_fprintf(stream, "{\n");
  mpi_fprintf(stream, "  int %s, %s; // MPI variables\n", RANK_VARIABLE, SIZE_VARIABLE);
  mpi_fprintf(stream, "  MPI_Group world_grp;\n");
  mpi_fprintf(stream, "  MPI_Init(&argc, &argv);\n");
  mpi_fprintf(stream, "  MPI_Comm_rank(MPI_COMM_WORLD, &%s);\n", RANK_VARIABLE);
  mpi_fprintf(stream, "  MPI_Comm_size(MPI_COMM_WORLD, &%s);\n", SIZE_VARIABLE);

  mpi_fprintf(stream, "\n  /** Begin of protocol %s **/\n\n", tree->info->name);

  if (ST_TYPE_LOCAL == tree->info->type) {

    for (int constant=0; constant<tree->info->nconst; constant++) {
      switch (tree->info->consts[constant]->type) {
        case ST_CONST_VALUE:
          mpi_fprintf(stream, "  const int %s = %u;\n",
              tree->info->consts[constant]->name,
              tree->info->consts[constant]->value);
          break;
        case ST_CONST_BOUND:
          mpi_fprintf(stream, "  const int %s = %u/* CHANGEME(%u..%u) */;\n",
              tree->info->consts[constant]->bounds.lbound,
              tree->info->consts[constant]->bounds.lbound,
              tree->info->consts[constant]->bounds.ubound);
          break;
        default:
          fprintf(stderr, "Undefined constant type consts[%d] (%d)\n",
              constant,
              tree->info->consts[constant]->type);
      }
    }

    // Group declaration
    mpi_fprintf(stream, "  MPI_Comm_group(MPI_COMM_WORLD, &world_grp);\n");
    for (int group=0; group<tree->info->ngroup; group++) {
      mpi_fprintf(stream, "  MPI_Group %s;\n", tree->info->groups[group]->name);
      mpi_fprintf(stream, "  MPI_Comm %s_comm;\n", tree->info->groups[group]->name);
      // Define group members
      mpi_fprintf(stream, "  int *grp_%s = NULL;\n");
      mpi_fprintf(stream, "  int ngrp_%s = 0;\n", tree->info->groups[group]->name);
      for (int role=0; role<tree->info->groups[group]->nmemb; role++) {
        for (int param=0; param<tree->info->groups[group]->membs[role]->dimen; param++) {
          if (tree->info->groups[group]->membs[role]->param[param]->type == ST_EXPR_TYPE_RNG) {
            mpi_fprintf(stream, "  for (int param%d=",param);
            mpi_fprint_const_or_var(stream, tree, tree->info->groups[group]->membs[role]->param[param]->rng->from);
            mpi_fprintf(stream, "; param%d<", param);
            mpi_fprint_const_or_var(stream, tree, tree->info->groups[group]->membs[role]->param[param]->rng->to);
            mpi_fprintf(stream, "; param%d++) {\n", param);
          } else if (tree->info->groups[group]->membs[role]->param[param]->type == ST_EXPR_TYPE_VAR) {
            mpi_fprintf(stream, "  int param%d = %s\n", param, tree->info->groups[group]->membs[role]->param[param]->var); // Rank[var]
          } else if (tree->info->groups[group]->membs[role]->param[param]->type == ST_EXPR_TYPE_CONST) {
            mpi_fprintf(stream, "  int param%d = %d\n", param, tree->info->groups[group]->membs[role]->param[param]->num); // Rank[num]
          }
        }
        // The group definition
        mpi_fprintf(stream, "    grp_%s = realloc(grp_%s, sizeof(int)*(ngrp_%s+1));\n",
            tree->info->groups[group]->name,
            tree->info->groups[group]->name,
            tree->info->groups[group]->name);
        mpi_fprintf(stream, "    grp_%s[ngrp_%s] = %s_RANK(", tree->info->groups[group]->name, tree->info->groups[group]->name, tree->info->groups[group]->name);
        for (int param=0; param<tree->info->groups[group]->membs[role]->dimen; param++) {
          if (param!=0) mpi_fprintf(stream, ",");
          mpi_fprintf(stream, "param%d", param);
        }
        mpi_fprintf(stream, "); ngrp_%s++;\n", tree->info->groups[group]->name);

        for (int param=0; param<tree->info->groups[group]->membs[role]->dimen; param++) {
          if (tree->info->groups[group]->membs[role]->param[param]->type == ST_EXPR_TYPE_RNG) {
            mpi_fprintf(stream, " }\n", param);
          }
        }
        /*
           if (tree->info->groups[group]->membs[role]->param[0]->type == ST_EXPR_TYPE_CONST) {
           mpi_fprintf(stream, "%d", tree->info->groups[group]->membs[role]->param[0]->num);
           } else if (tree->info->groups[group]->membs[role]->param[0]->type == ST_EXPR_TYPE_RNG) {
           assert(tree->info->groups[group]->membs[role]->param[0]->rng->from->type == ST_EXPR_TYPE_CONST);
           assert(tree->info->groups[group]->membs[role]->param[0]->rng->to->type == ST_EXPR_TYPE_CONST);
           for (int index=tree->info->groups[group]->membs[role]->param[0]->rng->from->num;
           index<tree->info->groups[group]->membs[role]->param[0]->rng->to->num;
           index++) {
           mpi_fprintf(stream, "%d", index);
           }
           } else {
           assert(0); //Group defined with role indices other than constant and range
           }
         */
      }
      mpi_fprintf(stream, "  };\n");
      mpi_fprintf(stream, "  MPI_Group_incl(world_grp, %d, &%s);\n",
          tree->info->groups[group]->nmemb,
          tree->info->groups[group]->name);
      mpi_fprintf(stream, "  MPI_Comm_create(MPI_COMM_WORLD, %s, %s_comm);\n",
          tree->info->groups[group]->name,
          tree->info->groups[group]->name);
    }

  } else { // Not Local protocol

    fprintf(stderr, "Protocol type %d not supported for MPI generation\n", tree->info->type);

  }

  //
  // Generate body of protocol
  //
  int indent = 1;
  mpi_fprint_node(stream, body_fp, tail_fp, tree, tree->root, indent);

  rewind(body_fp);
  rewind(tail_fp);

  char buffer[256];
  int bufsize = 0;
  memset(buffer, 0, 256);
  while (!feof(body_fp)) {
    bufsize = fread(buffer, 1, 256, body_fp);
    buffer[bufsize] = 0;
    fprintf(stream, "%s", buffer);
  }

  memset(buffer, 0, 256);
  while (!feof(tail_fp)) {
    bufsize = fread(buffer, 1, 256, tail_fp);
    buffer[bufsize] = 0;
    fprintf(stream, "%s", buffer);
  }

  mpi_fprintf(stream, "\n  /** End of protocol %s **/\n\n", tree->info->name);

  mpi_fprintf(stream, "  MPI_Finalize();\n");
  // Cleanup group construction
  for (int group=0; group<tree->info->ngroup; group++) {
    mpi_fprintf(stream, "  free(%s);\n", tree->info->groups[group]->name);
  }
  mpi_fprintf(stream, "  return EXIT_SUCCESS;\n");
  mpi_fprintf(stream, "}\n");

  fclose(body_fp);
  fclose(tail_fp);
}


/**
 *
 */
void mpi_fprintf(FILE *stream, const char *format, ...)
{
  char orig_str[2560];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(orig_str, 2560, format, argptr);
  va_end(argptr);

  fprintf(stream, "%s", orig_str);
}


/**
 *
 */
void mpi_print(FILE *stream, st_tree *local_tree)
{
  mpi_fprint(stream, local_tree);
}
