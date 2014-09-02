#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sesstype/st_node.h>
#include <sesstype/st_node_print.h>

#include <scribble/print.h>
#include <scribble/print_utils.h>

#include "scribble/mpi_print.h"


extern unsigned int mpi_primitive_count;


void mpi_fprint_allgather(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
#ifdef __DEBUG__
  fprintf_info(stderr, "%s:%d %s entry\nINFO/", __FILE__, __LINE__, __FUNCTION__);
  st_node_fprint(stderr, node, indent);
#endif

  assert(node != NULL);

  char *sbuf = sendbuf_name(node, mpi_primitive_count);
  char *rbuf = recvbuf_name(node, mpi_primitive_count);
  char *type = payload_type(node);
  char *count = payload_size(node);
  char *grp_name, *grp_comm, *grp_size;
  if (is_role_in_group(node->interaction->msg_cond->name, tree)) {
      grp_name = node->interaction->msg_cond->name;
      grp_comm = (char *)calloc(strlen(grp_name)+strlen("_comm")+1, sizeof(char));
      sprintf(grp_comm, "%s_comm", grp_name);
      grp_size = (char *)calloc(strlen("ngrp_")+strlen(grp_name)+1, sizeof(char));
      sprintf(grp_size, "ngrp_%s", grp_name);
  } else if (strcmp(node->interaction->msg_cond->name, ST_ROLE_ALL) == 0) {
      grp_name = NULL;
      grp_comm = strdup(COMM_VARIABLE);
      grp_size = strdup(SIZE_VARIABLE);
  } else {
      fprintf_error(stderr, "%s:%d %s Cannot find MPI process group (Is it properly defined?)\n", __FILE__, __LINE__, __FUNCTION__);
      assert(0);
  }

  mpi_fprintf(pre_stream, "  %s *%s;\n", type, rbuf);
  mpi_fprintf(pre_stream, "  %s *%s;\n", type, sbuf);

  mpi_fprintf(stream, "%*s%s = calloc(%s*%s, sizeof(%s));\n", indent, SPACE, rbuf, count, grp_size, type);
  mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);
  mpi_fprintf(stream, "%*s%s = pabble_sendq_dequeue();\n", indent, SPACE, sbuf);
  mpi_fprintf(stream, "%*sMPI_Allather(%s, %s, MPI_%s, %s, %s, MPI_%s, %s);\n", indent, SPACE, sbuf, count, type, rbuf, count, type, grp_comm);
  mpi_fprintf(stream, "%*sfree(%s);\n", indent, SPACE, sbuf);
  mpi_fprintf(stream, "%*spabble_recvq_enqueue(%s, %s);\n", indent, SPACE, node->interaction->msgsig.op, rbuf);
  mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);

  free(sbuf);
  free(rbuf);
  free(count);
  free(type);
  free(grp_comm);
  free(grp_size);

  mpi_primitive_count++;
}


