#ifndef GHB_H
#define GHB_H

/*
 * This class implements a stride prefetcher using a global history buffer
 * as described in the paper 'Data cache prefetching using a global history
 * buffer' by Nesbit and Smith.
 *
 * For Task 1 it is not necessary to change the aggressivness of the prefetcher
 * so we initialize the prefetch distance and degree to 4 as required by the
 * task description.
 *
 * For Task 2 the aggressivness i.e. the prefetch distance and degree is
 * adjusted at runtime using the method set_aggressiveness().
 */

#define IT_SIZE 256
#define IT_INDEX_MASK (IT_SIZE - 1)
#define GHB_SIZE 256
#define NUM_LAST_ENTRIES 3

typedef struct IT_Entry IT_Entry;
typedef struct GHB_Entry GHB_Entry;

// entry of the index table
struct IT_Entry {
    uint64_t ip;
    GHB_Entry *ghb_ptr;
};

// entry of the global history buffer
struct GHB_Entry {
    uint64_t addr;
    GHB_Entry *prev;
    GHB_Entry *next;
    IT_Entry *it;
};

class GHB {
  public:
    enum aggressivness_t {
        VERY_CONSERVATIVE,
        CONSERVATIVE,
        MIDDLE_OF_THE_ROAD,
        AGGRESSIVE,
        VERY_AGGRESSIVE,

        NUM_AGGRESSIVNESS
    };

    void operate(uint64_t addr, uint64_t ip, std::vector<uint64_t> *pf_addr_list);
    void set_aggressiveness(aggressivness_t aggressivness);

    GHB() {
        // init ghb head
        ghb_head_idx = 0;
        // init distance and degree to 4 so Task 1 still works
        prefetch_distance = 4;
        prefetch_degree = 4;
        aggressivness = MIDDLE_OF_THE_ROAD;

        // init prefetcher configuration
        pf_distance_table[VERY_CONSERVATIVE] = 4;
        pf_distance_table[CONSERVATIVE] = 8;
        pf_distance_table[MIDDLE_OF_THE_ROAD] = 16;
        pf_distance_table[AGGRESSIVE] = 32;
        pf_distance_table[VERY_AGGRESSIVE] = 48;

        pf_degree_table[VERY_CONSERVATIVE] = 1;
        pf_degree_table[CONSERVATIVE] = 1;
        pf_degree_table[MIDDLE_OF_THE_ROAD] = 2;
        pf_degree_table[AGGRESSIVE] = 4;
        pf_degree_table[VERY_AGGRESSIVE] = 4;
    }

  private:
    // aggressivness set by set_aggressiveness()
    aggressivness_t aggressivness;
    uint64_t prefetch_distance;
    uint64_t prefetch_degree;

    // index table
    IT_Entry it[IT_SIZE];

    // global history buffer
    GHB_Entry ghb[GHB_SIZE];
    int ghb_head_idx;

    // table that encodes prefetcher configuration
    int8_t pf_distance_table[NUM_AGGRESSIVNESS];
    int8_t pf_degree_table[NUM_AGGRESSIVNESS];
};

/*
 * This method updates the GHB and returns a list of suitable prefetch addresses
 * for the given instruction pointer.
 */
void GHB::operate(uint64_t addr, uint64_t ip, std::vector<uint64_t> *pf_addr_list) {
    uint64_t it_idx = IT_INDEX_MASK & ip;
    uint64_t cl_addr = addr >> LOG2_BLOCK_SIZE;

    // get entry from index table for given instruction pointer
    IT_Entry *it_entry = it + it_idx;

    // reset index table entry if the instruction pointers don't match
    if (it_entry->ip != ip) {
        it_entry->ip = ip;
        it_entry->ghb_ptr = NULL;
    }

    GHB_Entry *ghb_entry = ghb + ghb_head_idx;

    // invalidate prev of more recent GHB entry
    if (ghb_entry->next) {
        ghb_entry->next->prev = NULL;
    }

    // invalidate IT entry
    if (ghb_entry->it) {
        ghb_entry->it->ghb_ptr = NULL;
    }

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

    // increment GHB head
    ghb_head_idx = (ghb_head_idx + 1) % GHB_SIZE;

    // calculate prefetch addresses
    pf_addr_list->clear();
    GHB_Entry last_entries[NUM_LAST_ENTRIES];
    GHB_Entry *curr = ghb_entry;

    // traverse the GHB and find predecessors
    int num_entries = 0;
    for(; curr != NULL && num_entries < NUM_LAST_ENTRIES; num_entries++) {
        last_entries[num_entries] = *curr;
        curr = curr->prev;
    }

    if (num_entries == NUM_LAST_ENTRIES) {
        int64_t stride[NUM_LAST_ENTRIES - 1];

        // calculate strides between last entries
        for (int i = 0; i < NUM_LAST_ENTRIES - 1; i++) {
            if (last_entries[i].addr > last_entries[i+1].addr) {
                stride[i] = last_entries[i].addr - last_entries[i+1].addr;
            } else {
                stride[i] = last_entries[i+1].addr - last_entries[i].addr;
                stride[i] *= -1;
            }
        }

        // detect stride
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
            // prefetch addresses based on prefetch degree and distance
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

/*
 * Changes the aggressviness of the prefetcher i.e. increase/decrease prefetch
 * distance/degree
 */
void GHB::set_aggressiveness(aggressivness_t a) {
    // if (aggressivness != a) {
    //     cout << "aggressivness changed from " << aggressivness << " to " << a << endl;
    // }
    aggressivness = a;
    prefetch_distance = pf_distance_table[aggressivness];
    prefetch_degree = pf_degree_table[aggressivness];
}

#endif
