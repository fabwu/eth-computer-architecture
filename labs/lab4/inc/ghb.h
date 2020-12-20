#ifndef GHB_H
#define GHB_H

#define IT_SIZE 256
#define GHB_SIZE 256
#define NUM_LAST_ENTRIES 3

typedef struct IT_Entry IT_Entry;
typedef struct GHB_Entry GHB_Entry;

struct IT_Entry {
    uint64_t ip;
    GHB_Entry *ghb_ptr;
};

struct GHB_Entry {
    uint64_t addr;
    GHB_Entry *prev;
    GHB_Entry *next;
    IT_Entry *it;
};

class GHB {
  public:
    void operate(uint64_t addr, uint64_t ip, std::vector<uint64_t> *pf_addr_list);

    GHB() {
        it_head_idx = 0;
        ghb_head_idx = 0;
        prefetch_distance = 4;
        prefetch_degree = 4;
    }

  private:
    uint64_t prefetch_distance;
    uint64_t prefetch_degree;

    IT_Entry it[IT_SIZE];
    int it_head_idx;

    GHB_Entry ghb[GHB_SIZE];
    int ghb_head_idx;
};

void GHB::operate(uint64_t addr, uint64_t ip, std::vector<uint64_t> *pf_addr_list) {
    IT_Entry *it_entry = NULL;

    // use linear search for now
    for(int i = 0; i < IT_SIZE; i++) {
        IT_Entry *curr = it + i;
        if(curr->ip == ip) {
            it_entry = curr;
            break;
        }
    }

    if (it_entry == NULL) {
        // it not in index table -> allocate new IT entry
        it_entry = it + it_head_idx;
        it_entry->ip = ip;
        it_entry->ghb_ptr = NULL;

        it_head_idx = (it_head_idx + 1) % IT_SIZE;
    }

    GHB_Entry *ghb_entry = ghb + ghb_head_idx;

    // invalidate prev of more recent entry
    if (ghb_entry->next) {
        ghb_entry->next->prev = NULL;
    }

    // invalidate IT entry
    if (ghb_entry->it) {
        ghb_entry->it->ghb_ptr = NULL;
    }

    uint64_t cl_addr = addr >> LOG2_BLOCK_SIZE;

    // insert new entry into GHB
    ghb_entry->addr = cl_addr;
    ghb_entry->prev = it_entry->ghb_ptr;
    ghb_entry->it = it_entry;
    it_entry->ghb_ptr = ghb_entry;

    // update pointers of previous entry
    if (ghb_entry->prev) {
        ghb_entry->prev->next = ghb_entry;
        ghb_entry->prev->it = NULL;
    }

    ghb_head_idx = (ghb_head_idx + 1) % GHB_SIZE;

    // find stride
    pf_addr_list->clear();
    GHB_Entry last_entries[NUM_LAST_ENTRIES];
    GHB_Entry *curr = ghb_entry;
    int num_entries = 0;
    for(; curr != NULL && num_entries < NUM_LAST_ENTRIES; num_entries++) {
        last_entries[num_entries] = *curr;
        curr = curr->prev;
    }

    if (num_entries == NUM_LAST_ENTRIES) {
        int64_t stride[NUM_LAST_ENTRIES - 1];

        for (int i = 0; i < NUM_LAST_ENTRIES - 1; i++) {
            if (last_entries[i].addr > last_entries[i+1].addr) {
                stride[i] = last_entries[i].addr - last_entries[i+1].addr;
            } else {
                stride[i] = last_entries[i+1].addr - last_entries[i].addr;
                stride[i] *= -1;
            }
        }

        bool stride_detected = false;
        for (int i = 0; i < NUM_LAST_ENTRIES - 1; i++) {
            if (stride[i] == 0) {
                // abort if we see same address twice
                return;
            }

            for (int j = i + 1; j < NUM_LAST_ENTRIES - 1; j++) {
                stride_detected = (stride[i] == stride[j]);
            }
        }

        if (stride_detected) {
            for (uint64_t i = 0; i < prefetch_degree; i++) {
                uint64_t pf_addr =
                    (cl_addr + (stride[0] * (prefetch_distance + i)))
                    << LOG2_BLOCK_SIZE;

                // only issue a prefetch if the prefetch address is in the same 4
                // KB page as the current demand access address
                if ((pf_addr >> LOG2_PAGE_SIZE) != (addr >> LOG2_PAGE_SIZE))
                    break;

                pf_addr_list->push_back(pf_addr);
            }
        }
    }
}

#endif
