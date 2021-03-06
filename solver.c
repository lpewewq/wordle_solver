#include "solver.h"
#include "result.h"
#include "solver_hashmap.h"
#include <stdio.h>
#include <time.h>

#define LOG_DEPTH 1
#define SEARCH_DEPTH 50
#define SEARCH_ENTROPY_DEPTH 0.75
#define PADDING(depth)                     \
    {                                      \
        for (size_t i = 0; i < depth; i++) \
        {                                  \
            printf("- ");                  \
        }                                  \
    }

WordleNode *optimize(const WordleSolverInstance *solver_instance, size_t beta);

size_t filter_test_words(const WordleSolverInstance *solver_instance, const size_t test_index, const uint8_t score)
{

    size_t n_test = 0;
    const char *test_word = solver_instance->wordle_instance->test_words[test_index];
    char exact_chars[5] = {0};
    char included_chars[5] = {0};
    uint8_t tmp_score = score;
    for (size_t i = 0; i < 5; i++)
    {
        if (tmp_score % 3 != 0)
        {
            included_chars[i] = test_word[i];
        }
        if (tmp_score % 3 == 2)
        {
            exact_chars[i] = test_word[i];
        }
        tmp_score /= 3;
    }
    // assign every valid test word value one, zero otherwise
    for (size_t i = 0; i < solver_instance->n_test; i++)
    {
        char test_included_chars[5];
        for (size_t j = 0; j < 5; j++)
        {
            test_included_chars[j] = included_chars[j];
        }
        test_word = solver_instance->wordle_instance->test_words[solver_instance->test_vector[i].index];
        bool valid = true;
        for (size_t j = 0; j < 5; j++)
        {
            if (exact_chars[j] != 0 && exact_chars[j] != test_word[j])
            {
                valid = false;
                break;
            }
            for (size_t k = 0; k < 5; k++)
            {

                if (test_included_chars[k] != 0 && test_included_chars[k] == test_word[j])
                {
                    test_included_chars[k] = 0;
                    break;
                }
            }
        }
        for (size_t j = 0; j < 5; j++)
        {
            if (test_included_chars[j] != 0)
            {
                valid = false;
                break;
            }
        }
        if (valid)
        {
            n_test++;
            solver_instance->test_vector[i].value = 1;
        }
        else
        {
            solver_instance->test_vector[i].value = 0;
        }
    }
    qsort(solver_instance->test_vector, solver_instance->n_test, sizeof(tuple), compare_tuples);
    return n_test;
}

size_t sum_branch_total(const WordleSolverInstance *solver_instance, Branch *branch, const size_t beta, WordleBranch *branch_nodes)
{
    size_t total = 2 * solver_instance->n_hidden - branch->sizes[N_BRANCHES - 1].value;

    // branch total is already too large
    if (total >= beta)
    {
        return UINTMAX_MAX;
    }

    // Sort branch sizes: solve small branches first
    qsort(branch->sizes, N_BRANCHES, sizeof(tuple), compare_tuples);

    size_t progress = solver_instance->n_hidden;
    for (size_t i = branch->count; (total < beta) && (i-- > 0);)
    {
        size_t score = branch->sizes[i].index;
        size_t size = branch->sizes[i].value;
        size_t start = branch->starts[score];

        if (size == solver_instance->n_hidden)
        {
            total = UINTMAX_MAX;
            break;
        }

        size_t n_test = solver_instance->n_test;

        if (solver_instance->wordle_instance->hard_mode)
        {
            n_test = filter_test_words(solver_instance, branch->test_index, score);
        }

        WordleSolverInstance sub_instance = {
            .wordle_instance = solver_instance->wordle_instance,
            .n_hidden = size,
            .hidden_vector = branch->hidden_indicies + start,
            .n_test = n_test,
            .test_vector = solver_instance->test_vector,
            .score_cache = solver_instance->score_cache,
            .depth = solver_instance->depth + 1};
        WordleNode *node = optimize(&sub_instance, beta - total + size);
        if (node == NULL)
        {
            total = UINTMAX_MAX;
            break;
        }

        branch_nodes[i].node = node;
        branch_nodes[i].score = score;

        if (solver_instance->depth < LOG_DEPTH)
        {
            PADDING(solver_instance->depth)
            char decoded[25];
            descore(score, decoded);
            progress -= size;
            printf("%s - %f%% (%lu + %lu - %lu / %lu, %lu)\n", decoded, (100.0 * progress) / solver_instance->n_hidden, total, branch_nodes[i].node->total, size, beta, n_test);
        }

        total += branch_nodes[i].node->total - size;
    }
    return total;
}

size_t optimize_beta(const WordleSolverInstance *solver_instance, Branch *branch, WordleNode *node, const float progress, size_t beta)
{
    if (solver_instance->depth < LOG_DEPTH)
    {
        printf("\n");
        PADDING(solver_instance->depth)
        printf("testing %s (%lu) - %f%%\n", solver_instance->wordle_instance->test_words[branch->test_index], solver_instance->n_hidden, 100.0 * progress);
    }

    create_branches(solver_instance, branch);
    WordleBranch *branch_nodes = calloc(branch->count, sizeof(*branch_nodes));
    size_t total = sum_branch_total(solver_instance, branch, beta, branch_nodes);

    if (beta > total)
    {
        if (solver_instance->depth < LOG_DEPTH)
        {
            PADDING(solver_instance->depth)
            printf("improved beta: %lu -> %lu\n", beta, total);
        }
        beta = total;
        node->test_index = branch->test_index;
        node->total = total;
        free(node->branches);
        node->num_branches = branch->count;
        node->branches = branch_nodes;
    }
    else
    {
        free(branch_nodes);
    }
    return beta;
}

