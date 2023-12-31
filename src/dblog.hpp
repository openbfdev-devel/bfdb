#ifndef __DBLOG__H__
#define __DBLOG__H__
#include "bffile.hpp"
#include <cstddef>

namespace bfdb {
    enum log_record_type {
        DBLOG_RECORD_FULL,
        DBLOG_RECORD_FIRST,
        DBLOG_RECORD_MIDDLE,
        DBLOG_RECORD_LAST,
        DBLOG_RECORD_MAX,
    };

    static const int DBLOG_HEADER_SIZE = 4 + 2 + 1;
    static const int DBLOG_MINIMAL_DATA_SIZE = DBLOG_HEADER_SIZE;
    static const int DBLOG_BLOCK_SIZE = 32768;

    class dblog {
    public:
        int open(std::string db_name, uint64_t log_number);
        int put(const std::string &key, const std::string &value);
        // int append(enum log_record_type type, const char *data, size_t size);
        private:
            int append_records(const std::string &data);
            int append_record(enum log_record_type type, const char *data, size_t size);
            bffile log;
            size_t block_offset = 0;
    };
}


#endif  /*__DBLOG__H__*/
