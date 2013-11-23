#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sesstype/st_node.h>
#include <scribble/print.h>

#include "scribble/mpi_print.h"


extern unsigned int mpi_primitive_count;


void mpi_fprint_send(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_SEND);


  // If
  if (node->interaction->msg_cond != NULL) {
    mpi_fprintf(stream, "%*s", indent, SPACE);
    mpi_fprint_msg_cond(stream, tree, node->interaction->msg_cond, indent);
    mpi_fprintf(stream, "{\n");
    indent++;
  }

  // Variable declarations
  mpi_fprintf(pre_stream, "  int count%u = 1 /* CHANGE ME */;\n", mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *buf%u = malloc(sizeof(%s) * count%u);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(stream, "#pragma pabble compute%u (buf%u, count%u)\n", mpi_primitive_count, mpi_primitive_count, mpi_primitive_count);
  mpi_fprintf(stream, "%*sMPI_Send(buf%u, count%u, ", indent, SPACE, mpi_primitive_count, mpi_primitive_count);
  mpi_fprint_datatype(stream, node->interaction->msgsig);
  mpi_fprintf(stream, ", ");
  char *rankbindvar = NULL;
  if (node->interaction->msg_cond != NULL) { // There is msg_cond, so need to do role->rank conversion

    mpi_fprintf(stream, "/*");
    scribble_fprint_role(stream, node->interaction->to[0]);
    mpi_fprintf(stream, "*/");
    switch (node->interaction->msg_cond->dimen) {
      case 0: // Ordinary rules
        // Use the #define directly
        mpi_fprintf(stream, "%s_RANK", node->interaction->msg_cond->name);
        break;

      case 1: // Linear parameterised roles

        if (node->interaction->msg_cond->param[0]->type != ST_EXPR_TYPE_RNG) { // Just constants

          mpi_fprintf(stream, "%s_RANK(", node->interaction->to[0]->name);
          mpi_fprint_expr(stream, node->interaction->to[0]->param[0]);
          mpi_fprintf(stream, ")");

        } else {
          // Use the offset directly, replacing bind variable with rank
          //   eg. if R[i:1..3] D() to R[i+1] --> rank+1
          if (node->interaction->msg_cond->param[0]->type == ST_EXPR_TYPE_RNG) {
            rankbindvar = strdup(node->interaction->msg_cond->param[0]->rng->bindvar);
          }
          mpi_fprint_rank(stream, node->interaction->to[0]->param[0], rankbindvar, RANK_VARIABLE);
        }


        break;

      case 2: // 2D parameterised roles
        if (   node->interaction->msg_cond->param[0]->type != ST_EXPR_TYPE_RNG
            && node->interaction->msg_cond->param[1]->type != ST_EXPR_TYPE_RNG) { // Just constants

          mpi_fprintf(stream, "%s_RANK(", node->interaction->to[0]->name);
          mpi_fprint_expr(stream, node->interaction->to[0]->param[0]);
          mpi_fprintf(stream, ",");
          mpi_fprint_expr(stream, node->interaction->to[0]->param[1]);
          mpi_fprintf(stream, ")");

        } else { // not [const][const]

          // Expression in the form of
          //   rank
          //     + ( x-offset * y-size )
          //     + ( y-offset )

          mpi_fprintf(stream, "%s+(", RANK_VARIABLE);

          // If x-component condition is a range -->
          //   1. Replace bind variable with "0"
          //      ie. i+1 --> 0+1
          //   2. Multiply with 'unit'
          //
          if (node->interaction->msg_cond->param[0]->type == ST_EXPR_TYPE_RNG) { // cond == RNG
            rankbindvar = strdup(node->interaction->msg_cond->param[0]->rng->bindvar);
            mpi_fprint_rank(stream, node->interaction->to[0]->param[0], rankbindvar, "0"); // Hack that only work with +/- indices
          } else { // cond != RNG
            // Print the expression as expression (to be multiplied by the unit)
            mpi_fprint_expr(stream, node->interaction->to[0]->param[0]);
          }

          mpi_fprintf(stream, ")*(");

          // Find the 'unit/multiplier' of the argument

          for (int role=0; role<tree->info->nrole; role++) {
            if (strcmp(node->interaction->msg_cond->name, tree->info->roles[role]->name) == 0) {
              assert(tree->info->roles[role]->dimen == 2);
              mpi_fprint_const_or_var(stream, tree, tree->info->roles[role]->param[1]->rng->to);
              mpi_fprintf(stream, "-");
              mpi_fprint_const_or_var(stream, tree, tree->info->roles[role]->param[1]->rng->from);
              mpi_fprintf(stream, "+1");
            }
          }

          mpi_fprintf(stream, ")+(");

          // 2nd index

          if (node->interaction->msg_cond->param[1]->type == ST_EXPR_TYPE_RNG) {
            rankbindvar = strdup(node->interaction->msg_cond->param[1]->rng->bindvar);
            mpi_fprint_rank(stream, node->interaction->to[0]->param[1], rankbindvar, "0");
          } else {
            mpi_fprint_expr(stream, node->interaction->to[0]->param[1]);
          }

          mpi_fprintf(stream, ")");

        } // Not [const][const]
        break;

      default:
        assert(0 /* Not supported beyond 2D parameterised roles */);
        break;
    }
  }
  mpi_fprintf(stream, ", ");
  mpi_fprintf(stream, node->interaction->msgsig.op == NULL ? 0: node->interaction->msgsig.op); // This should be a constant
  mpi_fprintf(stream, ", MPI_COMM_WORLD);\n");

  mpi_fprintf(post_stream, "  free(buf%u);\n", mpi_primitive_count);

  if (node->interaction->msg_cond != NULL) {
    indent--;
    mpi_fprintf(stream, "%*s}\n", indent, SPACE);
  }

  mpi_primitive_count++;
}


