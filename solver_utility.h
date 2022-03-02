#pragma once
#include "wordle.h"
#include <stdbool.h>
#include <stdint.h>

#define N_BRANCHES 243

typedef struct tuple
{
    size_t index;
    double value;
} tuple;

typedef struct WordleSolverInstance
{
    const WordleInstance *wordle_instance;
    const size_t n_hidden;
    const size_t *hidden_vector;
    const size_t n_test;
    tuple *test_vector;
    const uint8_t **score_cache;
    const size_t depth;
} WordleSolverInstance;

typedef struct Branch
{
    size_t test_index;
    size_t count;
    tuple sizes[N_BRANCHES];
    size_t starts[N_BRANCHES];
    size_t *hidden_indicies;
} Branch;

bool sort_test_vector(const WordleSolverInstance *solver_instance, size_t *pruned_index);

void create_branches(const WordleSolverInstance *solver_instance, Branch *branch);

int compare_tuples(const void *a, const void *b);
