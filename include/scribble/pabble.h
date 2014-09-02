#ifndef PABBLE_H__
#define PABBLE_H__
#include <mpi.h>

typedef struct pabble_meta meta_t;

struct pabble_meta {
  int pid;
  int nprocs;
  MPI_Comm comm;
  // A function pointer for buffer length.
  unsigned int (*buflen)(int label);
};

/** Pabble send queue **/

void pabble_sendq_enqueue(int, void *);
void *pabble_sendq_dequeue();
int pabble_sendq_top_id();
int pabble_sendq_has_id(int);
int pabble_sendq_isempty();
int pabble_sendq_length();

/** Pabble recv queue **/

void pabble_recvq_enqueue(int, void *);
void *pabble_recvq_dequeue();
int pabble_recvq_top_id();
int pabble_recvq_has_id(int);
int pabble_recvq_isempty();
int pabble_recvq_length();

#endif // PABBLE_H__