WordleNode *_optimize(const WordleSolverInstance *solver_instance, size_t beta)
{
    WordleNode *node = calloc(1, sizeof(*node));
    node->total = UINTMAX_MAX;

    const size_t n_hidden = solver_instance->n_hidden;
    const size_t n_test = solver_instance->n_test;
    size_t pruned_index;
    bool prune = false;
    size_t hidden_indicies[n_hidden];
    Branch branch = {
        .hidden_indicies = hidden_indicies,
    };

    // max recursion depth or beta too small (smallest tree needs at least 2n-1 total tries)
    if (n_hidden == 0 || solver_instance->depth >= 10 || beta <= (2 * n_hidden - 1))
    {
        free(node);
        return NULL;
    }
    if (n_hidden == 1)
    {
        node->test_index = solver_instance->hidden_vector[0];
        node->total = 1;
        node->num_branches = 0;
        return node;
    }

    if (n_hidden == 2)
    {
        prune = true;
        pruned_index = solver_instance->hidden_vector[0];
    }
    else
    {
        // sort test words by information gain
        prune = sort_test_vector(solver_instance, &pruned_index);
    }

    if (prune)
    {
        // prune if wordle is solvable with at most two guesses
        branch.test_index = pruned_index;
        beta = optimize_beta(solver_instance, &branch, node, 1.0, beta);
    }
    else
    {
        // copy result as recursive calls change the ordering
        tuple test_ordering[SEARCH_DEPTH];
        for (size_t i = 0; i < n_test && i < SEARCH_DEPTH; i++)
        {
            test_ordering[i].index = solver_instance->test_vector[i].index;
            test_ordering[i].value = solver_instance->test_vector[i].value;
        }
        double min_entropy = SEARCH_ENTROPY_DEPTH * test_ordering[0].value;
        for (size_t i = 0; i < n_test && i < SEARCH_DEPTH; i++)
        {
            if (test_ordering[i].value < min_entropy)
            {
                break;
            }
            branch.test_index = test_ordering[i].index;
            beta = optimize_beta(solver_instance, &branch, node, (1.0 + i) / SEARCH_DEPTH, beta);
        }
    }

    if (node->total == UINTMAX_MAX)
    {
        free(node);
        return NULL;
    }
    return node;
}

WordleNode *optimize(const WordleSolverInstance *solver_instance, size_t beta)
{
    WordleNode *node;

    hasmap_key_t *key = NULL;
    if (!solver_instance->wordle_instance->hard_mode)
    {
        key = get_key(solver_instance);
        node = solver_hashmap_get(key);
        if (node != NULL)
        {
            return node;
        }
    }

    clock_t start = clock();
    node = _optimize(solver_instance, beta);

    if (node != NULL)
    {
        // save stats
        node->duration = (float)(clock() - start) / CLOCKS_PER_SEC;
        node->n_hidden = solver_instance->n_hidden;
        node->n_test = solver_instance->n_test;
        node->average_case = (float)node->total / node->n_hidden;
        if (node->num_branches == 0 && node->n_hidden > 0)
        {
            node->best_case = 1;
            node->worst_case = 1;
        }
        else
        {
            // best case equal 1 if test word is a hidden word
            node->best_case = (node->test_index < solver_instance->wordle_instance->n_hidden) ? 1 : UINTMAX_MAX;
            node->worst_case = 0;
            for (size_t i = 0; i < node->num_branches; i++)
            {
                size_t wc = 1 + node->branches[i].node->worst_case;
                if (wc > node->worst_case)
                {
                    node->worst_case = wc;
                }
                size_t bc = 1 + node->branches[i].node->best_case;
                if (bc < node->best_case)
                {
                    node->best_case = bc;
                }
            }
        }
        if (!solver_instance->wordle_instance->hard_mode)
        {
            node->key = key;
            solver_hashmap_put(key, node);
        }
    }
    else if (!solver_instance->wordle_instance->hard_mode)
    {
        free(key);
    }
    return node;
}

void optimize_decision_tree(const WordleInstance *wordle_instance, char *file_name)
{
    // initialize hidden vector indices
    size_t hidden_vector[wordle_instance->n_hidden];
    for (size_t i = 0; i < wordle_instance->n_hidden; i++)
    {
        hidden_vector[i] = i;
    }
    // initialize test vector indices
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
    if (!wordle_instance->hard_mode)
    {
        solver_hashmap_init();
    }
    WordleNode *decision_tree = optimize(&solver_instance, UINTMAX_MAX);
    save_node(file_name, wordle_instance, decision_tree);
    if (!wordle_instance->hard_mode)
    {
        solver_hashmap_cleanup();
    }
    free(solver_instance.score_cache);
}
