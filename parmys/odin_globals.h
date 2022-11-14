#ifndef GLOBALS_H
#define GLOBALS_H

#include "config_t.h"
#include "odin_types.h"
#include "string_cache.h"
#include "Hashtable.hpp"
#include "read_xml_arch_file.h"
#include "HardSoftLogicMixer.hpp"

/**
 * The cutoff for the number of netlist nodes. 
 * Technically, Odin-II prints statistics for 
 * netlist nodes that the total number of them
 * is greater than this value. 
 */
constexpr long long UNUSED_NODE_TYPE = 0;

extern global_args_t global_args;
extern config_t configuration;
extern loc_t my_location;

extern STRING_CACHE* output_nets_sc;
extern STRING_CACHE* input_nets_sc;

extern nnode_t* gnd_node;
extern nnode_t* vcc_node;
extern nnode_t* pad_node;

extern char* one_string;
extern char* zero_string;
extern char* pad_string;

extern t_arch Arch;
extern short physical_lut_size;

/* logic optimization mixer, once ODIN is classy, could remove that
 * and pass as member variable
 */
extern HardSoftLogicMixer* mixer;

/**
 * a global var to specify the need for cleanup after
 * receiving a coarsen BLIF file as the input.
 */
extern bool coarsen_cleanup;

extern strmap<file_type_e> file_type_strmap;
extern strmap<operation_list> yosys_subckt_strmap;

#endif
