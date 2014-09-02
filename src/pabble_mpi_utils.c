#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sesstype/st_node.h>
#include <sesstype/st_expr.h>

#include "scribble/pabble_mpi_utils.h"

const char *pabble_mpi_gentime()
{
  time_t current_time = time(NULL);
  if (current_time == ((time_t)-1)) return "";
  return ctime(&current_time);
}

unsigned int count_unbounded_const_role(const char *name, st_role *role_decl)
{
  int cnt = 0;
  for (int p=0; p<role_decl->dimen; p++)
    if (role_decl->param[p]->type == ST_EXPR_TYPE_RNG && role_decl->param[p]->rng->to->type == ST_EXPR_TYPE_VAR)
      cnt++;
  return cnt;
}

void st_expr_subst_const(st_expr *expr, const char *name, unsigned int value)
{
  assert(expr != NULL);

  switch (expr->type) {
    case ST_EXPR_TYPE_CONST:
      break;
    case ST_EXPR_TYPE_VAR:
      if (strcmp(expr->var, name) == 0) {
        expr->type = ST_EXPR_TYPE_CONST;
        expr->num = value;
      }
      break;
    case ST_EXPR_TYPE_RNG:
      st_expr_subst_const(expr->rng->from, name, value);
      st_expr_subst_const(expr->rng->to, name, value);
      break;
    case ST_EXPR_TYPE_ADD:
    case ST_EXPR_TYPE_SUB:
    case ST_EXPR_TYPE_MUL:
    case ST_EXPR_TYPE_DIV:
    case ST_EXPR_TYPE_MOD:
    case ST_EXPR_TYPE_SHL:
    case ST_EXPR_TYPE_SHR:
      st_expr_subst_const(expr->bin->left, name, value);
      st_expr_subst_const(expr->bin->right, name, value);
      break;
    case ST_EXPR_TYPE_SEQ:
      break;
    default:
      fprintf(stderr, "%s:%d %s Unknown expr type: %d\n", __FILE__, __LINE__, __FUNCTION__, expr->type);
  }
}

/**
 * \brief Substitute constant name with value.
 */
void st_node_subst_const(st_node *node, const char *name, unsigned int value)
{
  assert(node != NULL);

  // Check participant parameters and payload type parameters.
  switch (node->type) {
    case ST_NODE_ROOT:
      break;
    case ST_NODE_SENDRECV:
      for (int p=0; p<node->interaction->from->dimen; p++)
        st_expr_subst_const(node->interaction->from->param[p], name, value);
      for (int t=0; t<node->interaction->nto; t++)
        for (int p=0; p<node->interaction->to[t]->dimen; p++)
          st_expr_subst_const(node->interaction->to[t]->param[p], name, value);
      for (int pl=0; pl<node->interaction->msgsig.npayload; pl++)
        st_expr_subst_const(node->interaction->msgsig.payloads[pl].expr, name, value);
      break;
    case ST_NODE_RECV:
      if (node->interaction->msg_cond != NULL)
        for (int p=0; p<node->interaction->msg_cond->dimen; p++)
          st_expr_subst_const(node->interaction->msg_cond->param[p], name, value);
      for (int p=0; p<node->interaction->from->dimen; p++)
        st_expr_subst_const(node->interaction->from->param[p], name, value);
      for (int pl=0; pl<node->interaction->msgsig.npayload; pl++)
        if (node->interaction->msgsig.payloads[pl].expr != NULL)
          st_expr_subst_const(node->interaction->msgsig.payloads[pl].expr, name, value);
      break;
    case ST_NODE_SEND:
      if (node->interaction->msg_cond != NULL)
        for (int p=0; p<node->interaction->msg_cond->dimen; p++)
          st_expr_subst_const(node->interaction->msg_cond->param[p], name, value);
      for (int t=0; t<node->interaction->nto; t++)
        for (int p=0; p<node->interaction->to[t]->dimen; p++)
          st_expr_subst_const(node->interaction->to[t]->param[p], name, value);
      for (int pl=0; pl<node->interaction->msgsig.npayload; pl++)
        if (node->interaction->msgsig.payloads[pl].expr != NULL)
          st_expr_subst_const(node->interaction->msgsig.payloads[pl].expr, name, value);
      break;
    case ST_NODE_CHOICE:
      for (int p=0; p<node->choice->at->dimen; p++)
        st_expr_subst_const(node->choice->at->param[p], name, value);
      break;
    case ST_NODE_PARALLEL:
      break;
    case ST_NODE_RECUR:
      break;
    case ST_NODE_CONTINUE:
      break;
    case ST_NODE_FOR:
      st_expr_subst_const(node->forloop->range->from, name, value);
      st_expr_subst_const(node->forloop->range->to, name, value);
      break;
#ifdef PABBLE_DYNAMIC
    case ST_NODE_ONEOF:
      st_expr_subst(node->oneof->range->from);
      st_expr_subst(node->oneof->range->to);
      break;
#endif
    case ST_NODE_ALLREDUCE:
      break;
    case ST_NODE_IFBLK:
      for (int c=0; c<node->ifblk->cond->dimen; c++)
        st_expr_subst_const(node->ifblk->cond->param[c], name, value);
      break;
    default:
      fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
  }
  for (int child=0; child<node->nchild; child++) {
    st_node_subst_const(node->children[child], name, value);
  }
}
