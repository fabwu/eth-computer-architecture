#include <iostream>
#include <cassert>

typedef struct IT_Entry IT_Entry;

struct GHB_Entry {
    uint64_t addr;
    GHB_Entry *prev_ptr;
    GHB_Entry *next_ptr;
    IT_Entry *it_ptr;
};

#define GHB_SIZE 4
GHB_Entry ghb[GHB_SIZE];
uint64_t ghb_head_idx = 0;

struct IT_Entry {
    uint64_t pc;
    GHB_Entry *ghb_ptr;
};

#define IT_SIZE 4
IT_Entry it[IT_SIZE];
uint64_t it_head_idx = 0;

void IT_check(uint64_t pc, uint64_t addr) {
    IT_Entry *it_entry = NULL;

    // use linear search for now
    for(int i = 0; i < IT_SIZE; i++) {
        IT_Entry *curr = it + i;
        if(curr->pc == pc) {
            it_entry = curr;
            break;
        }
    }

    // pc not in index table
    if (it_entry == NULL) {
        it_entry = it + it_head_idx;
        it_head_idx = (it_head_idx + 1) % IT_SIZE;
        it_entry->pc = pc;
        it_entry->ghb_ptr = NULL;
    }

    GHB_Entry *ghb_entry = ghb + ghb_head_idx;

    // invalidate prev_ptr of newer entry
    if (ghb_entry->next_ptr) {
        ghb_entry->next_ptr->prev_ptr = NULL;
    }

    // invalidate IT entry
    if (ghb_entry->it_ptr) {
        ghb_entry->it_ptr->ghb_ptr = NULL;
    }

    // insert new entry into GHB
    ghb_entry->addr = addr;
    ghb_entry->prev_ptr = it_entry->ghb_ptr;
    ghb_entry->it_ptr = it_entry;
    it_entry->ghb_ptr = ghb_entry;

    // update pointers of previous entry
    if (ghb_entry->prev_ptr) {
        ghb_entry->prev_ptr->next_ptr = ghb_entry;
        ghb_entry->prev_ptr->it_ptr = NULL;
    }

    ghb_head_idx = (ghb_head_idx + 1) % GHB_SIZE;

    // find stride
    int num_last_entries = 3;
    GHB_Entry last_entries[num_last_entries];
    GHB_Entry *curr = ghb_entry;
    int i = 0;
    for(; curr != NULL && i < num_last_entries; i++) {
        last_entries[i] = *curr;
        curr = curr->prev_ptr;
    }

    if (i == num_last_entries) {
        int64_t stride_1 = last_entries[0].addr - last_entries[1].addr;
        int64_t stride_2 = last_entries[1].addr - last_entries[2].addr;

        if (stride_1 > 0 && stride_2 > 0 && stride_1 == stride_2) {
            printf("Stride: %ld %ld\n", stride_1, stride_2);
        }
    }

    //TODO Move implementation to l2c_pref
}


int main() {
    IT_check(1, 1);
    IT_check(1, 2);
    IT_check(1, 4);
    IT_check(1, 6);
    IT_check(1, 8);
    IT_check(2, 6);

    for (int i = 0; i < IT_SIZE; i++) {
        std::cout << it[i].pc << ": ";
        GHB_Entry *curr = it[i].ghb_ptr;
        while(curr) {
            std::cout << curr->addr << " -> ";
            curr = curr->prev_ptr;
        }
        std::cout << std::endl;
    }
}
