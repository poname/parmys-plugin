/*
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
 */
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "odin_ii.h"
#include "vtr_error.h"
#include "vtr_time.h"

#include "argparse.hpp"

#include "arch_util.h"

#include "Resolve.hpp"
#include "arch_types.h"
#include "ast_util.h"
#include "multipliers.h"
#include "netlist_check.h"
#include "netlist_cleanup.h"
#include "netlist_create_from_ast.h"
#include "netlist_utils.h"
#include "odin_globals.h"
#include "odin_types.h"
#include "parse_making_ast.h"
#include "partial_map.h"
#include "read_xml_arch_file.h"
#include "read_xml_config_file.h"

#include "BlockMemories.hpp"
#include "hard_blocks.h"
#include "memories.h"
#include "simulate_blif.h"

#include "adders.h"
#include "netlist_statistic.h"
#include "netlist_visualizer.h"
#include "subtractions.h"

#include "HardSoftLogicMixer.hpp"
#include "vtr_memory.h"
#include "vtr_path.h"
#include "vtr_util.h"

#define DEFAULT_OUTPUT "."

loc_t my_location;

t_arch Arch;
global_args_t global_args;
std::vector<t_physical_tile_type> physical_tile_types;
std::vector<t_logical_block_type> logical_block_types;
short physical_lut_size = -1;
int block_tag = -1;
ids default_net_type = WIRE;
HardSoftLogicMixer *mixer;

struct ParseInitRegState {
    int from_str(std::string str)
    {
        if (str == "0")
            return 0;
        else if (str == "1")
            return 1;
        else if (str == "X")
            return -1;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(int val)
    {
        if (val == 0)
            return "0";
        else if (val == 1)
            return "1";
        else if (val == -1)
            return "X";
        std::stringstream msg;
        msg << "Invalid conversion from " << val;
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::vector<std::string> default_choices() { return {"0", "1", "X"}; }
};

/*---------------------------------------------------------------------------
 * (function: set_default_options)
 *-------------------------------------------------------------------------*/
void set_default_config()
{
    /* Set up the global configuration. */
    configuration.coarsen = false;
    configuration.fflegalize = false;
    configuration.show_yosys_log = false;
    configuration.tcl_file = "";
    configuration.output_file_type = file_type_e::_BLIF;
    configuration.elaborator_type = elaborator_e::_ODIN;
    configuration.output_ast_graphs = 0;
    configuration.output_netlist_graphs = 0;
    configuration.print_parse_tokens = 0;
    configuration.output_preproc_source = 0; // TODO: unused
    configuration.debug_output_path = std::string(DEFAULT_OUTPUT);
    configuration.dsp_verilog = "arch_dsp.v";
    configuration.arch_file = "";

    configuration.fixed_hard_multiplier = 0;
    configuration.split_hard_multiplier = 0;

    configuration.split_memory_width = 0;
    configuration.split_memory_depth = 0;

    configuration.adder_cin_global = false;

    /*
     * Soft logic cutoffs. If a memory or a memory resulting from a split
     * has a width AND depth below these, it will be converted to soft logic.
     */
    configuration.soft_logic_memory_width_threshold = 0;
    configuration.soft_logic_memory_depth_threshold = 0;
}
