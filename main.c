#include "main.h"
#include "solver.h"

int main(int argc, char *argv[])
{
    size_t n_hidden = N_HIDDEN;
    size_t n_test = N_TEST;
    char *file_name = "result.json";
    if (argc > 1)
    {
        n_hidden = strtol(argv[1], NULL, 0);
    }
    if (argc > 2)
    {
        n_test = strtol(argv[2], NULL, 0);
    }
    if (argc > 3)
    {
        file_name = argv[3];
    }
    WordleInstance wordle_instance = {
        .n_hidden = n_hidden,
        .hidden_words = hidden_words,
        .n_test = n_test,
        .test_words = test_words,
    };
    optimize_decision_tree(&wordle_instance, file_name);
    return 0;
}
