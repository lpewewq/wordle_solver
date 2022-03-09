#include "result.h"
#include <math.h>
#include <stdio.h>

void _save_node(FILE *fp, const WordleInstance *wordle_instance, const WordleNode *node)
{
    if (node == NULL)
    {
        fprintf(fp, "null\n");
        return;
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

            _save_node(fp, wordle_instance, node->branches[i].node);
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

void save_node(const char *file_name, const WordleInstance *wordle_instance, const WordleNode *decision_tree)
{
    FILE *fp;
    fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        printf("Could not open file!\n");
        exit(-1);
        return;
    }
    _save_node(fp, wordle_instance, decision_tree);
    fclose(fp);
}
