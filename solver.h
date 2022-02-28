#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define N_BRANCHES 243

typedef struct WordleInstance
{
    const size_t n_hidden;
    const char (*hidden_words)[6];
    const size_t n_test;
    const char (*test_words)[6];
} WordleInstance;

typedef struct WordleNode
{
    size_t test_index;
    size_t beta;
    uint8_t score;
    size_t num_branches;
    struct WordleNode *branches;
} WordleNode;

typedef struct WordleSolverResult
{
    float duration;
    size_t total;
    float average;
    WordleNode *decision_tree;
} WordleSolverResult;

void free_tree(WordleNode *node, bool free_root);

size_t tree_max_depth(WordleNode *node);

void descore(uint8_t score, char *output);

uint8_t score(const char *test_word, const char *hidden_word);

bool solve(const WordleInstance *wordle_instance, WordleSolverResult *output);
