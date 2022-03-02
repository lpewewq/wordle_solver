#include "solver_utility.h"
#include <math.h>
#include <string.h>

int compare_tuples(const void *a, const void *b)
{
    // descending order
    tuple arg1 = *(const tuple *)a;
    tuple arg2 = *(const tuple *)b;

    if (arg1.value < arg2.value)
        return 1;
    if (arg1.value > arg2.value)
        return -1;
    return 0;
}

double unnormalized_entropy(const size_t *branch_sizes, const size_t n_hidden)
{
    double result = 0;
    for (size_t i = 0; i < N_BRANCHES; i++)
    {
        double branch_size = branch_sizes[i];
        if (branch_size > 0)
        {
            result += branch_size * log2(n_hidden / branch_size);
        }
    }
    return result;
}

bool sort_test_vector(const WordleSolverInstance *solver_instance, size_t *pruned_index)
{
    size_t pruned_index_non_hidden = UINTMAX_MAX;
    for (size_t i = 0; i < solver_instance->n_test; i++)
    {
        bool prune = true;
        size_t branch_sizes[N_BRANCHES] = {0};
        for (size_t j = 0; j < solver_instance->n_hidden; j++)
        {
            size_t score = solver_instance->score_cache[solver_instance->test_vector[i].index][solver_instance->hidden_vector[j]];
            branch_sizes[score] += 1;
            prune &= branch_sizes[score] == 1;
        }
        if (prune)
        {
            if (branch_sizes[N_BRANCHES - 1] == 1)
            {
                // directly prune on hidden word
                *pruned_index = solver_instance->test_vector[i].index;
                return prune;
            }
            else
            {
                // maybe prune on non hidden word later
                pruned_index_non_hidden = solver_instance->test_vector[i].index;
            }
        }
        solver_instance->test_vector[i].value = unnormalized_entropy(branch_sizes, solver_instance->n_hidden);
    }

    if (pruned_index_non_hidden != UINTMAX_MAX)
    {
        *pruned_index = pruned_index_non_hidden;
        return true;
    }

    qsort(solver_instance->test_vector, solver_instance->n_test, sizeof(tuple), compare_tuples);
    return false;
}

void create_branches(const WordleSolverInstance *solver_instance, Branch *branch)
{
    size_t test_index = branch->test_index;
    memset(branch->sizes, 0, sizeof(branch->sizes));
    branch->count = 0;
    // count branch sizes
    for (size_t j = 0; j < solver_instance->n_hidden; j++)
    {
        size_t score = solver_instance->score_cache[test_index][solver_instance->hidden_vector[j]];
        branch->sizes[score].index = score;
        branch->sizes[score].value++;
    }
    // set branch starts
    for (size_t j = 1; j < N_BRANCHES; j++)
    {
        branch->starts[j] = branch->starts[j - 1] + branch->sizes[j - 1].value;
    }
    // write partitioned indices
    for (size_t j = 0; j < solver_instance->n_hidden; j++)
    {
        size_t score = solver_instance->score_cache[test_index][solver_instance->hidden_vector[j]];
        branch->hidden_indicies[branch->starts[score]] = solver_instance->hidden_vector[j];
        branch->starts[score]++;
    }
    // correct branch starts & count nonzero branches
    for (size_t j = 0; j < N_BRANCHES; j++)
    {
        branch->starts[j] -= branch->sizes[j].value;
        if (branch->sizes[j].value > 0 && j != N_BRANCHES - 1) // ignore GGGGG (242)
        {
            branch->count++;
        }
    }
}
