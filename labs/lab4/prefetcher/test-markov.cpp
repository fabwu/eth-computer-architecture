#include <iostream>
#include <cassert>
#include <string.h>

//TODO remove
#define LOG2_BLOCK_SIZE 6

#define NUM_MARKOV_NODES 8
#define NUM_MARKOV_TRANSISTIONS 4

#define MARKOV_TABLE_IDX_MASK (NUM_MARKOV_NODES - 1)

struct Prediction {
    uint64_t cl_addr;
    uint64_t priority;
};

struct Tag {
    uint64_t cl_addr;
    Prediction predictions[NUM_MARKOV_TRANSISTIONS];
    uint64_t next_pred;
    uint64_t num_matches;
};

Tag markov_table[NUM_MARKOV_NODES];
Tag *last_tag = NULL;

void update_model(uint64_t addr) {
    uint64_t cl_addr = addr >> LOG2_BLOCK_SIZE;
    uint64_t table_idx = MARKOV_TABLE_IDX_MASK & cl_addr;

    if (last_tag) {
        // increase counter for how many times this tag was predecessor to calculate propability
        last_tag->num_matches++;

        bool existed = false;
        for (int i = 0; i < NUM_MARKOV_TRANSISTIONS; i++) {
            Prediction *p = last_tag->predictions + i;
            if (p->cl_addr == cl_addr) {
                // if exists increase occurance counter
                p->priority++;
                existed = true;
                break;
            }
        }

        if (!existed) {
            // if not exists insert into prediction table
            Prediction *next = last_tag->predictions + last_tag->next_pred;
            // remove matches for evicted prediction
            last_tag->num_matches -= next->priority;
            // insert new prediction
            next->cl_addr = cl_addr;
            next->priority = 1;
            last_tag->next_pred = (last_tag->next_pred + 1) % 4;
        }
    }

    Tag *current_tag = markov_table + table_idx;
    if (current_tag->cl_addr != cl_addr) {
        // evict old tag
        current_tag->cl_addr = cl_addr;
        memset(current_tag->predictions, 0, sizeof(current_tag->predictions));
    }
    last_tag = current_tag;
}

int main() {
    std::string seq = "ABCDCEACFFEAABCDEABCDC";

    for (char addr : seq) {
        uint64_t cl_addr = (addr - 'A' + 1) << LOG2_BLOCK_SIZE;
        update_model(cl_addr);
    }

    for (Tag tag : markov_table) {
        std::cout << tag.cl_addr << ": ";
        if (tag.num_matches == 0) {
            std::cout << "-" << std::endl;
            continue;
        }
        for (Prediction p : tag.predictions) {
            if (p.cl_addr == 0) {
                std::cout << " - ";
            } else {
                std::cout << p.cl_addr << "/" << p.priority / (float)tag.num_matches << " ";
            }
        }
        std::cout << std::endl;
    }
}
