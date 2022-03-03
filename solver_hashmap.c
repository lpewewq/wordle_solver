#include "solver_hashmap.h"
#include <string.h>

HASHMAP(hasmap_key_t, WordleNode)
solver_hashmap;

int compare(const hasmap_key_t *k1, const hasmap_key_t *k2)
{
    for (size_t i = 0; i < KEY_SIZE; i++)
    {
        int diff = (*k1)[i] - (*k2)[i];
        if (diff != 0)
        {
            return diff;
        }
    }
    return 0;
}

size_t hash(const hasmap_key_t *key)
{
    return hashmap_hash_default(*key, KEY_SIZE * sizeof(bool));
}

void solver_hashmap_init()
{
    hashmap_init(&solver_hashmap, hash, compare);
}

hasmap_key_t *get_key(const WordleSolverInstance *solver_instance)
{
    hasmap_key_t *key = calloc(1, sizeof(*key));
    for (size_t i = 0; i < solver_instance->n_hidden; i++)
    {
        (*key)[solver_instance->hidden_vector[i]] = true;
    }
    return key;
}

WordleNode *solver_hashmap_get(const hasmap_key_t *key)
{
    return hashmap_get(&solver_hashmap, key);
}

void solver_hashmap_put(const hasmap_key_t *key, WordleNode *node)
{
    hashmap_put(&solver_hashmap, key, node);
}

void free_node(WordleNode *node)
{
    if (node == NULL)
    {
        return;
    }
    free(node->key);
    free(node->branches);
    free(node);
}

void solver_hashmap_cleanup()
{
    WordleNode *node;
    hashmap_foreach_data(node, &solver_hashmap)
    {
        free_node(node);
    }
    hashmap_cleanup(&solver_hashmap);
}