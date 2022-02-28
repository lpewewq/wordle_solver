#include "solver.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define LOG_DEPTH 0
#define SEARCH_DEPTH 44 // 44 -> salet 7920

const uint8_t POWERS_OF_THREE[6] = {1, 3, 9, 27, 81, 243};
const char *WORDLE_EMOJIS[3] = {"⬛", "🟨", "🟩"};

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
    size_t sizes[N_BRANCHES];
    size_t starts[N_BRANCHES];
    size_t *hidden_indicies;
} Branch;

void free_tree(WordleNode *node, bool free_root)
{
    if (node == NULL)
    {
        return;
    }
    for (int j = node->num_branches - 1; j >= 0; j--)
    {
        free_tree(node->branches + j, j == 0);
    }
    if (free_root)
    {
        free(node);
    }
}

size_t tree_max_depth(WordleNode *node)
{
    if (node == NULL)
    {
        return 0;
    }
    size_t max_depth = 0;
    for (size_t j = 0; j < node->num_branches; j++)
    {
        size_t depth = tree_max_depth(node->branches + j);
        if (depth > max_depth)
        {
            max_depth = depth;
        }
    }
    return 1 + max_depth;
}

int compare_tuples_desc(const void *a, const void *b)
{
    tuple arg1 = *(const tuple *)a;
    tuple arg2 = *(const tuple *)b;

    if (arg1.value < arg2.value)
        return 1;
    if (arg1.value > arg2.value)
        return -1;
    return 0;
}

void descore(uint8_t score, char *output)
{
    // output will be a at most 5*5=25 chars long string
    output[0] = '\0'; // ignore string content
    uint8_t div = score;
    for (uint8_t i = 0; i < 5; i++)
    {
        strncat(output, WORDLE_EMOJIS[div % 3], 5);
        div /= 3;
    }
}

uint8_t score(const char *test_word, const char *hidden_word)
{
    uint8_t result = 0;
    bool crossed[5] = {false, false, false, false, false};

    // look for exact matches (green)
    for (uint8_t i = 0; i < 5; i++)
    {
        if (test_word[i] == hidden_word[i])
        {
            result += 2 * POWERS_OF_THREE[i];
            crossed[i] = true;
        }
    }

    // look for non-exact matches (yellow)
    for (uint8_t i = 0; i < 5; i++)
    {
        if (test_word[i] == hidden_word[i])
        {
            continue;
        }

        for (uint8_t j = 0; j < 5; j++)
        {
            if (test_word[i] == hidden_word[j] && !crossed[j])
            {
                result += POWERS_OF_THREE[i];
                crossed[j] = true;
                break;
            }
        }
    }
    return result;
}

double entropy(const size_t *branch_sizes, const size_t n_hidden)
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
    return result / n_hidden;
}

size_t sort_test_vector(const WordleSolverInstance *solver_instance, size_t *pruned_instance)
{
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
        if (prune && branch_sizes[N_BRANCHES - 1] == 1)
        {
            *pruned_instance = solver_instance->test_vector[i].index;
            return 2 * solver_instance->n_hidden - 1;
        }
        solver_instance->test_vector[i].value = entropy(branch_sizes, solver_instance->n_hidden);
    }

    qsort(solver_instance->test_vector, solver_instance->n_test, sizeof(tuple), compare_tuples_desc);
    return 0;
}

void create_branches(const WordleSolverInstance *solver_instance, Branch *branch)
{
    size_t test_index = branch->test_index;
    memset(branch->sizes, 0, sizeof branch->sizes);
    branch->count = 0;
    for (size_t j = 0; j < solver_instance->n_hidden; j++)
    {
        size_t score = solver_instance->score_cache[test_index][solver_instance->hidden_vector[j]];
        branch->sizes[score]++;
    }
    for (size_t j = 1; j < N_BRANCHES; j++)
    {
        branch->starts[j] = branch->starts[j - 1] + branch->sizes[j - 1];
    }
    for (size_t j = 0; j < solver_instance->n_hidden; j++)
    {
        size_t score = solver_instance->score_cache[test_index][solver_instance->hidden_vector[j]];
        branch->hidden_indicies[branch->starts[score]] = solver_instance->hidden_vector[j];
        branch->starts[score]++;
    }
    for (size_t j = 0; j < N_BRANCHES; j++)
    {
        branch->starts[j] -= branch->sizes[j];
        if (branch->sizes[j] > 0 && j != N_BRANCHES - 1) // ignore GGGGG
        {
            branch->count++;
        }
    }
}

