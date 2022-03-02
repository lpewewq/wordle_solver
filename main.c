#include "main.h"
#include "solver.h"
#include <math.h>
#include <stdio.h>

void save_node(FILE *fp, const WordleInstance *wordle_instance, const WordleNode *node)
{
    if (node == NULL)
    {
        fprintf(fp, "null\n");
    }
    fprintf(fp, "{\n");
    fprintf(fp, "\"n_hidden\": %lu,\n", node->n_hidden);
    fprintf(fp, "\"n_test\": %lu,\n", node->n_test);
    fprintf(fp, "\"duration\":%f,\n", node->duration);
    if (node->total == UINTMAX_MAX || node->n_hidden == 0)
    {
        fprintf(fp, "\"guess\": null,\n");
    }
    else
    {
        fprintf(fp, "\"guess\": \"%s\",\n", wordle_instance->test_words[node->test_index]);
    }
    fprintf(fp, "\"total\": %lu,\n", node->total);
    fprintf(fp, "\"best_case\": %lu,\n", node->best_case);
    fprintf(fp, "\"worst_case\": %lu,\n", node->worst_case);
    fprintf(fp, "\"average_case\": %f,\n", isnan(node->average_case) ? UINTMAX_MAX : node->average_case);
    fprintf(fp, "\"branches\":");
    if (node->num_branches > 0)
    {
        fprintf(fp, "{\n");
        for (size_t i = 0; i < node->num_branches;)
        {
            char buffer[25];
            descore(node->branches[i].score, buffer);
            fprintf(fp, "\"%s\":\n", buffer);

            save_node(fp, wordle_instance, node->branches[i].node);
            i++;
            if (i != node->num_branches)
            {
                fprintf(fp, ",\n");
            }
        }
        fprintf(fp, "}\n");
    }
    else
    {
        fprintf(fp, "null\n");
    }
    fprintf(fp, "}\n");
}

void save_decision_tree(const char *file_name, const WordleInstance *wordle_instance, const WordleNode *decision_tree)
{
    FILE *fp;
    fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        printf("Could not open file!");
        exit(-1);
        return;
    }
    save_node(fp, wordle_instance, decision_tree);
    fclose(fp);
}

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
    WordleNode *decision_tree = optimize_decision_tree(&wordle_instance);
    save_decision_tree(file_name, &wordle_instance, decision_tree);
    return 0;
}
