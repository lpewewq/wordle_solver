#include "main.h"
#include "solver.h"

int main(int argc, char *argv[])
{
    size_t n_hidden = N_HIDDEN;
    size_t n_test = N_TEST;
    bool hard_mode = false;
    char *file_name = "result.json";
    if (argc > 1)
    {
        hard_mode = strtol(argv[1], NULL, 0) == 1;
    }
    if (argc > 2)
    {
        n_hidden = strtol(argv[2], NULL, 0);
    }
    if (argc > 3)
    {
        n_test = strtol(argv[3], NULL, 0);
    }
    if (argc > 4)
    {
        file_name = argv[4];
    }
    WordleInstance wordle_instance = {
        .n_hidden = n_hidden,
        .hidden_words = hidden_words,
        .n_test = n_test,
        .test_words = test_words,
        .hard_mode = hard_mode,
    };
    optimize_decision_tree(&wordle_instance, file_name);
    return 0;
}
