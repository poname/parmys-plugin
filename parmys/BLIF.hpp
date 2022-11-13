/**
 * Copyright (c) 2021 Seyed Alireza Damghani (sdamghann@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * @file This file includes the definition of the basic structure used 
 * in Odin-II BLIF class to parse a BLIF file. Moreover, it provides
 * the declaration of BLIF class routines
 *
 */

#ifndef __BLIF_H__
#define __BLIF_H__

// #include "GenericReader.hpp"
// #include "GenericWriter.hpp"

#include "config_t.h"
#include "odin_types.h"
#include "Hashtable.hpp"

#include "vtr_util.h"
#include "vtr_memory.h"

#define TOKENS " \t\n"
#define YOSYS_TOKENS "[]"
#define YOSYS_ID_FIRST_DELIMITER "\\\\"
#define YOSYS_ID_LAST_DELIMITER "\""
#define YOSYS_TOKENS "[]"
#define GND_NAME "gnd"
#define VCC_NAME "vcc"
#define HBPAD_NAME "unconn"

#define READ_BLIF_BUFFER 1048576 // 1MB

// Stores pin names of the form port[pin]
struct hard_block_pins {
    int count;
    char** names;
    // Maps name to index.
    Hashtable* index;
};

// Stores port names, and their sizes.
struct hard_block_ports {
    char* signature;
    int count;
    int* sizes;
    char** names;
    // Maps portname to index.
    Hashtable* index;
};

// Stores all information pertaining to a hard block model. (.model)
struct hard_block_model {
    char* name;

    hard_block_pins* inputs;
    hard_block_pins* outputs;

    hard_block_ports* input_ports;
    hard_block_ports* output_ports;
};

// A cache structure for models.
struct hard_block_models {
    hard_block_model** models;
    int count;
    // Maps name to model
    Hashtable* index;
};

// extern int line_count;
extern int num_lines;
extern bool skip_reading_bit_map;
extern bool insert_global_clock;

extern FILE* file;

/**
 *---------------------------------------------------------------------------------------------
 * (function: getbline)
 * 
 * @brief reading blif file lines according to the blif file type.
 * For instance, in yosys blif we must read lines since there is 
 * no '\' usage. For arbitrary long lines fgets does not work due
 * to size limit. However, the scenario in Odin is on the flipside
 * and we ought to use fgets to pass '\' and the line length are 
 * always fix and less than a const value
 * 
 * @param buf buffer pointer
 * @param size buffer size (in the case of using fgets)
 * @param fd file stream
 * 
 * @return null pointer if end of the file, otherwise the read line
 *---------------------------------------------------------------------------------------------*/
inline char* getbline(char*& buf, size_t size, FILE* fd) {
    char* retval = NULL;
    if (buf) {
        vtr::free(buf);
        buf = NULL;
    }
    /* decide whether need to read line or using fgets */
    if (configuration.coarsen) {
        retval = vtr::getline(buf, fd);
    } else {
        buf = (char*)vtr::malloc(READ_BLIF_BUFFER * sizeof(char));
        retval = vtr::fgets(buf, size, fd);
    }

    return (retval);
}

#endif //__BLIF_H__