void mpi_fprint_recv(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_RECV);

  mpi_fprintf(stream, "%*s", indent, SPACE);

  // If
  if (node->interaction->msg_cond != NULL) {
    mpi_fprintf(stream, "%*s", indent, SPACE);
    mpi_fprint_msg_cond(stream, tree, node->interaction->msg_cond, indent);
    mpi_fprintf(stream, "{\n");
    indent++;
  }


    // Variable declarations
    mpi_fprintf(pre_stream, "  int count%u = 1 /* CHANGE ME */;\n", mpi_primitive_count);

    mpi_fprintf(pre_stream, "  %s *buf%u = malloc(sizeof(%s) * count%u);\n",
        strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
        mpi_primitive_count,
        strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
        mpi_primitive_count);

    mpi_fprintf(stream, "%*sMPI_Recv(buf%u, count%u, ", indent, SPACE, mpi_primitive_count, mpi_primitive_count);
    mpi_fprint_datatype(stream, node->interaction->msgsig);
    mpi_fprintf(stream, ", ");
    char *rankbindvar = NULL;
    if (node->interaction->msg_cond != NULL) { // There is msg_cond, so need to do role->rank conversion

      mpi_fprintf(stream, "/*");
      scribble_fprint_role(stream, node->interaction->from);
      mpi_fprintf(stream, "*/");
      switch (node->interaction->msg_cond->dimen) {
        case 0: // Ordinary rules
          // Use the #define directly
          mpi_fprintf(stream, "%s_RANK", node->interaction->msg_cond->name);
          break;

        case 1: // Linear parameterised roles

          if (node->interaction->msg_cond->param[0]->type != ST_EXPR_TYPE_RNG) { // Just constants

            mpi_fprintf(stream, "%s_RANK(", node->interaction->from->name);
            mpi_fprint_expr(stream, node->interaction->from->param[0]);
            mpi_fprintf(stream, ")");

          } else {
            // Use the offset directly, replacing bind variable with rank
            //   eg. if R[i:1..3] D() from R[i-1] --> rank-1
            if (node->interaction->msg_cond->param[0]->type == ST_EXPR_TYPE_RNG) {
              rankbindvar = strdup(node->interaction->msg_cond->param[0]->rng->bindvar);
            }
            mpi_fprint_rank(stream, node->interaction->from->param[0], rankbindvar, RANK_VARIABLE);
          }


          break;

        case 2: // 2D parameterised roles
          if (   node->interaction->msg_cond->param[0]->type != ST_EXPR_TYPE_RNG
              && node->interaction->msg_cond->param[1]->type != ST_EXPR_TYPE_RNG) { // Just constants

            mpi_fprintf(stream, "%s_RANK(", node->interaction->from->name);
            mpi_fprint_expr(stream, node->interaction->from->param[0]);
            mpi_fprintf(stream, ",");
            mpi_fprint_expr(stream, node->interaction->from->param[1]);
            mpi_fprintf(stream, ")");

          } else { // not [const][const]

            // Expression in the form of
            //   rank
            //     + ( x-offset * y-size )
            //     + ( y-offset )

            mpi_fprintf(stream, "%s+(", RANK_VARIABLE);

            // If x-component condition is a range -->
            //   1. Replace bind variable with "0"
            //      ie. i+1 --> 0+1
            //   2. Multiply with 'unit'
            //
            if (node->interaction->msg_cond->param[0]->type == ST_EXPR_TYPE_RNG) { // cond == RNG
              rankbindvar = strdup(node->interaction->msg_cond->param[0]->rng->bindvar);
              mpi_fprint_rank(stream, node->interaction->from->param[0], rankbindvar, "0"); // Hack that only work with +/- indices
            } else { // cond != RNG
              // Print the expression as expression (to be multiplied by the unit)
              mpi_fprint_expr(stream, node->interaction->from->param[0]);
            }

            mpi_fprintf(stream, ")*(");

            // Find the 'unit/multiplier' of the argument

            for (int role=0; role<tree->info->nrole; role++) {
              if (strcmp(node->interaction->msg_cond->name, tree->info->roles[role]->name) == 0) {
                assert(tree->info->roles[role]->dimen == 2);
                mpi_fprint_const_or_var(stream, tree, tree->info->roles[role]->param[1]->rng->to);
                mpi_fprintf(stream, "-");
                mpi_fprint_const_or_var(stream, tree, tree->info->roles[role]->param[1]->rng->from);
                mpi_fprintf(stream, "+1");
              }
            }

            mpi_fprintf(stream, ")+(");

            // 2nd index

            if (node->interaction->msg_cond->param[1]->type == ST_EXPR_TYPE_RNG) {
              rankbindvar = strdup(node->interaction->msg_cond->param[1]->rng->bindvar);
              mpi_fprint_rank(stream, node->interaction->from->param[1], rankbindvar, "0");
            } else {
              mpi_fprint_expr(stream, node->interaction->from->param[1]);
            }

            mpi_fprintf(stream, ")");

          } // Not [const][const]
          break;

        default:
          assert(0 /* Not supported beyond 2D parameterised roles */);
          break;
      }

    }
    mpi_fprintf(stream, ", ");
    mpi_fprintf(stream, node->interaction->msgsig.op == NULL ? 0: node->interaction->msgsig.op); // This should be a constant
    mpi_fprintf(stream, ", MPI_COMM_WORLD, MPI_STATUS_IGNORE);\n"); // Ignore status (this is not non-blocking)
    mpi_fprintf(stream, "#pragma pabble compute%u (buf%u, count%u)\n", mpi_primitive_count, mpi_primitive_count, mpi_primitive_count);

    mpi_fprintf(post_stream, "  free(buf%u);\n", mpi_primitive_count);

  if (node->interaction->msg_cond != NULL) {
    indent--;
    mpi_fprintf(stream, "%*s}\n", indent, SPACE);
  }

  mpi_primitive_count++;
}
