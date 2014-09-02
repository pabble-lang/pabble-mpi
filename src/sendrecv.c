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

  if (strcmp(node->interaction->to[0]->name, "__self") != 0) {
    char *sbuf = sendbuf_name(node, mpi_primitive_count);
    char *count = payload_size(node);
    char *type = payload_type(node);

    // If
    if (node->interaction->msg_cond != NULL) {
      mpi_fprint_msg_cond(pre_stream, stream, post_stream, tree, node->interaction->msg_cond, indent);
      mpi_fprintf(stream, "{\n");
      indent++;
    }

    if (node->interaction->msgsig.npayload != 1) {
      fprintf(stderr, "Warning: there are %d payload types, expecting 1\n", node->interaction->msgsig.npayload);
    }

    // Typedef for variable types
    mpi_fprint_datatype_def(pre_stream, stream, post_stream, node->interaction->msgsig);

    // Buffer and req/stat definition
    mpi_fprintf(pre_stream, "  %s *%s;\n", type, sbuf);
    mpi_fprintf(pre_stream, "  MPI_Request req%u_s; MPI_Status stat%u_s;\n", mpi_primitive_count, mpi_primitive_count);

    mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);
    mpi_fprintf(stream, "%*s%s = pabble_sendq_dequeue();\n", indent, SPACE, sbuf);
    mpi_fprintf(stream, "%*sMPI_Isend(%s, %s, MPI_%s, ", indent, SPACE, sbuf, count, type);
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
                //XXX Allowing arbritary expressions
                //mpi_fprint_const_or_var(stream, tree, tree->info->roles[role]->param[1]->rng->to);
                mpi_fprint_expr(stream, tree->info->roles[role]->param[1]->rng->to);
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
    } else {
      assert(node->interaction->to[0]->dimen == 0);
      mpi_fprintf(stream, "/*w/o cond.*/%s", node->interaction->to[0]->name);
    }
    mpi_fprintf(stream, ", %s, %s, &req%u_s);\n", node->interaction->msgsig.op, COMM_VARIABLE, mpi_primitive_count);
    mpi_fprintf(stream, "%*sMPI_Wait(&req%u_s, &stat%u_s);\n", indent, SPACE, mpi_primitive_count, mpi_primitive_count);
    mpi_fprintf(stream, "%*sfree(%s);\n", indent, SPACE, sbuf);

    if (node->interaction->msg_cond != NULL) {
      indent--;
      mpi_fprintf(stream, "%*s}\n", indent, SPACE);
    }
    free(sbuf);
    free(count);
    free(type);
    mpi_primitive_count++;
  } else {
    mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);
  }
}


void mpi_fprint_recv(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_RECV);

  if (strcmp(node->interaction->from->name, "__self") != 0) {
    char *rbuf = recvbuf_name(node, mpi_primitive_count);
    char *count = payload_size(node);
    char *type = payload_type(node);

    // If
    if (node->interaction->msg_cond != NULL) {
      mpi_fprint_msg_cond(pre_stream, stream, post_stream, tree, node->interaction->msg_cond, indent);
      mpi_fprintf(stream, "{\n");
      indent++;
    }

    if (node->interaction->msgsig.npayload != 1) {
      fprintf(stderr, "Warning: there are %d payload types, expecting 1\n", node->interaction->msgsig.npayload);
    }

    // Typedef for variable types
    mpi_fprint_datatype_def(pre_stream, stream, post_stream, node->interaction->msgsig);

    // Buffer and req/stat definition
    mpi_fprintf(pre_stream, "  %s *%s;\n", type, rbuf);
    mpi_fprintf(pre_stream, "  MPI_Request req%u_r; MPI_Status stat%u_r;\n", mpi_primitive_count, mpi_primitive_count);

    mpi_fprintf(stream, "%*s%s = calloc(%s, sizeof(%s));\n", indent, SPACE, rbuf, count, type);
    mpi_fprintf(stream, "%*sMPI_Irecv(%s, %s, MPI_%s, ", indent, SPACE, rbuf, count, type);
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
                //XXX Allowing arbritary expressions
                mpi_fprint_expr(stream, tree->info->roles[role]->param[1]->rng->to);
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

    } else {
      assert(node->interaction->from->dimen == 0);
      mpi_fprintf(stream, "/*w/o cond.*/%s", node->interaction->from->name);
    }
    mpi_fprintf(stream, ", %s, %s, &req%u_r);\n", node->interaction->msgsig.op, COMM_VARIABLE, mpi_primitive_count);
    mpi_fprintf(stream, "%*sMPI_Wait(&req%u_r, &stat%u_r);\n", indent, SPACE, mpi_primitive_count, mpi_primitive_count);
    mpi_fprintf(stream, "%*spabble_recvq_enqueue(%s, %s);\n", indent, SPACE, node->interaction->msgsig.op, rbuf);
    mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);

    if (node->interaction->msg_cond != NULL) {
      indent--;
      mpi_fprintf(stream, "%*s}\n", indent, SPACE);
    }
    free(rbuf);
    free(count);
    free(type);
  }
}
