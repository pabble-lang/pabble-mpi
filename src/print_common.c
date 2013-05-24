#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sesstype/st_node.h>
#include <scribble/print.h>

#include "scribble/mpi_print.h"


/**
 * Print a constant or variable expression.
 */
void mpi_fprint_const_or_var(FILE *stream, st_tree *tree, st_expr *expr)
{
  int printed = 0;
  switch (expr->type) {
    case ST_EXPR_TYPE_CONST:
      mpi_fprintf(stream, "%d", expr->num);
      break;

    case ST_EXPR_TYPE_VAR:
      for (int constant=0; constant<tree->info->nconst; constant++) {
        // If matches constant name and has defined value
        if (strcmp(tree->info->consts[constant]->name, expr->var) == 0 && tree->info->consts[constant]->type == ST_CONST_VALUE) {
          mpi_fprintf(stream, "%u", tree->info->consts[constant]->value);
          printed = 1;
        }
      }
      if (!printed) mpi_fprintf(stream, "%s", expr->var);
      break;

    default:
      fprintf(stderr, "%s:%s:%d Expression is neither CONST or VAR (%d)\n", __FUNCTION__, __FILE__, __LINE__, expr->type);
      assert(0/*Expression is neither CONST or VAR*/);
      break;
  }
}


/**
 * Print an expression.
 */
void mpi_fprint_expr(FILE *stream, st_expr *expr)
{
  assert(expr != NULL);

  st_expr_eval(expr);

  switch (expr->type) {
    case ST_EXPR_TYPE_ADD:
      mpi_fprintf(stream, "(");
      mpi_fprint_expr(stream, expr->bin->left);
      mpi_fprintf(stream, "+");
      mpi_fprint_expr(stream, expr->bin->right);
      mpi_fprintf(stream, ")");
      break;
    case ST_EXPR_TYPE_SUB:
      mpi_fprintf(stream, "(");
      mpi_fprint_expr(stream, expr->bin->left);
      mpi_fprintf(stream, "-");
      mpi_fprint_expr(stream, expr->bin->right);
      mpi_fprintf(stream, ")");
      break;
    case ST_EXPR_TYPE_MUL:
      mpi_fprintf(stream, "(");
      mpi_fprint_expr(stream, expr->bin->left);
      mpi_fprintf(stream, "*");
      mpi_fprint_expr(stream, expr->bin->right);
      mpi_fprintf(stream, ")");
      break;
    case ST_EXPR_TYPE_MOD:
      mpi_fprintf(stream, "(");
      mpi_fprint_expr(stream, expr->bin->left);
      mpi_fprintf(stream, "%%");
      mpi_fprint_expr(stream, expr->bin->right);
      mpi_fprintf(stream, ")");
      break;
    case ST_EXPR_TYPE_DIV:
      mpi_fprintf(stream, "(");
      mpi_fprint_expr(stream, expr->bin->left);
      mpi_fprintf(stream, "/");
      mpi_fprint_expr(stream, expr->bin->right);
      mpi_fprintf(stream, ")");
      break;
    case ST_EXPR_TYPE_SHL:
      mpi_fprintf(stream, "(");
      mpi_fprint_expr(stream, expr->bin->left);
      mpi_fprintf(stream, "<<");
      mpi_fprint_expr(stream, expr->bin->right);
      mpi_fprintf(stream, ")");
      break;
    case ST_EXPR_TYPE_SHR:
      mpi_fprintf(stream, "(");
      mpi_fprint_expr(stream, expr->bin->left);
      mpi_fprintf(stream, ">>");
      mpi_fprint_expr(stream, expr->bin->right);
      mpi_fprintf(stream, ")");
      break;
    case ST_EXPR_TYPE_CONST:
      mpi_fprintf(stream, "%d", expr->num);
      break;
    case ST_EXPR_TYPE_VAR:
      mpi_fprintf(stream, "%s", expr->var);
      break;
    case ST_EXPR_TYPE_RNG:
      assert(0 /* Only support printing pure expression */);
      break;
    default:
      fprintf(stderr, "%s:%d %s Unknown expr type: %d\n", __FILE__, __LINE__, __FUNCTION__, expr->type);
  };
}


void mpi_fprint_datatype(FILE *stream, st_node_msgsig_t msgsig)
{
  if (strcmp(msgsig.payload, "int") == 0) {
    mpi_fprintf(stream, "MPI_INT");
  } else if (strcmp(msgsig.payload, "float") == 0) {
    mpi_fprintf(stream, "MPI_FLOAT");
  } else if (strcmp(msgsig.payload, "double") == 0) {
    mpi_fprintf(stream, "MPI_DOUBLE");
  } else if (strcmp(msgsig.payload, "char") == 0) {
    mpi_fprintf(stream, "MPI_CHAR");
  } else {
    mpi_fprintf(stream, "MPI_BYTE/*undefined type*/");
  }
}