void mpi_fprint_allreduce(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
#ifdef __DEBUG__
  fprintf_info(stderr, "%s:%d %s entry\nINFO/", __FILE__, __LINE__, __FUNCTION__);
  st_node_fprint(stderr, node, indent);
#endif

  assert(node != NULL && node->type == ST_NODE_ALLREDUCE);

  char *sbuf = sendbuf_name(node, mpi_primitive_count);
  char *rbuf = recvbuf_name(node, mpi_primitive_count);
  char *type = payload_type(node);
  char *count = payload_size(node);
  char *grp_name, *grp_comm, *grp_size;
  if (is_role_in_group(node->interaction->msg_cond->name, tree)) {
      grp_name = node->interaction->msg_cond->name;
      grp_comm = (char *)calloc(strlen(grp_name)+strlen("_comm")+1, sizeof(char));
      sprintf(grp_comm, "%s_comm", grp_name);
      grp_size = (char *)calloc(strlen("ngrp_")+strlen(grp_name)+1, sizeof(char));
      sprintf(grp_size, "ngrp_%s", grp_name);
  } else if (strcmp(node->interaction->msg_cond->name, ST_ROLE_ALL) == 0) {
      grp_name = NULL;
      grp_comm = strdup(COMM_VARIABLE);
      grp_size = strdup(SIZE_VARIABLE);
  } else {
      fprintf_error(stderr, "%s:%d %s Cannot find MPI process group (Is it properly defined?)\n", __FILE__, __LINE__, __FUNCTION__);
      assert(0);
  }

  mpi_fprintf(pre_stream, "  %s *%s;\n", type, rbuf);
  mpi_fprintf(pre_stream, "  %s *%s;\n", type, sbuf);

  mpi_fprintf(stream, "%*s%s = calloc(%s*%s, sizeof(%s));\n", indent, SPACE, rbuf, count, grp_size, type);
  mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);
  mpi_fprintf(stream, "%*s%s = pabble_sendq_dequeue();\n", indent, SPACE, sbuf);
  mpi_fprintf(stream, "%*sMPI_Allreduce(%s, %s, %s, MPI_%s, %s, %s);\n", grp_comm, indent, SPACE, sbuf, rbuf, count, type, node->interaction->msgsig.op, grp_comm);
  mpi_fprintf(stream, "%*sfree(%s);\n", indent, SPACE, sbuf);
  mpi_fprintf(stream, "%*spabble_recvq_enqueue(%s, %s);\n", indent, SPACE, node->interaction->msgsig.op, rbuf);
  mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);

  free(sbuf);
  free(rbuf);
  free(count);
  free(type);
  free(grp_comm);
  free(grp_size);

  mpi_primitive_count++;
}

void mpi_fprint_scatter(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_RECV);

#ifdef __DEBUG__
  fprintf_info(stderr, "%s:%d %s entry\nINFO/", __FILE__, __LINE__, __FUNCTION__);
  st_node_fprint(stderr, node, indent);
#endif

  char *sbuf = sendbuf_name(node, mpi_primitive_count);
  char *rbuf = recvbuf_name(node, mpi_primitive_count);
  char *type = payload_type(node);
  char *count = payload_size(node);
  char *grp_name, *grp_comm, *grp_size;
  if (is_role_in_group(node->interaction->msg_cond->name, tree)) {
      grp_name = node->interaction->msg_cond->name;
      grp_comm = (char *)calloc(strlen(grp_name)+strlen("_comm")+1, sizeof(char));
      sprintf(grp_comm, "%s_comm", grp_name);
      grp_size = (char *)calloc(strlen("ngrp_")+strlen(grp_name)+1, sizeof(char));
      sprintf(grp_size, "ngrp_%s", grp_name);
  } else if (strcmp(node->interaction->msg_cond->name, ST_ROLE_ALL) == 0) {
      grp_name = NULL;
      grp_comm = strdup(COMM_VARIABLE);
      grp_size = strdup(SIZE_VARIABLE);
  } else {
      fprintf_error(stderr, "%s:%d %s Cannot find MPI process group (Is it properly defined?)\n", __FILE__, __LINE__, __FUNCTION__);
      assert(0);
  }

  // Typedef for variable types
  mpi_fprint_datatype_def(pre_stream, stream, post_stream, node->interaction->msgsig);

  // Buffers definitions
  mpi_fprintf(pre_stream, "  %s *%s;\n", type, rbuf);
  mpi_fprintf(pre_stream, "  %s *%s;\n", type, sbuf);

  mpi_fprintf(stream, "%*s%s = calloc(%s, sizeof(%s));\n", indent, SPACE, rbuf, count, type);
  mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);
  mpi_fprintf(stream, "%*s%s = pabble_sendq_dequeue();\n", indent, SPACE, sbuf);
  mpi_fprintf(stream, "%*sMPI_Scatter(%s, %s, MPI_%s, %s, %s, MPI_%s, ", indent, SPACE, sbuf, count, type, rbuf, count, type);
  mpi_fprintf(stream, "%s_RANK(", node->interaction->from->name);
  for (int param=0; param<node->interaction->from->dimen; param++) {
    if (param!=0) mpi_fprintf(stream, ",");
    mpi_fprint_const_or_var(stream, tree, node->interaction->from->param[param]);
  }
  mpi_fprintf(stream, "), %s);\n", grp_comm);
  mpi_fprintf(stream, "%*sfree(%s);\n", indent, SPACE, sbuf);
  mpi_fprintf(stream, "%*spabble_recvq_enqueue(%s, %s);\n", indent, SPACE, node->interaction->msgsig.op, rbuf);
  mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);

  free(sbuf);
  free(rbuf);
  free(count);
  free(type);
  free(grp_comm);
  free(grp_size);

  mpi_primitive_count++;
}


