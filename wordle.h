#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct WordleInstance
{
    const size_t n_hidden;
    const char (*hidden_words)[6];
    const size_t n_test;
    const char (*test_words)[6];
    const bool hard_mode;
} WordleInstance;

void descore(uint8_t score, char *output);

uint8_t score(const char *test_word, const char *hidden_word);

uint8_t **populate_score_cache(const WordleInstance *wordle_instance);