size_t min_try_sum(const WordleSolverInstance *solver_instance, const size_t beta, WordleNode *node);
size_t branch_try_sum(const WordleSolverInstance *solver_instance, const Branch *branch, const size_t beta, WordleNode *branch_nodes)
{
    size_t try_sum = 2 * solver_instance->n_hidden - branch->sizes[N_BRANCHES - 1];
    if (try_sum >= beta)
    {
        return UINTMAX_MAX;
    }

    size_t progress = solver_instance->n_hidden;
    size_t branch_count = 0;
    for (size_t j = 0; j < N_BRANCHES - 1; j++) // ignore 242 (GGGGG)
    {
        if (branch->sizes[j] == 0)
        {
            continue;
        }
        if (branch->sizes[j] == solver_instance->n_hidden)
        {
            return UINTMAX_MAX;
        }
        WordleSolverInstance sub_instance = {
            .wordle_instance = solver_instance->wordle_instance,
            .n_hidden = branch->sizes[j],
            .hidden_vector = branch->hidden_indicies + branch->starts[j],
            .n_test = solver_instance->n_test,
            .test_vector = solver_instance->test_vector,
            .score_cache = solver_instance->score_cache,
            .depth = solver_instance->depth + 1};
        (branch_nodes + branch_count)->score = j;
        size_t result = min_try_sum(&sub_instance, beta - try_sum + branch->sizes[j], branch_nodes + branch_count);
        branch_count++;
        if (solver_instance->depth < LOG_DEPTH)
        {
            for (size_t i = 0; i < solver_instance->depth; i++)
            {
                printf("- ");
            }
            char decoded[25];
            descore(j, decoded);
            progress -= branch->sizes[j];
            printf("%s - %f%% (%lu + %lu - %lu / %lu)\n", decoded, (100.0 * progress) / solver_instance->n_hidden, try_sum, result, branch->sizes[j], beta);
        }
        if (result == UINTMAX_MAX)
        {
            try_sum = UINTMAX_MAX;
            break;
        }
        try_sum += result - branch->sizes[j];
        if (try_sum >= beta)
        {
            break;
        }
    }
    return try_sum;
}