void mpi_fprint_gather(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent)
{
#ifdef __DEBUG__
  fprintf_info(stderr, "%s:%d %s entry\nINFO/", __FILE__, __LINE__, __FUNCTION__);
  st_node_fprint(stderr, node, indent);
#endif

  assert(node != NULL && node->type == ST_NODE_SEND);

  char *sbuf = sendbuf_name(node, mpi_primitive_count);
  char *rbuf = recvbuf_name(node, mpi_primitive_count);
  char *type = payload_type(node);
  char *count = payload_size(node);
  char *grp_name, *grp_comm, *grp_size;
  if (is_role_in_group(node->interaction->msg_cond->name, tree)) {
      grp_name = node->interaction->msg_cond->name;
      grp_comm = (char *)calloc(strlen(grp_name)+strlen("_comm")+1, sizeof(char));
      sprintf(grp_comm, "%s_comm", grp_name);
      grp_size = (char *)calloc(strlen("ngrp_")+strlen(grp_name)+1, sizeof(char));
      sprintf(grp_size, "ngrp_%s", grp_name);
  } else if (strcmp(node->interaction->msg_cond->name, ST_ROLE_ALL) == 0) {
      grp_name = NULL;
      grp_comm = strdup(COMM_VARIABLE);
      grp_size = strdup(SIZE_VARIABLE);
  } else {
      fprintf_error(stderr, "%s:%d %s Cannot find MPI process group (Is it properly defined?)\n", __FILE__, __LINE__, __FUNCTION__);
      assert(0);
  }

  // Typedef for variable types
  mpi_fprint_datatype_def(pre_stream, stream, post_stream, node->interaction->msgsig);

  // Buffers definitions
  mpi_fprintf(pre_stream, "  %s *%s;\n", type, rbuf);
  mpi_fprintf(pre_stream, "  %s *%s;\n", type, sbuf);

  mpi_fprintf(stream, "%*s%s = calloc(%s*%s, sizeof(%s));\n", indent, SPACE, rbuf, count, grp_size, type);
  mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);
  mpi_fprintf(stream, "%*s%s = pabble_sendq_dequeue();\n", indent, SPACE, sbuf);
  if (strncmp(node->interaction->msgsig.op, "MPI_", 4) == 0) {
    mpi_fprintf(stream, "%*sMPI_Reduce(%s, %s, %s, MPI_%s, %s, ", indent, SPACE, sbuf, rbuf, count, type, node->interaction->msgsig.op);
  } else {
    mpi_fprintf(stream, "%*sMPI_Gather(%s, %s, MPI_%s, %s, %s, MPI_%s, ", indent, SPACE, sbuf, count, type, rbuf, count, type);
  }
  mpi_fprintf(stream, "%s_RANK(", node->interaction->to[0]->name);
  for (int param=0; param<node->interaction->to[0]->dimen; param++) {
    if (param!=0) mpi_fprintf(stream, ",");
    mpi_fprint_const_or_var(stream, tree, node->interaction->to[0]->param[param]);
  }
  mpi_fprintf(stream, "), %s);\n", grp_comm);
  mpi_fprintf(stream, "%*sfree(%s);\n", indent, SPACE, sbuf);
  mpi_fprintf(stream, "%*spabble_recvq_enqueue(%s, %s);\n", indent, SPACE, node->interaction->msgsig.op, rbuf);
  mpi_fprintf(stream, "#pragma pabble %s\n", node->interaction->msgsig.op);

  free(sbuf);
  free(rbuf);
  free(count);
  free(type);
  free(grp_comm);
  free(grp_size);

  mpi_primitive_count++;
}
