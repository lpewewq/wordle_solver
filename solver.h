#pragma once

#include "solver_utility.h"
#include "wordle.h"

typedef struct WordleBranch
{
    uint8_t score;
    struct WordleNode *node;
} WordleBranch;

typedef struct WordleNode
{
    size_t test_index;
    size_t num_branches;
    struct WordleBranch *branches;
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
