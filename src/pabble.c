#include <stdio.h>
#include "scribble/pabble.h"

#define PABBLE_QUEUE_SIZE 10

typedef struct {
  int id;
  void *item;
} pabble_queue_t;

static pabble_queue_t send_queue[PABBLE_QUEUE_SIZE];
static unsigned int send_queue_front = 0; // Next index to pop
static unsigned int send_queue_back = 0; // Next index to add element
static unsigned int send_queue_length = 0;

static pabble_queue_t recv_queue[PABBLE_QUEUE_SIZE];
static unsigned int recv_queue_front = 0; // Next index to pop
static unsigned int recv_queue_back = 0; // Next index to add element
static unsigned int recv_queue_length = 0;

extern meta_t meta;

void pabble_sendq_enqueue(int id, void *item)
{
  send_queue[send_queue_back].id = id;
  send_queue[send_queue_back].item = item;
#ifdef __DEBUG__
  fprintf(stderr, "[%d/%d] SEND enqueue {id: %d} len: %u front: %u back: %u => len: %d front: %u back: %u\n",
      meta.pid, meta.nprocs, id,
      send_queue_length, send_queue_front, send_queue_back,
      send_queue_length+1, send_queue_front, (send_queue_back+1) % PABBLE_QUEUE_SIZE);
#endif
  send_queue_back = (send_queue_back+1) % PABBLE_QUEUE_SIZE;
  send_queue_length++;
}

void *pabble_sendq_dequeue()
{
  if (pabble_sendq_isempty()) {
    fprintf(stderr, "Error: Send queue is empty! %u = %u\n", send_queue_front, send_queue_back);
    return NULL;
  }
#ifdef __DEBUG__
  fprintf(stderr, "[%d/%d] SEND dequeue {id: %d} len: %u front: %u back: %u => len: %d front: %u back: %u\n",
      meta.pid, meta.nprocs, send_queue[send_queue_front].id,
      send_queue_length, send_queue_front, send_queue_back,
      send_queue_length-1, (send_queue_front+1) % PABBLE_QUEUE_SIZE, send_queue_back);
#endif
  void *item = send_queue[send_queue_front].item;
  send_queue_front = (send_queue_front+1) % PABBLE_QUEUE_SIZE;
  send_queue_length--;
  return item;
}

int pabble_sendq_top_id()
{
  if (send_queue_length == 0) return -1;
  return send_queue[send_queue_front].id;
}

int pabble_sendq_length()
{
  return send_queue_length;
}

int pabble_sendq_isempty()
{
  return send_queue_front == send_queue_back;
}

void pabble_recvq_enqueue(int id, void *item)
{
  recv_queue[recv_queue_back].id = id;
  recv_queue[recv_queue_back].item = item;
#ifdef __DEBUG__
  fprintf(stderr, "[%d/%d] RECV enqueue {id: %d} len: %u front: %u back: %u => len: %d front: %u back: %u\n",
      meta.pid, meta.nprocs, id,
      recv_queue_length, recv_queue_front, recv_queue_back,
      recv_queue_length+1, recv_queue_front, (recv_queue_back+1) % PABBLE_QUEUE_SIZE);
#endif
  recv_queue_back = (recv_queue_back+1) % PABBLE_QUEUE_SIZE;
  recv_queue_length++;
}

void *pabble_recvq_dequeue()
{
  if (pabble_recvq_isempty()) {
    fprintf(stderr, "Error: Recv queue is empty!\n");
    return NULL;
  }
#ifdef __DEBUG__
  fprintf(stderr, "[%d/%d] RECV dequeue {id: %d} len: %u front: %u back: %u => len: %d front: %u back: %u\n",
      meta.pid, meta.nprocs, recv_queue[recv_queue_front].id,
      recv_queue_length, recv_queue_front, recv_queue_back,
      recv_queue_length-1, (recv_queue_front+1) % PABBLE_QUEUE_SIZE, recv_queue_back);
#endif
  void *item = recv_queue[recv_queue_front].item;
  recv_queue_front = (recv_queue_front+1) % PABBLE_QUEUE_SIZE;
  recv_queue_length--;
  return item;
}

int pabble_recvq_top_id()
{
  if (recv_queue_length == 0) return -1;
  return recv_queue[recv_queue_front].id;
}

int pabble_recvq_length()
{
  return recv_queue_length;

}

int pabble_recvq_isempty()
{
  return recv_queue_front == recv_queue_back;
}