size_t min_try_sum(const WordleSolverInstance *solver_instance, size_t beta, WordleNode *node)
{
    size_t n_hidden = solver_instance->n_hidden;
    size_t n_test = solver_instance->n_test;
    if (n_hidden == 0)
    {
        return 0;
    }
    if (n_hidden == 1)
    {
        node->test_index = solver_instance->hidden_vector[0];
        node->beta = 1;
        node->num_branches = 0;
        return 1;
    }
    if (n_hidden == 2)
    {
        node->test_index = solver_instance->hidden_vector[0];
        node->beta = 3;
        node->num_branches = 1;
        node->branches = calloc(1, sizeof(WordleNode));
        node->branches->test_index = solver_instance->hidden_vector[1];
        node->branches->beta = 1;
        node->branches->num_branches = 0;
        node->branches->score = solver_instance->score_cache[node->test_index][node->branches->test_index];
        return 3;
    }
    if (solver_instance->depth > 8)
    {
        return UINTMAX_MAX; // max recursion depth
    }
    if (beta <= 2 * n_hidden - 1)
    {
        return UINTMAX_MAX;
    }

    // sort test words by information gain
    size_t pruned_instance;
    size_t result = sort_test_vector(solver_instance, &pruned_instance);
    if (result != 0)
    {
        node->test_index = pruned_instance;
        node->beta = result;
        node->num_branches = n_hidden - (size_t)(pruned_instance < n_test);
        node->branches = calloc(node->num_branches, sizeof(WordleNode));
        for (size_t i = 0; i < node->num_branches; i++)
        {
            size_t pruned_shift = solver_instance->hidden_vector[i] >= pruned_instance;
            (node->branches + i)->test_index = solver_instance->hidden_vector[i + pruned_shift];
            (node->branches + i)->beta = 1;
            (node->branches + i)->num_branches = 0;
            (node->branches + i)->score = solver_instance->score_cache[node->test_index][(node->branches + i)->test_index];
        }
        return result;
    }

    size_t test_ordering[SEARCH_DEPTH]; // copy result as recursive calls change the ordering
    for (size_t i = 0; i < SEARCH_DEPTH; i++)
    {
        test_ordering[i] = solver_instance->test_vector[i].index;
    }

    size_t hidden_indicies[n_hidden];
    Branch branch = {
        .hidden_indicies = hidden_indicies,
    };
    for (size_t i = 0; i < n_test && i < SEARCH_DEPTH; i++)
    {
        // if (solver_instance->depth == 0)
        // {
        //     branch.test_index = test_ordering[7];
        // }
        // else
        // {
        branch.test_index = test_ordering[i];
        // }
        if (solver_instance->depth < LOG_DEPTH)
        {
            printf("\n");
            for (size_t i = 0; i < solver_instance->depth; i++)
            {
                printf("- ");
            }
            printf("testing %s (%lu) - %f%%\n", solver_instance->wordle_instance->test_words[branch.test_index], solver_instance->n_hidden, (100.0 * (1 + i)) / SEARCH_DEPTH);
        }
        create_branches(solver_instance, &branch);
        WordleNode *branch_nodes = calloc(branch.count, sizeof(WordleNode));
        size_t test_beta = branch_try_sum(solver_instance, &branch, beta, branch_nodes);
        if (beta > test_beta)
        {
            if (solver_instance->depth < LOG_DEPTH)
            {
                for (size_t i = 0; i < solver_instance->depth; i++)
                {
                    printf("- ");
                }
                printf("improved beta: %lu -> %lu\n", beta, test_beta);
            }
            beta = test_beta;
            node->test_index = branch.test_index;
            node->beta = test_beta;
            free_tree(node, false);
            node->num_branches = branch.count;
            node->branches = branch_nodes;
        }
        else
        {
            for (int j = branch.count - 1; j >= 0; j--)
            {
                free_tree(branch_nodes + j, j == 0);
            }
        }
        // if (solver_instance->depth == 0)
        // {
        //     break;
        // }
    }
    return beta;
}

uint8_t **populate_score_cache(const WordleInstance *wordle_instance)
{
    size_t rows = wordle_instance->n_test;
    size_t cols = wordle_instance->n_hidden;

    uint8_t **score_cache = malloc(sizeof(uint8_t *) * rows + sizeof(uint8_t) * cols * rows);

    // ptr is now pointing to the first element in of 2D array
    uint8_t *ptr = (uint8_t *)(score_cache + rows);

    // for loop to point rows pointer to appropriate location in 2D array
    for (size_t i = 0; i < rows; i++)
    {
        score_cache[i] = (ptr + cols * i);
    }

    // populate cache
    for (size_t i = 0; i < rows; i++)
    {
        for (size_t j = 0; j < cols; j++)
        {
            score_cache[i][j] = score(wordle_instance->test_words[i], wordle_instance->hidden_words[j]);
        }
    }
    return score_cache;
}

bool solve(const WordleInstance *wordle_instance, WordleSolverResult *result)
{
    // hidden vector
    size_t hidden_vector[wordle_instance->n_hidden];
    for (size_t i = 0; i < wordle_instance->n_hidden; i++)
    {
        hidden_vector[i] = i;
    }
    // test vector
    tuple test_vector[wordle_instance->n_test];
    for (size_t i = 0; i < wordle_instance->n_test; i++)
    {
        test_vector[i].index = i;
    }
    WordleSolverInstance solver_instance = {
        .wordle_instance = wordle_instance,
        .n_hidden = wordle_instance->n_hidden,
        .hidden_vector = hidden_vector,
        .n_test = wordle_instance->n_test,
        .test_vector = test_vector,
        .score_cache = (const uint8_t **)populate_score_cache(wordle_instance),
        .depth = 0,
    };

    if (wordle_instance->n_hidden > 0)
    {
        result->decision_tree = calloc(1, sizeof(WordleNode));
    }
    size_t beta = UINTMAX_MAX;
    clock_t start = clock();
    result->total = min_try_sum(&solver_instance, beta, result->decision_tree);
    result->duration = (float)(clock() - start) / CLOCKS_PER_SEC;
    result->average = (float)result->total / wordle_instance->n_hidden;

    free(solver_instance.score_cache);
    return true;
}