void mpi_fprint_rank(FILE *stream, st_expr *param, const char *replace, const char *with)
{
  switch (param->type) {
    case ST_EXPR_TYPE_VAR: // Constant
      if (replace != NULL && strcmp(replace, param->var) == 0) {
        mpi_fprintf(stream, "%s", with);
      } else {
        mpi_fprintf(stream, "%s", param->var);
      }
      break;

    case ST_EXPR_TYPE_CONST:
      mpi_fprintf(stream, "%d", param->num);
      break;

    case ST_EXPR_TYPE_ADD:
      mpi_fprintf(stream, "(");
      mpi_fprint_rank(stream, param->bin->left, replace, with);
      mpi_fprintf(stream, "+");
      mpi_fprint_rank(stream, param->bin->right, replace, with);
      mpi_fprintf(stream, ")");
      break;

    case ST_EXPR_TYPE_SUB:
      mpi_fprintf(stream, "(");
      mpi_fprint_rank(stream, param->bin->left, replace, with);
      mpi_fprintf(stream, "-");
      mpi_fprint_rank(stream, param->bin->right, replace, with);
      mpi_fprintf(stream, ")");
      break;

    case ST_EXPR_TYPE_MUL:
      mpi_fprintf(stream, "(");
      mpi_fprint_rank(stream, param->bin->left, replace, with);
      mpi_fprintf(stream, "*");
      mpi_fprint_rank(stream, param->bin->right, replace, with);
      mpi_fprintf(stream, ")");
      break;

    case ST_EXPR_TYPE_DIV:
      mpi_fprintf(stream, "(");
      mpi_fprint_rank(stream, param->bin->left, replace, with);
      mpi_fprintf(stream, "/");
      mpi_fprint_rank(stream, param->bin->right, replace, with);
      mpi_fprintf(stream, ")");
      break;

    case ST_EXPR_TYPE_MOD:
      mpi_fprintf(stream, "(");
      mpi_fprint_rank(stream, param->bin->left, replace, with);
      mpi_fprintf(stream, "%%");
      mpi_fprint_rank(stream, param->bin->right, replace, with);
      mpi_fprintf(stream, ")");
      break;

    case ST_EXPR_TYPE_SHL:
      mpi_fprintf(stream, "(");
      mpi_fprint_rank(stream, param->bin->left, replace, with);
      mpi_fprintf(stream, "<<");
      mpi_fprint_rank(stream, param->bin->right, replace, with);
      mpi_fprintf(stream, ")");
      break;

    case ST_EXPR_TYPE_SHR:
      mpi_fprintf(stream, "(");
      mpi_fprint_rank(stream, param->bin->left, replace, with);
      mpi_fprintf(stream, ">>");
      mpi_fprint_rank(stream, param->bin->right, replace, with);
      mpi_fprintf(stream, ")");
      break;

    default:
      assert(0 /*Not supported*/);
  }
}


