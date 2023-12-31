#include "memtable.hpp"
#include "bfdev/include/bfdev/skiplist.h"
#include "bfdev/skiplist.h"
#include "serializer.hpp"
#include <cstddef>
#include <cassert>
#include <cstring>

namespace bfdb {

    long memtable::mtable_node_insert_cmp(const void *a, const void *b) {
        mtable_node_t *nodea = (mtable_node_t *)a, *nodeb = (mtable_node_t *)b;
        int result;

        result = strcmp(nodea->key, nodeb->key);
        if (result)
            return result;

        return nodea->sequence - nodeb->sequence;
    }

    long memtable::mtable_node_find_cmp(const void *node, const void *key) {
        return strcmp(((mtable_node_t *)node)->key, (const char *)key);
    }

    void memtable::mtable_node_free(void *p) {
        mtable_node_t *node;
        node = (mtable_node_t *)p;

        ::free(node->key);
        ::free(node->value);
        ::free(node);
    }

    int memtable::mtable_node_build(mtable_node_t* &node, const std::string &key, const std::string &value, uint64_t sequence, uint8_t type) {
        node = (mtable_node_t *)malloc(sizeof(mtable_node_t));
        if (node == NULL) {
            return BFDB_ERR;
        }

        node->key_size = key.size();

        node->key = (char *)::malloc(node->key_size + 1);
        if (node->key == NULL) {
            return BFDB_ERR;
        }

        ::memcpy(node->key, key.data(), key.size());
        node->key[node->key_size] = '\0';

        node->sequence = sequence;
        node->type = type;

        node->value_size = value.size();

        if (node->value_size == 0) {
            node->value = NULL;
            return BFDB_OK;
        }

        node->value = (char *)::malloc(node->value_size + 1);
        if (node->value == NULL) {
            return BFDB_ERR;
        }

        ::memcpy(node->value, value.data(), value.size());
        node->value[node->value_size] = '\0';

        return BFDB_OK;
    }

    int memtable::put(const std::string &key, const std::string &value, uint64_t sequence, uint8_t type) {
        mtable_node_t *mnode;

        if (mtable_node_build(mnode, key, value, sequence, type) != BFDB_OK) {
            return BFDB_ERR;
        }


        if (bfdev_skiplist_insert(table, (void *)mnode, mtable_node_insert_cmp) != BFDEV_ENOERR) {
            return BFDB_ERR;
        }

        return BFDB_OK;
    }

    //find the node with sequence y that have math expr: y = max(sequence_x <= max_sequence).
    //FIXME: refactor it..
    memtable::mtable_node_t* memtable::find_le_max_sequence(struct bfdev_skip_node *startn, uint64_t max_sequence) {
        mtable_node_t *mnode, *result = NULL;;
        struct bfdev_skip_node *node;
        assert(startn != NULL);

        node = startn;
        mnode = (mtable_node_t *)node->pdata;

        if (mnode->sequence <= max_sequence) {
            result = mnode;

            bfdev_skiplist_for_each_continue(node, table, 0) {
                mnode = (mtable_node_t *)node->pdata;

                if (strcmp(mnode->key, ((mtable_node_t *)startn->pdata)->key))
                    break;

                if (mnode->sequence > max_sequence)
                    break;

                result = mnode;
            }
        } else { /* mnode->sequence > sequence */
            bfdev_skiplist_for_each_reverse_continue(node, table, 0) {
                mnode = (mtable_node_t *)node->pdata;

                if (strcmp(mnode->key, ((mtable_node_t *)startn->pdata)->key))
                    break;

                if (mnode->sequence <= max_sequence) {
                    result = mnode;
                    break;
                }
            }
        }

        return result;
    }

    int memtable::get(const std::string &key, std::string &value, uint64_t sequence) {
        mtable_node_t *result = NULL;
        struct bfdev_skip_node *node;

        value.clear();

        node = (struct bfdev_skip_node *)bfdev_skiplist_find(table, (void *)key.data(), mtable_node_find_cmp);
        if (node == NULL)
            return BFDB_ERR;

        result = find_le_max_sequence(node, sequence);

        if (result == NULL) {
            return BFDB_ERR;
        }

        if (result->type == MTABLE_PUT_DELETE) {
            assert(value.size() == 0);
            return BFDB_OK;
        }

        assert(result->type == MTABLE_PUT_INSERT);
        value.assign(result->value, (size_t)result->value_size);

        return BFDB_OK;

    }

    memtable::memtable() {
        table = bfdev_skiplist_create(NULL, 6);
        assert(table);
    }

    memtable::~memtable() {
        bfdev_skiplist_destroy(table, mtable_node_free);
    }
}
