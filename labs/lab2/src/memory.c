#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"

void memory_init(Memory_State *m, Interconnect_State *i) {
  m->interconnect = i;
  m->pending_requests = list_new();
  m->ongoing_requests = list_new();
}

void memory_free(Memory_State *m) {
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(m->pending_requests, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    free(node->val);
    list_remove(m->pending_requests, node);
  }
  list_iterator_destroy(it);
  list_destroy(m->pending_requests);

  it = list_iterator_new(m->ongoing_requests, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    free(node->val);
    list_remove(m->ongoing_requests, node);
  }
  list_iterator_destroy(it);
  list_destroy(m->ongoing_requests);
}

void memory_add_request(Memory_State *m, Cache_Block *b) {
  Memory_Request *request = (Memory_Request *)malloc(sizeof(Memory_Request));
  request->cache_block = b;

  uint32_t tag = request->cache_block->tag;
  int bank_idx = (tag & 0xFF) >> 5;
  assert(bank_idx >= 0 && bank_idx < MEM_NUM_BANKS);

  request->bank = m->banks + bank_idx;
  request->bank_idx = bank_idx;

  request->row = tag & MEM_ROW_ADDRESS_MASK;

  list_lpush(m->pending_requests, list_node_new(request));
  debug_mem("[0x%X] memory request added\n", tag);
}

static enum Memory_Row_Buffer_Status memory_get_rb_status(Memory_Request *r) {
  if (!r->bank->row_buffer_open) {
    return MEM_ROW_BUFFER_MISS;
  }

  return r->row == r->bank->row_buffer ? MEM_ROW_BUFFER_HIT
                                       : MEM_ROW_BUFFER_CONFLICT;
}

static void memory_calculate_usages(Memory_Request *r, int curr_cycle) {
  Memory_Interval *precharge_int = r->cmd_ints + MEM_PRE_IDX;
  Memory_Interval *activate_int = r->cmd_ints + MEM_ACT_IDX;
  Memory_Interval *read_write_int = r->cmd_ints + MEM_RW_IDX;

  precharge_int->valid = false;
  activate_int->valid = false;

  /*
   * Assumption: bank is busy at first command and remains busy until request
   * is finished
   */
  r->bank_int.start = curr_cycle;
  /* use fall-through to calculate ressource usage */
  switch (r->status) {
  case MEM_ROW_BUFFER_CONFLICT:
    precharge_int->valid = true;
    precharge_int->start = curr_cycle;
    precharge_int->end = curr_cycle + 3;

    r->bank_int.end = curr_cycle + 99;

    curr_cycle = r->bank_int.end + 1;
  case MEM_ROW_BUFFER_MISS:
    activate_int->valid = true;
    activate_int->start = curr_cycle;
    activate_int->end = curr_cycle + 3;

    r->bank_int.end = curr_cycle + 99;

    curr_cycle = r->bank_int.end + 1;
  case MEM_ROW_BUFFER_HIT:
    read_write_int->start = curr_cycle;
    read_write_int->end = curr_cycle + 3;
    read_write_int->valid = true;

    r->bank_int.end = curr_cycle + 99;
    r->bank_int.valid = true;

    r->data_int.start = r->bank_int.end + 1;
    r->data_int.end = r->data_int.start + 49;
    r->data_int.valid = true;

    debug_mem("[0x%X] ", r->cache_block->tag);
    if (precharge_int->valid)
      debug_mem("PRECHARGE [%d,%d] ", precharge_int->start, precharge_int->end);
    if (activate_int->valid)
      debug_mem("ACTIVATE [%d,%d] ", activate_int->start, activate_int->end);
    debug_mem("READ/WRITE [%d,%d] DATA [%d,%d] BANK %d [%d,%d]\n",
              read_write_int->start, read_write_int->end, r->data_int.start,
              r->data_int.end, r->bank_idx, r->bank_int.start, r->bank_int.end);
  }
}

static bool memory_intervals_overlap(Memory_Interval a, Memory_Interval b) {
  assert(a.start <= a.end && b.start <= b.end);
  return a.start <= b.end && b.start <= a.end;
}

static bool memory_cmd_conflict(Memory_Request *ongoing_r, Memory_Request *r) {
  for (int i = 0; i < MEM_NUM_CMD_INTERVALS; ++i) {
    Memory_Interval ongoing_int = ongoing_r->cmd_ints[i];
    if (!ongoing_int.valid) {
      continue;
    }
    for (int j = 0; j < MEM_NUM_CMD_INTERVALS; ++j) {
      Memory_Interval curr_int = r->cmd_ints[j];
      if (curr_int.valid && memory_intervals_overlap(ongoing_int, curr_int)) {
        return true;
      }
    }
  }
  return false;
}

static bool memory_data_conflict(Memory_Request *ongoing_r, Memory_Request *r) {
  return memory_intervals_overlap(ongoing_r->data_int, r->data_int);
}

static bool memory_bank_conflict(Memory_Request *ongoing_r, Memory_Request *r) {
  return ongoing_r->bank_idx == r->bank_idx &&
         memory_intervals_overlap(ongoing_r->bank_int, r->bank_int);
}

/*
 * Check if request is scheduable i.e. a candidate for scheduling.
 *
 * Go through each ongoing request and check if conflict on cmd/addr bus, data
 * bus or bank. If no conflict was found return true.
 */
static bool memory_is_candidate(Memory_State *m, Memory_Request *r) {
  bool result = true;
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(m->ongoing_requests, LIST_TAIL);
  while ((node = list_iterator_next(it))) {
    Memory_Request *ongoing_r = (Memory_Request *)node->val;
    if (memory_cmd_conflict(ongoing_r, r) ||
        memory_data_conflict(ongoing_r, r) ||
        memory_bank_conflict(ongoing_r, r)) {
      result = false;
      goto out;
    }
  }

out:
  list_iterator_destroy(it);
  return result;
}

void memory_cycle(Memory_State *m) {
  list_node_t *node;
  list_iterator_t *it;

  /* process ongoing requests */
  it = list_iterator_new(m->ongoing_requests, LIST_TAIL);
  while ((node = list_iterator_next(it))) {
    Memory_Request *r = (Memory_Request *)node->val;

    if (r->data_int.end <= m->curr_cycle) {
      interconnect_mem_to_l2(m->interconnect, r->cache_block);
      free(r);
      list_remove(m->ongoing_requests, node);
    }
  }
  list_iterator_destroy(it);

  /* find request to schedule */
  list_node_t *best_request_node = NULL;
  /* pending requests are traversed from oldest to newst */
  it = list_iterator_new(m->pending_requests, LIST_TAIL);
  while ((node = list_iterator_next(it))) {
    Memory_Request *request = (Memory_Request *)node->val;
    request->status = memory_get_rb_status(request);

    memory_calculate_usages(request, m->curr_cycle);
    if (memory_is_candidate(m, request)) {
      /* prioritize row hits */
      if (request->status == MEM_ROW_BUFFER_HIT) {
        best_request_node = node;
        break;
      }

      /* if no row hit pick oldest pending request */
      if (best_request_node == NULL) {
        best_request_node = node;
      }
    }
  }

  if (best_request_node != NULL) {
    Memory_Request *r = (Memory_Request *)best_request_node->val;
    debug_mem("scheduled request for 0x%X in cycle %d\n", r->cache_block->tag,
              m->curr_cycle);
    list_lpush(m->ongoing_requests, list_node_new(r));
    list_remove(m->pending_requests, best_request_node);
  }
  list_iterator_destroy(it);

  m->curr_cycle++;
}

