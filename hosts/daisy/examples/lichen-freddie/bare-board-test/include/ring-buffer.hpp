#pragma once
#include <stddef.h> // For size_t

namespace sig {
    template<typename T, size_t size> class RingBuffer {
        public:
            size_t capacity = size;
            T buffer[size];
            volatile size_t readIdx;
            volatile size_t writeIdx;

        void Init() {
            readIdx = 0;
            writeIdx = 0;
        }

        /**
         * @brief Returns the number of unread elements in the buffer.
         *
         * @return size_t the number of unread elements
         */
        inline size_t readable() {
            return (size + writeIdx - readIdx) % size;
        }

        /**
         * @brief Returns the number of elements that can be written
         * without overwriting unread data.
         *
         * @return size_t the number of writeable items remaining
         */
        inline size_t writeable() {
            // FIXME: This returns 0 whenever all elements have been read
            // (i.e. when it's empty), including in its initial state.
            // This article has a good overview of the issue:
            // https://www.snellman.net/blog/archive/2016-12-13-ring-buffers/
            return (size + readIdx - writeIdx) % size;
        }

        /**
         * @brief Directly
         *
         * @return T
         */
        inline T read() {
            size_t i = readIdx;
            T result = buffer[i];
            readIdx = (i + 1) % size;

            return result;
        }

        inline void write(T value) {
            size_t i = writeIdx;
            buffer[i] = value;
            writeIdx = (i + 1) % size;
        }

        inline void Flush() {
            writeIdx = readIdx;
        }
    };
};
