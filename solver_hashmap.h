#pragma once

#include "solver_utility.h"
#include <hashmap.h>
#include <stdbool.h>
#include <stdlib.h>

#define KEY_SIZE 2315

typedef bool hasmap_key_t[KEY_SIZE];

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
    // hashmap key
    hasmap_key_t *key;
} WordleNode;

void solver_hashmap_init();

hasmap_key_t *get_key(const WordleSolverInstance *solver_instance);

WordleNode *solver_hashmap_get(const hasmap_key_t *key);

void solver_hashmap_put(const hasmap_key_t *key, WordleNode *node);

void solver_hashmap_cleanup();
