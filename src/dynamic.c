#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sesstype/st_node.h>
#include <scribble/print.h>

#include "scribble/mpi_print.h"


extern unsigned int mpi_primitive_count;


void mpi_fprint_ifblk(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_IFBLK);

  // If
  mpi_fprintf(stream, "%*s", indent, SPACE);
  mpi_fprint_msg_cond(pre_stream, stream, post_stream, tree, node->ifblk->cond, indent);
  mpi_fprintf(stream, "{ // IF-BLK\n");

  indent++;
  for (int child=0; child<node->nchild; ++child) {
    mpi_fprint_node(pre_stream, stream, post_stream, tree, node->children[child], indent);
  }
  indent--;

  mpi_fprintf(stream, "%*s}\n", indent, SPACE);

  indent++;
}


#ifdef PABBLE_DYNAMIC
void mpi_fprint_oneof(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_ONEOF);

  if (node->oneof->unordered) {
    mpi_fprintf(stream, "%*sfor (int %s = ", indent, SPACE, node->oneof->range->bindvar);
    mpi_fprint_expr(stream, node->forloop->range->from);
    mpi_fprintf(stream, "; %s <= ", node->oneof->range->bindvar);
    mpi_fprint_expr(stream, node->forloop->range->to);
    mpi_fprintf(stream, "; %s++) {\n", node->oneof->range->bindvar);
    indent++;
  }

  // Variable declarations
  st_node *first_interaction = next_interaction(node);
  mpi_fprintf(pre_stream, "  int count%u = ", mpi_primitive_count);
  if (node->interaction->msgsig.npayload > 0 && node->interaction->msgsig.payloads[0].expr != NULL) {
    st_expr_fprint(pre_stream, node->interaction->msgsig.payloads[0].expr);
  } else {
    mpi_fprintf(pre_stream, "1");
  }
  mpi_fprintf(pre_stream, ";\n");

  mpi_fprintf(pre_stream, "  %s *buf%u = malloc(sizeof(%s) * count%u);\n",
      node->interaction->msgsig.npayload < 1 ? "unsigned char" : node->interaction->msgsig.payloads[0].type,
      mpi_primitive_count,
      node->interaction->msgsig.npayload < 1 ? "unsigned char" : node->interaction->msgsig.payloads[0].type,
      mpi_primitive_count);

  mpi_fprintf(stream, "%*sMPI_Recv(buf%u, count%u, ", indent, SPACE, mpi_primitive_count, mpi_primitive_count);
  mpi_fprint_datatype(pre_stream, stream, post_stream, node->interaction->msgsig);
  mpi_fprintf(stream, ", MPI_ANY_RANK, ");
  mpi_fprintf(stream, first_interaction->interaction->msgsig.op == NULL ? 0: first_interaction->interaction->msgsig.op); // This should be a constant
  mpi_fprintf(stream, ", MPI_COMM_WORLD, MPI_STATUS_IGNORE);\n"); // Ignore status (this is not non-blocking)

  mpi_fprintf(post_stream, "  free(buf%u);\n", mpi_primitive_count);

  for (int child=0; child<node->nchild; ++child) {
    mpi_fprint_node(pre_stream, stream, post_stream, tree, node->children[child], indent);
  }

  if (node->oneof->unordered) {
    indent--;
    mpi_fprintf(stream, "%*s}\n", indent, SPACE);
  }
}
#endif
