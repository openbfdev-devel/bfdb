#ifndef __MEMTABLE__H__
#define __MEMTABLE__H__

#include <string>
#include <cstdint>
#include "bfdev/skiplist.h"

namespace bfdb {
    class memtable {
        public:
            enum {
                MTABLE_PUT_INSERT,
                MTABLE_PUT_DELETE,
            };
            memtable();
            ~memtable();
            int get(const std::string &key, std::string &value, uint64_t sequence);
            int put(const std::string &key, const std::string &value);
            int put(const std::string &key, const std::string &value, uint64_t sequence, uint8_t type);
        private:
            typedef struct mtable_node_s {
                uint32_t key_size;
                char    *key;
                uint64_t sequence;
                uint8_t  type;

                uint32_t value_size;
                char    *value;
            } mtable_node_t;
            int mtable_node_build(mtable_node_t* &node, const std::string &key, const std::string &value, uint64_t sequence, uint8_t type);
            static long mtable_node_insert_cmp(const void *ap, const void *bp);
            static long mtable_node_find_cmp(const void *node, const void *key);
            static void mtable_node_free(void *p);
            struct mtable_node_s *find_le_max_sequence(struct bfdev_skip_node *startn, uint64_t max_sequence);
            struct bfdev_skip_head *table;
    };
}

#endif  /*__MEMTABLE__H__*/
