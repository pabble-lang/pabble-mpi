#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sesstype/st_node.h>
#include <scribble/print.h>

#include "scribble/mpi_print.h"


extern unsigned int mpi_primitive_count;


void mpi_fprint_allgather(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  // Variable declarations
  mpi_fprintf(pre_stream, "  int count%u = 1 /* CHANGE ME */;\n", mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *sbuf%u = malloc(sizeof(%s) * count%u);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *rbuf%u = malloc(sizeof(%s) * count%u * size);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(stream, "%*sMPI_Allgather(sbuf%u, count%u, ", indent, SPACE, mpi_primitive_count, mpi_primitive_count);
  mpi_fprint_datatype(stream, node->interaction->msgsig);
  mpi_fprintf(stream, ", ");
  mpi_fprintf(stream, "rbuf%u, count%u, ", mpi_primitive_count, mpi_primitive_count);
  mpi_fprint_datatype(stream, node->interaction->msgsig);
  mpi_fprintf(stream, ", ");
  if ((node->type == ST_NODE_SEND && strcmp(node->interaction->to[0]->name, ST_ROLE_ALL) == 0)
      ||(node->type == ST_NODE_RECV && strcmp(node->interaction->from->name, ST_ROLE_ALL) == 0)) {
    mpi_fprintf(stream, "MPI_COMM_WORLD);\n");
  } else {
    mpi_fprintf(stream, "%s_comm);\n", node->interaction->from == NULL? node->interaction->to[0]->name : node->interaction->from->name);
  }

  mpi_fprintf(post_stream, "  free(sbuf%u);\n", mpi_primitive_count);
  mpi_fprintf(post_stream, "  free(rbuf%u);\n", mpi_primitive_count);

  mpi_primitive_count++;
}


void mpi_fprint_allreduce(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_ALLREDUCE);


  // Variable declarations
  mpi_fprintf(pre_stream, "  int count%u = 1 /* CHANGE ME */;\n", mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *sbuf%u = malloc(sizeof(%s) * count%u);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *rbuf%u = malloc(sizeof(%s) * count%u);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(stream, "%*sMPI_Reduce(sbuf%u, rbuf%u, count, ", indent, SPACE, mpi_primitive_count, mpi_primitive_count);
  mpi_fprint_datatype(stream, node->interaction->msgsig);
  mpi_fprintf(stream, ", %s, MPI_COMM_WORLD);\n", node->interaction->msgsig.op); // MPI_Op

  mpi_fprintf(post_stream, "  free(sbuf%u);\n", mpi_primitive_count);
  mpi_fprintf(post_stream, "  free(rbuf%u);\n", mpi_primitive_count);

  mpi_primitive_count++;
}

void mpi_fprint_scatter(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_RECV);


  // Variable declarations
  mpi_fprintf(pre_stream, "  int count%u = 1 /* CHANGE ME */;\n", mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *sbuf%u = malloc(sizeof(%s) * count%u);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *rbuf%u = malloc(sizeof(%s) * count%u * size);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(stream, "%*sMPI_Scatter(sbuf%u, count%u, ", indent, SPACE, mpi_primitive_count, mpi_primitive_count);
  mpi_fprint_datatype(stream, node->interaction->msgsig);
  mpi_fprintf(stream, ", ");
  mpi_fprintf(stream, "rbuf%u, count%u, ", mpi_primitive_count, mpi_primitive_count);
  mpi_fprint_datatype(stream, node->interaction->msgsig);
  mpi_fprintf(stream, ", %s_RANK(", node->interaction->from->name);
  for (int param=0; param<node->interaction->from->dimen; param++) {
    if (param!=0) mpi_fprintf(stream, ",");
    mpi_fprint_const_or_var(stream, tree, node->interaction->from->param[param]);
  }
  mpi_fprintf(stream, "), MPI_COMM_WORLD);\n");

  mpi_fprintf(post_stream, "  free(sbuf%u);\n", mpi_primitive_count);
  mpi_fprintf(post_stream, "  free(rbuf%u);\n", mpi_primitive_count);

  mpi_primitive_count++;
}

void mpi_fprint_gather(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_SEND);


  // Variable declarations
  mpi_fprintf(pre_stream, "  int count%u = 1 /* CHANGE ME */;\n", mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *sbuf%u = malloc(sizeof(%s) * count%u);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(pre_stream, "  %s *rbuf%u = malloc(sizeof(%s) * count%u * size);\n",
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count,
      strlen(node->interaction->msgsig.payload) == 0 ? "unsigned char" : node->interaction->msgsig.payload,
      mpi_primitive_count);

  mpi_fprintf(stream, "%*s", indent, SPACE);

  if (strlen(node->interaction->msgsig.op)>0) {
    mpi_fprintf(stream, "MPI_Reduce(sbuf%u, count%u, ", mpi_primitive_count, mpi_primitive_count); // Reduce (MPI_Op as Label)
  } else {
    mpi_fprintf(stream, "MPI_Gather(sbuf%u, count%u, ", mpi_primitive_count, mpi_primitive_count); // Gather
  }
  mpi_fprint_datatype(stream, node->interaction->msgsig);
  mpi_fprintf(stream, ", rbuf%u, count%u, ", mpi_primitive_count, mpi_primitive_count);
  mpi_fprint_datatype(stream, node->interaction->msgsig);
  if (strlen(node->interaction->msgsig.op)>0) {
    mpi_fprintf(stream, ", %s", node->interaction->msgsig.op); // MPI_Op
  }
  mpi_fprintf(stream, ", %s_RANK(", node->interaction->to[0]->name);
  for (int param=0; param<node->interaction->to[0]->dimen; param++) {
    if (param!=0) mpi_fprintf(stream, ",");
    mpi_fprint_const_or_var(stream, tree, node->interaction->to[0]->param[param]);
  }
  mpi_fprintf(stream, "), MPI_COMM_WORLD);\n");

  mpi_fprintf(post_stream, "  free(sbuf%u);\n", mpi_primitive_count);
  mpi_fprintf(post_stream, "  free(rbuf%u);\n", mpi_primitive_count);

  mpi_primitive_count++;
}