void mpi_fprint_msg_cond(FILE *stream, st_tree *tree, const msg_cond_t *msg_cond, int indent)
{
  static int condition_count = 0;
  assert(msg_cond != NULL);

  int in_group = -1;
  int group_memb_idx = 0;
  if (msg_cond->dimen == 0) {
    for (int group=0; group<tree->info->ngroup; group++) {
      if (strcmp(tree->info->groups[group]->name, msg_cond->name) == 0) {
        in_group = group;
      }
    }
  }

  // No dimension, ie. non-parameterised role.
  if (msg_cond->dimen == 0 && in_group == -1) {
    // if ( rank == Worker_RANK )
    mpi_fprintf(stream, "if ( %s == %s_RANK ) ", RANK_VARIABLE, msg_cond->name);
  }

  // 1D parameterised role.
  if (msg_cond->dimen == 1) {
    switch (msg_cond->param[0]->type) {
      case ST_EXPR_TYPE_CONST:
      case ST_EXPR_TYPE_VAR:
      case ST_EXPR_TYPE_ADD:
      case ST_EXPR_TYPE_SUB:
      case ST_EXPR_TYPE_MUL:
      case ST_EXPR_TYPE_DIV:
      case ST_EXPR_TYPE_MOD:
      case ST_EXPR_TYPE_SHL:
      case ST_EXPR_TYPE_SHR:
        // if ( rank == Worker_RANK(N-1) )
        mpi_fprintf(stream, "if ( %s == %s_RANK(", RANK_VARIABLE, msg_cond->name);
        mpi_fprint_expr(stream, msg_cond->param[0]);
        mpi_fprintf(stream, ") ) ");
        break;

      case ST_EXPR_TYPE_RNG:
        // if ( Worker_RANK(1) <= rank && rank <= Worker_RANK(N) )
        mpi_fprintf(stream, "if ( %s_RANK(", RANK_VARIABLE, msg_cond->name);
        mpi_fprint_expr(stream, msg_cond->param[0]->rng->from);
        mpi_fprintf(stream, ") <= %s && %s <= %s_RANK(", RANK_VARIABLE, RANK_VARIABLE, msg_cond->name);
        mpi_fprint_expr(stream, msg_cond->param[0]->rng->from);
        mpi_fprintf(stream, ") ) ");
        break;

      default:
        assert(0 /* No other forms of conditions are allowed */);
    }
  }

  if (msg_cond->dimen >= 2 || in_group != -1) {
    mpi_fprintf(stream, "int cond%d = 0;\n", condition_count);

    while (1) {
      if (in_group != -1) {
        mpi_fprintf(stream, "%*s// Group %s #%d\n", indent, SPACE, tree->info->groups[in_group]->name, group_memb_idx);
        msg_cond = tree->info->groups[in_group]->membs[group_memb_idx];
      }

      for (int param=0; param<msg_cond->dimen-1; param++) {
        // for (int x=1; x<=N; x++)
        // for (int y=1; y<=N-1; y++)
        // for (int z=2; z<=N; z++)
        if (msg_cond->param[param]->type == ST_EXPR_TYPE_RNG) {
          // 1..N
          indent += param;
          mpi_fprintf(stream, "%*sfor (int %c=", indent, SPACE, 'x'+param);
          mpi_fprint_expr(stream, msg_cond->param[param]->rng->from);
          mpi_fprintf(stream, "; %c<=", 'x'+param);
          mpi_fprint_expr(stream, msg_cond->param[param]->rng->to);
          mpi_fprintf(stream, "; %c++)\n", 'x'+param);
          indent -= param;
        } else {
          // 1 -> 1..1
          indent += param;
          mpi_fprintf(stream, "%*sfor (int %c=", indent, SPACE, 'x'+param);
          mpi_fprint_const_or_var(stream, tree, msg_cond->param[param]);
          mpi_fprintf(stream, "; %c<=", 'x'+param);
          mpi_fprint_const_or_var(stream, tree, msg_cond->param[param]);
          mpi_fprintf(stream, "; %c++)\n", 'x'+param);
          indent -= param;
        }
      }
      if (msg_cond->param[msg_cond->dimen-1]->type == ST_EXPR_TYPE_RNG) {
        indent += (msg_cond->dimen - 1);
        // cond0 |= ( Worker_RANK(x,y,1) <= rank && rank <= Worker_RANK(x,y,N) );
        mpi_fprintf(stream, "%*scond%d |= ( %s_RANK(", indent, SPACE, condition_count, msg_cond->name);
        for (int param=0; param<msg_cond->dimen-1; param++) mpi_fprintf(stream, "%c,", 'x'+param);
        mpi_fprint_const_or_var(stream, tree, msg_cond->param[msg_cond->dimen-1]->rng->from);
        mpi_fprintf(stream, ") <= %s && %s <= %s_RANK(", RANK_VARIABLE, RANK_VARIABLE, msg_cond->name);
        for (int param=0; param<msg_cond->dimen-1; param++) mpi_fprintf(stream, "%c,", 'x'+param);
        if (msg_cond->param[msg_cond->dimen-1]->rng->to->type > 2) {
          mpi_fprint_expr(stream, msg_cond->param[msg_cond->dimen-1]->rng->to);
        } else {
          mpi_fprint_const_or_var(stream, tree, msg_cond->param[msg_cond->dimen-1]->rng->to);
        }
        mpi_fprintf(stream, ") );\n");
        indent -= (msg_cond->dimen - 1);
      } else {
        indent += (msg_cond->dimen - 1);
        // cond0 |= ( rank == Worker_RANK(x,y,1) );
        mpi_fprintf(stream, "%*scond%d |= ( %s == %s_RANK(", indent, SPACE, condition_count, RANK_VARIABLE,  msg_cond->name);
        for (int param=0; param<msg_cond->dimen-1; param++) mpi_fprintf(stream, "%c,", 'x'+param);
        mpi_fprint_const_or_var(stream, tree, msg_cond->param[msg_cond->dimen-1]);
        mpi_fprintf(stream, ") );\n", condition_count, RANK_VARIABLE,  msg_cond->name);
        indent -= (msg_cond->dimen - 1);
      }

      if (in_group != -1 && group_memb_idx < tree->info->groups[in_group]->nmemb-1) {
        group_memb_idx++;
        continue;
      }

      break; // For normal case (ie. in_group == -1) this is a one-off loop
    }

    mpi_fprintf(stream, "%*sif (cond%d) ", indent, SPACE, condition_count);

    condition_count++;
  }

}


