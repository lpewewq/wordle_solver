#include "main.h"
#include "solver.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void test3()
{
    WordleInstance wordle_instance = {
        .n_hidden = 100,
        .hidden_words = hidden_words,
        .n_test = 100,
        .test_words = test_words,
    };
    WordleSolverResult result;
    solve(&wordle_instance, &result);
    assert(result.total == 253);
}

void test2()
{
    WordleInstance wordle_instance = {
        .n_hidden = 75,
        .hidden_words = hidden_words,
        .n_test = 75,
        .test_words = test_words,
    };
    WordleSolverResult result;
    solve(&wordle_instance, &result);
    assert(result.total == 183);
}

void test1()
{
    WordleInstance wordle_instance = {
        .n_hidden = 50,
        .hidden_words = hidden_words,
        .n_test = 50,
        .test_words = test_words,
    };
    WordleSolverResult result;
    solve(&wordle_instance, &result);
    assert(result.total == 117);
}

// 200: 538
// 500: 1480
// 1000: 3168
// N_HIDDEN: 7973 (slate)

int main()
{
    // test1();
    // test2();
    // test3();
    WordleInstance wordle_instance = {
        .n_hidden = N_HIDDEN,
        .hidden_words = hidden_words,
        .n_test = N_TEST,
        .test_words = test_words,
    };
    WordleSolverResult result;
    solve(&wordle_instance, &result);
    printf("\nSolver result:");
    printf("\n duration: %fs", result.duration);
    printf("\n average: %f", result.average);
    printf("\n total: %lu\n", result.total);
    return 0;
}
