#include "wordle.h"
#include <stdbool.h>
#include <string.h>

const uint8_t POWERS_OF_THREE[5] = {1, 3, 9, 27, 81};
const char *WORDLE_EMOJIS[3] = {"⬛", "🟨", "🟩"};

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
