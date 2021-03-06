/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <iterator>
#include <boost/noncopyable.hpp>

#include <aether/proxy/types.hpp>

namespace util::buffer {
    /*
        Base attributes and methods for managing data that may be collected
            from an input buffer or stream in one or more method calls.
    */
    class base_segment 
        : private boost::noncopyable {
    protected:
        // Data cannot be read if the segment is marked as complete
        bool is_complete;

        proxy::streambuf committed;

        // Vector as a buffer because it can be conveniently resized as needed
        // and we don't ever read segments of it

        std::vector<char> buffer;
        std::size_t bytes_in_buffer;
        std::size_t num_bytes_read_last;

        base_segment();

        // Moves data in buffer to committted
        void commit_buffer(std::size_t bytes = std::numeric_limits<std::size_t>::max());

        void trim_buffer();

    public:
        /*
            Returns a copy of the data read by the segment, emptying it from the buffer.
        */
        std::string export_data();

        /*
            Returns a reference to the committed data buffer.
        */
        proxy::streambuf &committed_buffer();

        /*
            Returns the number of bytes of committed data being held.
        */
        std::size_t bytes_committed() const;

        /*
            Returns the number of bytes in the input buffer that has not been committed.
        */
        std::size_t bytes_not_committed() const;

        /*
            Returns the number of bytes last read, regardless of whether it was committed or not.
        */
        std::size_t bytes_last_read() const;

        /*
            Checks if the segment is marked as complete.
            The segment is marked as complete when an input reader
                method fulfills its completion condition.
        */
        bool complete() const;

        /*
            Resets all data and flags.
        */
        void reset();

        /*
            Commits all currently buffered data to be exported and marks the segment
                as complete.
        */
        void commit();

        void mark_as_incomplete();
        void mark_as_complete();

        /*
            Copies the segment data to an iterator location.
        */
        template <typename Iterator>
        void copy_data(Iterator dest) const {
            auto data = committed.data();
            std::copy(boost::asio::buffers_begin(data), boost::asio::buffers_end(data), dest);
        }

        /*
            Moves the segment data to an iterator location, permanently.
        */
        template <typename Iterator>
        void move_data(Iterator dest) {
            std::copy(std::istreambuf_iterator<char>(&committed), std::istreambuf_iterator<char>(), dest);
        }
    };

    /*
        Utility class for managing data that may need to be read from an
            input stream multiple times to meet the completion condition.
        Permanently removes data from the stream.
    */
    class buffer_segment : 
        public base_segment {
    private:
        bool ends_with_delim(char delim);
        bool ends_with_delim(std::string_view delim);

    public:

        // Provide versions for both std::streambuf and std::istream
        // Using std::streambuf can be much more efficient in many cases,
        // but std::istream is good for convenience and when multiple reads
        // are done in a row.

        /*
            Reads from the buffer until the total number of bytes read
                matches the number passed to this method.
            This method is stateful, so it will account for the results
                of previous reads.
            This method will fail if bytes_read is greater than bytes.
        */
        bool read_up_to_bytes(std::streambuf &in, std::size_t bytes);

        /*
            Reads from the stream until the total number of bytes read
                matches the number passed to this method.
            This method is stateful, so it will account for the results
                of previous reads.
            This method will fail if bytes_read is greater than bytes.
        */
        bool read_up_to_bytes(std::istream &in, std::size_t bytes);

        /*
            Reads from the buffer until a character delimiter is found.
        */
        bool read_until(std::streambuf &in, char delim);

        /*
            Reads from the buffer until a multi-character delimiter is found.
        */
        bool read_until(std::streambuf &in, std::string_view delim);

        /*
            Reads from the stream until a character delimiter is found.
        */
        bool read_until(std::istream &in, char delim);

        /*
            Reads from the stream until a multi-character delimiter is found.
        */
        bool read_until(std::istream &in, std::string_view delim);

        /*
            Reads all data from the buffer.
        */
        void read_all(std::streambuf &in);

        /*
            Reads all data from the stream.
        */
        void read_all(std::istream &in);
    };

    /*
        Utility class for managing data that may need to be read from a
            constant buffer multiple times to meet the completion condition.
        Does not remove data when reading.
    */
    class const_buffer_segment 
        : public base_segment {
    public:
        /*
            Reads from the buffer until the total number of bytes in the buffer
                matches the number passed to this method.
            This method is stateful, so it will account for the results
                of previous reads.
            This method will fail if bytes_read is greater than bytes.
        */
        bool read_up_to_bytes(proxy::const_streambuf &buf, std::size_t bytes, std::size_t size);
    };
}