#ifndef SCRIBBLE__MPI_PRINT__H__
#define SCRIBBLE__MPI_PRINT__H__

#include <stdio.h>
#include <sesstype/st_node.h>

#define RANK_VARIABLE "rank"
#define SIZE_VARIABLE "size"
#define SPACE         "  "


typedef struct {
  char **strings;
  int count;
} string_list;

#ifdef __cplusplus
extern "C" {
#endif


// Toplevel

void mpi_print(FILE *stream, st_tree *local_tree);
void mpi_fprintf(FILE *stream, const char *format, ...);

// Helpers

void mpi_fprint_const_or_var(FILE *stream, st_tree *tree, st_expr *expr);
void mpi_fprint_msg_cond(FILE *stream, st_tree *tree, const msg_cond_t *msg_cond, int indent);
void mpi_fprint_datatype(FILE *stream, st_node_msgsig_t msgsig);
void mpi_fprint_expr(FILE *stream, st_expr *expr);
void mpi_fprint_rank(FILE *stream, st_expr *param, const char *replace, const char *with);
void mpi_fprint_role(FILE *stream, st_role *role);
void mpi_fprint_children(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);

void mpi_fprint_node(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_root(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_send(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_recv(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_choice(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_recur(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_continue(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_parallel(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_for(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_allreduce(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_allgather(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_scatter(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_gather(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);


#ifdef __cplusplus
}
#endif

#endif // SCRIBBLE__MPI__PRINT__H__
