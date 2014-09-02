#ifndef SCRIBBLE__PABBLE_MPI_UTILS__H__
#define SCRIBBLE__PABBLE_MPI_UTILS__H__
/**
 * Pabble-to-MPI conversion utils.
 */
#include <sesstype/st_node.h>
#include <sesstype/st_expr.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *pabble_mpi_version();
const char *pabble_mpi_gentime();
void st_node_subst_const(st_node *node, const char *name, unsigned int value);
void st_expr_subst_const(st_expr *expr, const char *name, unsigned int value);
unsigned int count_unbounded_const_role(const char *name, st_role *role_decl);

#ifdef __cplusplus
}
#endif

#endif // SCRIBBLE__MPI__PRINT__H__
