#include "main.h"
#include "solver.h"
#include <math.h>
#include <stdio.h>

size_t save_node(FILE *fp, const WordleInstance *wordle_instance, const WordleNode *node, const size_t depth)
{
    size_t max_depth = 1;
    if (node == NULL)
    {
        fprintf(fp, "null\n");
        return max_depth;
    }
    fprintf(fp, "{\n");
    fprintf(fp, "\"guess\": \"%s\",\n", wordle_instance->test_words[node->test_index]);
    fprintf(fp, "\"beta\": %lu,\n", node->beta);
    fprintf(fp, "\"branches\":");
    if (node->num_branches > 0)
    {
        fprintf(fp, "{\n");
        for (size_t i = 0; i < node->num_branches;)
        {
            char buffer[25];
            descore((node->branches + i)->score, buffer);
            fprintf(fp, "\"%s\":\n", buffer);

            size_t sub_depth = save_node(fp, wordle_instance, node->branches + i, depth + 1);
            if (sub_depth > max_depth)
            {
                max_depth = sub_depth;
            }
            i++;
            if (i != node->num_branches)
            {
                fprintf(fp, ",\n");
            }
        }
        fprintf(fp, "},\n");
    }
    else
    {
        fprintf(fp, "null,\n");
    }
    fprintf(fp, "\"max_depth\": %lu\n", max_depth);
    fprintf(fp, "}\n");
    return 1 + max_depth;
}
void save_result(const WordleInstance *wordle_instance, const WordleSolverResult *result)
{
    FILE *fp;
    fp = fopen("result.json", "w");
    if (fp == NULL)
    {
        printf("Could not open file!");
        exit(-1);
        return;
    }
    fprintf(fp, "{\n");
    fprintf(fp, "\"n_hidden\": %lu,\n", wordle_instance->n_hidden);
    fprintf(fp, "\"n_test\": %lu,\n", wordle_instance->n_test);
    fprintf(fp, "\"duration\":%f,\n", result->duration);
    fprintf(fp, "\"total\": %lu,\n", result->total);
    fprintf(fp, "\"average\": %f,\n", isnan(result->average) ? 0.0 : result->average);
    fprintf(fp, "\"decision_tree\":\n");
    save_node(fp, wordle_instance, result->decision_tree, 0);
    fprintf(fp, "}\n");
    fclose(fp);
}

int main(int argc, char *argv[])
{
    size_t n_hidden = N_HIDDEN;
    size_t n_test = N_TEST;
    if (argc > 1)
    {
        n_hidden = strtol(argv[1], NULL, 0);
    }
    if (argc > 2)
    {
        n_test = strtol(argv[2], NULL, 0);
    }
    WordleInstance wordle_instance = {
        .n_hidden = n_hidden,
        .hidden_words = hidden_words,
        .n_test = n_test,
        .test_words = test_words,
    };
    WordleSolverResult result = {0};
    solve(&wordle_instance, &result);
    save_result(&wordle_instance, &result);
    free_tree(result.decision_tree, true);
    return 0;
}
