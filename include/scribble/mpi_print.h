#ifndef SCRIBBLE__MPI_PRINT__H__
#define SCRIBBLE__MPI_PRINT__H__

#include <stdio.h>
#include <sesstype/st_node.h>

#define META_VARIABLE "meta"
#define RANK_VARIABLE META_VARIABLE".pid"
#define SIZE_VARIABLE META_VARIABLE".nprocs"
#define COMM_VARIABLE META_VARIABLE".comm"
#define DATASIZE_FUNC META_VARIABLE".buflen"
#define SPACE "  "
#define ST_ROLE_ALL "__All"


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
//
int is_role_in_group(char *role_name, st_tree *tree);

void mpi_fprint_const_or_var(FILE *stream, st_tree *tree, st_expr *expr);
void mpi_fprint_msg_cond(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, const msg_cond_t *msg_cond, int indent);
void mpi_fprint_datatype_def(FILE *pre_stream, FILE *stream, FILE *post_stream, st_node_msgsig_t msgsig);
void mpi_fprint_datatype(FILE *pre_stream, FILE *stream, FILE *post_stream, st_node_msgsig_t msgsig);
void mpi_fprint_expr(FILE *stream, st_expr *expr);
void mpi_fprint_rank(FILE *stream, st_expr *param, const char *replace, const char *with);
void mpi_fprint_role(FILE *stream, st_role *role);
void mpi_fprint_children(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
st_node *next_interaction(st_node *node);

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

void mpi_fprint_ifblk(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);
void mpi_fprint_oneof(FILE *pre_stream, FILE *stream, FILE *post_stream, st_tree *tree, st_node *node, int indent);

char *payload_type(st_node *node);
char *payload_size(st_node *node);
char *sendbuf_name(st_node *node, unsigned int idx);
char *recvbuf_name(st_node *node, unsigned int idx);

#ifdef __cplusplus
}
#endif

#endif // SCRIBBLE__MPI__PRINT__H__
