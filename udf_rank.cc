/*
MIT License

Copyright (c) 2017 Athena Capital Research LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
  input parameters:
  rank (column, column, ..)

  output:
  - count of successive rows where *all provided columns* match
  - resets when encountering a new value

  example;
  INSERT INTO FOO(integer) VALUES(1);
  INSERT INTO FOO(integer) VALUES(1);
  INSERT INTO FOO(integer) VALUES(2);

  SELECT integer, RANK(integer) FROM FOO ->
  1 0 // 0 successive '1' ; counter starts at 0
  1 1 // 1 successive '1' ; counter gets incremented because 1 == 1
  2 0 // 0 successive '2' ; counter gets reset because 2 != 1

  registering the function:
  CREATE AGGREGATE FUNCTION rank RETURNS STRING SONAME 'udf_rank.so';

  getting rid of the function:
  DROP FUNCTION rank;
*/

#include <m_ctype.h>
#include <m_string.h>
#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>
#include <pwd.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CHAR_BUFFER_SIZE 120

// This contains the rank state:
// - the rank itself ( starts and resets to 0 )
// - storage per column:
//   - value type ( string, int, decimal, real )
//   - last known value ( char[], double or long long )
// - a variable telling us how many columns we store
struct rank_data {
    int rank;

    struct storage_t {
        // This is a mysql enum that holds the data type.
        Item_result contents_tp;
        union contents_t {
            char chr[CHAR_BUFFER_SIZE];
            double dbl;
            long long integer;
        } contents;
    };

    // Variable size array of storage_t
    storage_t* storage;
    size_t num_stored;
};

extern "C" {

// This is where we figure out how to parse data.
// We check that the arguments are as expected.
// We also malloc the buffer and clean it out for first use.
my_bool rank_init(UDF_INIT* initid, UDF_ARGS* args, char* message) {
    rank_data* buffer = (rank_data*)calloc(1,sizeof(rank_data));
    buffer->rank = 0;
    buffer->num_stored = args->arg_count;
    buffer->storage = (rank_data::storage_t*)calloc(buffer->num_stored, sizeof(rank_data::storage_t));

    for (size_t i = 0; i < buffer->num_stored; i++) {
        if (args->arg_type[i] == ROW_RESULT) {
            strcpy(message, "wrong argument type - should not be ROW");
            return 1;
        } else if (args->arg_type[i] != STRING_RESULT and
                   args->arg_type[i] != DECIMAL_RESULT and
                   args->arg_type[i] != REAL_RESULT and
                   args->arg_type[i] != INT_RESULT) {
            strcpy(message,
                   "unknown argument type - please update library code");
            return 1;
        }
        buffer->storage[i].contents_tp = args->arg_type[i];
    }

    initid->maybe_null = 1;
    initid->max_length = 32;
    initid->ptr = (char*)buffer;

    return 0;
}

// Clean up, deallocate
void rank_deinit(UDF_INIT* initid) {
    rank_data* buffer = (rank_data*)initid->ptr;
    free(buffer->storage);
    free(initid->ptr);
}

// Read the input value
// Depending on the data type, use a different way of comparing to the
// previously stored value. If the new value != old value, then we reset the
// rank ( to 0 ). Otherwise we increment it. We return the rank.
int rank(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length,
         char* is_null, char* is_error) {
    rank_data* buffer = (rank_data*)initid->ptr;
    bool all_fields_equal = true;
    for (size_t i = 0; i < buffer->num_stored; i++) {
        // NULL values are represented as such, and when we encounter this in
        // any of our inputs we return a rank of 0.
        if( args->args[i]==NULL )
        {
            return 0;
        }
        rank_data::storage_t& storage = buffer->storage[i];
        // We have figured out in the 'init' function what data type to expect.
        // Switch over that here to figure out which functionality to use to
        // check old vs new values.
        switch (storage.contents_tp) {
            case STRING_RESULT:
            case DECIMAL_RESULT:  // Confusingly decimals are supposed to be
                                  // treated as strings we didn't come up with
                                  // this, mysql did
            {
                const bool equal =
                    (0 == strncmp(args->args[i], storage.contents.chr,
                                  min(args->lengths[i], CHAR_BUFFER_SIZE)));
                all_fields_equal &= equal;
                // Memcpying is expensive so only do it if the contents have
                // changed
                if (!equal) {
                    memset(storage.contents.chr, 0, CHAR_BUFFER_SIZE);
                    memcpy(storage.contents.chr, args->args[i],
                           min(args->lengths[i], CHAR_BUFFER_SIZE));
                }
                break;
            }
            case REAL_RESULT: {
                double real_val = *((double*)args->args[i]);
                double prev_val = storage.contents.dbl;
                // We're dealing with floating point nastiness here..
                const bool equal = ( fabs(real_val - prev_val) < 0.00001 );
                all_fields_equal &= equal;
                storage.contents.dbl = real_val;
                break;
            }
            case INT_RESULT: {
                long long int_val = *((long long*)args->args[i]);
                long long prev_val = storage.contents.integer;
                const bool equal = (int_val == prev_val);
                all_fields_equal &= equal;
                storage.contents.integer = int_val;
                break;
            }
            default:
                // We should not get here but we really don't want to throw an
                // exception or anything. No point bringing the whole database
                // down when this method fails.
                break;
        }
    }
    buffer->rank = all_fields_equal ? (buffer->rank + 1) : 0;
    return buffer->rank;
}
}
