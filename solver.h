#pragma once

#include "solver_utility.h"
#include "wordle.h"

typedef struct WordleNode
{
    size_t test_index;
    uint8_t score; // irrelevant for root node
    size_t num_branches;
    struct WordleNode **branches;

    // stats
    float duration;
    size_t n_hidden;
    size_t n_test;
    size_t total;
    size_t best_case;
    size_t worst_case;
    float average_case;
} WordleNode;

WordleNode *optimize_decision_tree(const WordleInstance *wordle_instance);