void mpi_fprint_children(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  int skip_node = 0;
  for (int child=0; child<node->nchild; child++) {

    skip_node = 0;
    if (child+1 < node->nchild) {
      // Do Allgather check.
      if (node->children[child]->type == ST_NODE_SEND && node->children[child+1]->type == ST_NODE_RECV) {
        if (   strcmp(node->children[child]->interaction->to[0]->name, ST_ROLE_ALL) == 0
            && strcmp(node->children[child+1]->interaction->from->name, ST_ROLE_ALL) == 0) {

          indent++;
          mpi_fprintf(stream, "%*s/* Allgather/Alltoall\n", indent, SPACE);
          scribble_fprint_send(stream, node->children[child], indent+1);
          scribble_fprint_recv(stream, node->children[child+1], indent+1);
          mpi_fprintf(stream, "%*s*/\n", indent, SPACE);
          mpi_fprint_allgather(pre_stream, stream, post_stream, tree, node->children[child], indent);
          indent--;

          skip_node = 1;
          child += 1;
        }
      }

      // Do Group-to-group check.
      if (!skip_node && node->children[child]->type == ST_NODE_RECV && node->children[child+1]->type == ST_NODE_SEND
          && node->children[child]->interaction->msg_cond == NULL && node->children[child+1]->interaction->msg_cond == NULL) {
        for (int group=0; group<tree->info->ngroup; group++) {
          if (   strcmp(tree->info->groups[group]->name, node->children[child]->interaction->from->name) == 0
              && strcmp(tree->info->groups[group]->name, node->children[child+1]->interaction->to[0]->name) == 0) {
            indent++;
            mpi_fprintf(stream, "%*s/* Group-to-group\n", indent, SPACE);
            scribble_fprint_recv(stream, node->children[child], indent+1);
            scribble_fprint_send(stream, node->children[child+1], indent+1);
            mpi_fprintf(stream, "%*s*/\n", indent, SPACE);
            mpi_fprint_allgather(pre_stream, stream, post_stream, tree, node->children[child], indent);
            indent--;
            skip_node = 1;
            child += 1;
          }
        }
      }

      // Do Scatter check.
      if (!skip_node && node->children[child]->type == ST_NODE_SEND && node->children[child+1]->type == ST_NODE_RECV
          && node->children[child]->interaction->msg_cond != NULL && node->children[child+1]->interaction->msg_cond != NULL) {
        if (   strcmp(node->children[child]->interaction->to[0]->name, ST_ROLE_ALL) == 0
            && strcmp(node->children[child+1]->interaction->msg_cond->name, ST_ROLE_ALL) == 0) {
          indent++;
          mpi_fprintf(stream, "%*s/* Scatter\n", indent, SPACE);
          scribble_fprint_send(stream, node->children[child], indent+1);
          scribble_fprint_recv(stream, node->children[child+1], indent+1);
          mpi_fprintf(stream, "%*s*/\n", indent, SPACE);
          mpi_fprint_scatter(pre_stream, stream, post_stream, tree, node->children[child+1], indent); // root role is in receive line.
          indent--;
          skip_node = 1;
          child += 1;
        }
      }

      // Do Gather check.
      if (!skip_node && node->children[child]->type == ST_NODE_RECV && node->children[child+1]->type == ST_NODE_SEND
          && node->children[child]->interaction->msg_cond != NULL && node->children[child+1]->interaction->msg_cond != NULL) {
        if (   strcmp(node->children[child]->interaction->from->name, ST_ROLE_ALL) == 0
            && strcmp(node->children[child+1]->interaction->msg_cond->name, ST_ROLE_ALL) == 0) {
          indent++;
          mpi_fprintf(stream, "%*s/* Gather\n", indent, SPACE);
          scribble_fprint_recv(stream, node->children[child], indent+1);
          scribble_fprint_send(stream, node->children[child+1], indent+1);
          mpi_fprintf(stream, "%*s*/\n", indent, SPACE);
          mpi_fprint_gather(pre_stream, stream, post_stream, tree, node->children[child+1], indent); // root role is in send line.
          indent--;
          skip_node = 1;
          child += 1;
        }
      }

    }

    if (!skip_node) {
      mpi_fprint_node(pre_stream, stream, post_stream, tree, node->children[child], indent+1);
    }

  }
}