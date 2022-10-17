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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hard_blocks.h"
#include "memories.h"
#include "netlist_utils.h"
#include "odin_globals.h"
#include "odin_types.h"
#include "odin_util.h"

#include "kernel/yosys.h"

#include "yosys_utils.hpp"

STRING_CACHE *hard_block_names = NULL;

void cache_hard_block_names();
void register_hb_port_size(t_model_ports *hb_ports, int size);

void register_hb_port_size(t_model_ports *hb_ports, int size)
{
	if (hb_ports)
		hb_ports->size = size;
	/***
	 * else
	 *	TODO error
	 */
}

t_model_ports *get_model_port(t_model_ports *ports, const char *name)
{
	while (ports && strcmp(ports->name, name))
		ports = ports->next;

	return ports;
}

void cache_hard_block_names()
{
	t_model *hard_blocks = NULL;

	hard_blocks = Arch.models;
	hard_block_names = sc_new_string_cache();
	while (hard_blocks) {
		int sc_spot = sc_add_string(hard_block_names, hard_blocks->name);
		hard_block_names->data[sc_spot] = (void *)hard_blocks;
		hard_blocks = hard_blocks->next;
	}
}

void register_hard_blocks()
{
	cache_hard_block_names();
	single_port_rams = find_hard_block(SINGLE_PORT_RAM_string);
	dual_port_rams = find_hard_block(DUAL_PORT_RAM_string);

	if (single_port_rams) {
		if (configuration.split_memory_width) {
			register_hb_port_size(get_model_port(single_port_rams->inputs, "data"), 1);

			register_hb_port_size(get_model_port(single_port_rams->outputs, "out"), 1);
		}

		register_hb_port_size(get_model_port(single_port_rams->inputs, "addr"), get_sp_ram_split_depth());
	}

	if (dual_port_rams) {
		if (configuration.split_memory_width) {
			register_hb_port_size(get_model_port(dual_port_rams->inputs, "data1"), 1);
			register_hb_port_size(get_model_port(dual_port_rams->inputs, "data2"), 1);

			register_hb_port_size(get_model_port(dual_port_rams->outputs, "out1"), 1);
			register_hb_port_size(get_model_port(dual_port_rams->outputs, "out2"), 1);
		}

		int split_depth = get_dp_ram_split_depth();

		register_hb_port_size(get_model_port(dual_port_rams->inputs, "addr1"), split_depth);
		register_hb_port_size(get_model_port(dual_port_rams->inputs, "addr2"), split_depth);
	}
}

void deregister_hard_blocks()
{
	sc_free_string_cache(hard_block_names);
	return;
}

t_model *find_hard_block(const char *name)
{
	t_model *hard_blocks;

	hard_blocks = Arch.models;
	while (hard_blocks)
		if (!strcmp(hard_blocks->name, name))
			return hard_blocks;
		else
			hard_blocks = hard_blocks->next;

	return NULL;
}

void define_hard_block(nnode_t *node, FILE *out)
{
	int i;
	int index, port;
	char buffer[MAX_BUF];

	/* Assert that every hard block has at least an input and output */
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

	// IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in define_add_function and define_mult_function)
	if (strcmp(node->related_ast_node->identifier_node->types.identifier, "multiply") == 0 ||
	    strcmp(node->related_ast_node->identifier_node->types.identifier, "adder") == 0)
		return;

	fprintf(out, "\n.subckt ");
	fprintf(out, "%s", node->related_ast_node->identifier_node->types.identifier);

	/* print the input port mappings */
	port = index = 0;
	for (i = 0; i < node->num_input_pins; i++) {
		/* Check that the input pin is driven */
		if (node->input_pins[i]->net->num_driver_pins == 0 && node->input_pins[i]->net != syn_netlist->zero_net &&
		    node->input_pins[i]->net != syn_netlist->one_net && node->input_pins[i]->net != syn_netlist->pad_net) {
			warning_message(NETLIST, node->loc, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
			add_fanout_pin_to_net(syn_netlist->zero_net, node->input_pins[i]);
		} else if (node->input_pins[i]->net->num_driver_pins > 1) {
			error_message(NETLIST, node->loc, "Multiple (%d) driver pins not supported in hard block definition\n",
				      node->input_pins[i]->net->num_driver_pins);
		}

		if (node->input_port_sizes[port] == 1) {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " %s=%s", node->input_pins[i]->mapping, node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " %s=%s", node->input_pins[i]->mapping, node->input_pins[i]->net->driver_pins[0]->node->name);
		} else {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " %s[%d]=%s", node->input_pins[i]->mapping, index,
					     node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " %s[%d]=%s", node->input_pins[i]->mapping, index,
					     node->input_pins[i]->net->driver_pins[0]->node->name);
		}

		fprintf(out, "%s", buffer);

		index++;
		if (node->input_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	/* print the output port mappings */
	port = index = 0;
	for (i = 0; i < node->num_output_pins; i++) {
		if (node->output_port_sizes[port] != 1)
			odin_sprintf(buffer, " %s[%d]=%s", node->output_pins[i]->mapping, index, node->output_pins[i]->name);
		else
			odin_sprintf(buffer, " %s=%s", node->output_pins[i]->mapping, node->output_pins[i]->name);

		fprintf(out, "%s", buffer);

		index++;
		if (node->output_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	fprintf(out, "\n\n");
	return;
}

void cell_hard_block(nnode_t *node, Yosys::Module *module, Yosys::Design *design)
{
	int index, port;

	/* Assert that every hard block has at least an input and output */
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

	Yosys::IdString celltype = Yosys::RTLIL::escape_id(node->related_ast_node->identifier_node->types.identifier);
	Yosys::RTLIL::Cell *cell = module->addCell(NEW_ID, celltype);

	Yosys::hashlib::dict<Yosys::RTLIL::IdString, Yosys::hashlib::dict<int, Yosys::SigBit>> cell_wideports_cache;

	/* print the input port mappings */
	port = index = 0;
	for (int i = 0; i < node->num_input_pins; i++) {
		/* Check that the input pin is driven */
		if (node->input_pins[i]->net->num_driver_pins == 0 && node->input_pins[i]->net != syn_netlist->zero_net &&
		    node->input_pins[i]->net != syn_netlist->one_net && node->input_pins[i]->net != syn_netlist->pad_net) {
			warning_message(NETLIST, node->loc, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
			add_fanout_pin_to_net(syn_netlist->zero_net, node->input_pins[i]);
		} else if (node->input_pins[i]->net->num_driver_pins > 1) {
			error_message(NETLIST, node->loc, "Multiple (%d) driver pins not supported in hard block definition\n",
				      node->input_pins[i]->net->num_driver_pins);
		}
		std::string p, q;

		if (node->input_port_sizes[port] == 1) {
			p = node->input_pins[i]->mapping;
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				q = node->input_pins[i]->net->driver_pins[0]->name;
			else
				q = node->input_pins[i]->net->driver_pins[0]->node->name;
		} else {
			p = Yosys::stringf("%s[%d]", node->input_pins[i]->mapping, index);
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				q = node->input_pins[i]->net->driver_pins[0]->name;
			else
				q = node->input_pins[i]->net->driver_pins[0]->node->name;
		}

		std::pair<Yosys::RTLIL::IdString, int> wp = wideports_split(p);
		if (wp.first.empty())
			cell->setPort(Yosys::RTLIL::escape_id(p), to_wire(q, module));
		else
			cell_wideports_cache[wp.first][wp.second] = to_wire(q, module);

		index++;
		if (node->input_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	/* print the output port mappings */
	port = index = 0;
	for (int i = 0; i < node->num_output_pins; i++) {
		std::string p, q;
		if (node->output_port_sizes[port] != 1) {
			p = Yosys::stringf("%s[%d]", node->output_pins[i]->mapping, index);
			q = node->output_pins[i]->name;
		} else {
			p = node->output_pins[i]->mapping;
			q = node->output_pins[i]->name;
		}

		std::pair<Yosys::RTLIL::IdString, int> wp = wideports_split(p);
		if (wp.first.empty())
			cell->setPort(Yosys::RTLIL::escape_id(p), to_wire(q, module));
		else
			cell_wideports_cache[wp.first][wp.second] = to_wire(q, module);

		index++;
		if (node->output_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	handle_cell_wideports_cache(&cell_wideports_cache, design, module, cell);

	for (auto &param : node->cell_parameters) {
		cell->parameters[Yosys::RTLIL::IdString(param.first)] = Yosys::Const(param.second);
	}

	return;
}

void define_skip_block_yosys(nnode_t *node, FILE *out)
{
	int i;
	int index, port;
	char buffer[MAX_BUF];

	/* Assert that every hard block has at least an input and output */
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

	// IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in define_add_function and define_mult_function)
	// if (strcmp(node->related_ast_node->identifier_node->types.identifier, "multiply") == 0 ||
	// strcmp(node->related_ast_node->identifier_node->types.identifier, "adder") == 0)
	//     return;

	fprintf(out, ".subckt ");
	fprintf(out, "%s", node->related_ast_node->identifier_node->types.identifier);

	/* print the input port mappings */
	port = index = 0;
	for (i = 0; i < node->num_input_pins; i++) {
		/* Check that the input pin is driven */
		if (node->input_pins[i]->net->num_driver_pins == 0 && node->input_pins[i]->net != syn_netlist->zero_net &&
		    node->input_pins[i]->net != syn_netlist->one_net && node->input_pins[i]->net != syn_netlist->pad_net) {
			warning_message(NETLIST, node->loc, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
			add_fanout_pin_to_net(syn_netlist->zero_net, node->input_pins[i]);
		} else if (node->input_pins[i]->net->num_driver_pins > 1) {
			error_message(NETLIST, node->loc, "Multiple (%d) driver pins not supported in hard block definition\n",
				      node->input_pins[i]->net->num_driver_pins);
		}

		if (node->input_port_sizes[port] == 1) {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " %s=%s", node->input_pins[i]->mapping, node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " %s=%s", node->input_pins[i]->mapping, node->input_pins[i]->net->driver_pins[0]->node->name);
		} else {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " %s[%d]=%s", node->input_pins[i]->mapping, index,
					     node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " %s[%d]=%s", node->input_pins[i]->mapping, index,
					     node->input_pins[i]->net->driver_pins[0]->node->name);
		}

		fprintf(out, "%s", buffer);

		index++;
		if (node->input_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	/* print the output port mappings */
	port = index = 0;
	for (i = 0; i < node->num_output_pins; i++) {
		if (node->output_port_sizes[port] != 1)
			odin_sprintf(buffer, " %s[%d]=%s", node->output_pins[i]->mapping, index, node->output_pins[i]->name);
		else
			odin_sprintf(buffer, " %s=%s", node->output_pins[i]->mapping, node->output_pins[i]->name);

		fprintf(out, "%s", buffer);

		index++;
		if (node->output_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	fprintf(out, "\n");
	return;
}

// void cell_skip_block_yosys(nnode_t* node, Yosys::Module* module, Yosys::Design* design) {
//     int i;
//     int index, port;
//     char buffer[MAX_BUF];

//     /* Assert that every hard block has at least an input and output */
//     oassert(node->input_port_sizes[0] > 0);
//     oassert(node->output_port_sizes[0] > 0);

//     //IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in define_add_function and define_mult_function)
//     //if (strcmp(node->related_ast_node->identifier_node->types.identifier, "multiply") == 0 ||
//     strcmp(node->related_ast_node->identifier_node->types.identifier, "adder") == 0)
//     //    return;

//     // fprintf(out, ".subckt ");
//     // fprintf(out, "%s", node->related_ast_node->identifier_node->types.identifier);

//     Yosys::IdString celltype = Yosys::RTLIL::escape_id(node->related_ast_node->identifier_node->types.identifier);
// 	Yosys::RTLIL::Cell *cell = module->addCell(NEW_ID, celltype);
// 	Yosys::RTLIL::Module *cell_mod = design->module(celltype);

// 	Yosys::hashlib::dict<Yosys::RTLIL::IdString, Yosys::hashlib::dict<int, Yosys::SigBit>> cell_wideports_cache;

//     /* print the input port mappings */
//     port = index = 0;
//     for (i = 0; i < node->num_input_pins; i++) {
//         /* Check that the input pin is driven */
//         if (node->input_pins[i]->net->num_driver_pins == 0
//             && node->input_pins[i]->net != syn_netlist->zero_net
//             && node->input_pins[i]->net != syn_netlist->one_net
//             && node->input_pins[i]->net != syn_netlist->pad_net) {
//             warning_message(NETLIST, node->loc, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
//             add_fanout_pin_to_net(syn_netlist->zero_net, node->input_pins[i]);
//         } else if (node->input_pins[i]->net->num_driver_pins > 1) {
//             error_message(NETLIST, node->loc, "Multiple (%d) driver pins not supported in hard block definition\n",
//             node->input_pins[i]->net->num_driver_pins);
//         }

//         if (node->input_port_sizes[port] == 1) {
//             if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
//                 odin_sprintf(buffer, " %s=%s", node->input_pins[i]->mapping, node->input_pins[i]->net->driver_pins[0]->name);
//             else
//                 odin_sprintf(buffer, " %s=%s", node->input_pins[i]->mapping, node->input_pins[i]->net->driver_pins[0]->node->name);
//         } else {
//             if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
//                 odin_sprintf(buffer, " %s[%d]=%s", node->input_pins[i]->mapping, index, node->input_pins[i]->net->driver_pins[0]->name);
//             else
//                 odin_sprintf(buffer, " %s[%d]=%s", node->input_pins[i]->mapping, index, node->input_pins[i]->net->driver_pins[0]->node->name);
//         }

//         fprintf(out, "%s", buffer);

//         index++;
//         if (node->input_port_sizes[port] == index) {
//             index = 0;
//             port++;
//         }
//     }

//     /* print the output port mappings */
//     port = index = 0;
//     for (i = 0; i < node->num_output_pins; i++) {
//         if (node->output_port_sizes[port] != 1)
//             odin_sprintf(buffer, " %s[%d]=%s", node->output_pins[i]->mapping, index, node->output_pins[i]->name);
//         else
//             odin_sprintf(buffer, " %s=%s", node->output_pins[i]->mapping, node->output_pins[i]->name);

//         fprintf(out, "%s", buffer);

//         index++;
//         if (node->output_port_sizes[port] == index) {
//             index = 0;
//             port++;
//         }
//     }

//     fprintf(out, "\n");
//     return;
// }

void define_subckt_block(nnode_t *node, FILE *out)
{
	int i;
	int index, port;
	char buffer[MAX_BUF];

	/* Assert that every hard block has at least an input and output */
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

	// IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in define_add_function and define_mult_function)
	//  if (strcmp(node->related_ast_node->identifier_node->types.identifier, "multiply") == 0 ||
	//  strcmp(node->related_ast_node->identifier_node->types.identifier, "adder") == 0)
	//      return;

	fprintf(out, ".subckt ");
	fprintf(out, "%s", node->type == LOGICAL_OR ? "$reduce_or" : node->type == LOGICAL_AND ? "$reduce_and" : "$unk");

	/* print the input port mappings */
	port = index = 0;
	for (i = 0; i < node->num_input_pins; i++) {
		/* Check that the input pin is driven */
		if (node->input_pins[i]->net->num_driver_pins == 0 && node->input_pins[i]->net != syn_netlist->zero_net &&
		    node->input_pins[i]->net != syn_netlist->one_net && node->input_pins[i]->net != syn_netlist->pad_net) {
			warning_message(NETLIST, node->loc, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
			add_fanout_pin_to_net(syn_netlist->zero_net, node->input_pins[i]);
		} else if (node->input_pins[i]->net->num_driver_pins > 1) {
			error_message(NETLIST, node->loc, "Multiple (%d) driver pins not supported in hard block definition\n",
				      node->input_pins[i]->net->num_driver_pins);
		}

		if (node->input_port_sizes[port] == 1) {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " A=%s", node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " A=%s", node->input_pins[i]->net->driver_pins[0]->node->name);
		} else {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " A[%d]=%s", index, node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " A[%d]=%s", index, node->input_pins[i]->net->driver_pins[0]->node->name);
		}

		fprintf(out, "%s", buffer);

		index++;
		if (node->input_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	/* print the output port mappings */
	port = index = 0;
	for (i = 0; i < node->num_output_pins; i++) {
		if (node->output_port_sizes[port] != 1)
			odin_sprintf(buffer, " Y[%d]=%s", index, node->output_pins[i]->name);
		else
			odin_sprintf(buffer, " Y=%s", node->name);

		fprintf(out, "%s", buffer);

		index++;
		if (node->output_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	fprintf(out, "\n.param A_WIDTH %s\n", Yosys::RTLIL::Const(int(node->num_input_pins)).as_string().c_str());
	fprintf(out, ".param Y_WIDTH %s\n", Yosys::RTLIL::Const(int(node->num_output_pins)).as_string().c_str());
	fprintf(out, ".param A_SIGNED %s\n", Yosys::RTLIL::Const(false).as_string().c_str());

	fprintf(out, "\n");
	return;
}

void define_lut_block(nnode_t *node, FILE *out)
{
	int i;
	int index, port;
	char buffer[MAX_BUF];

	/* Assert that every hard block has at least an input and output */
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

	fprintf(out, "\n.names ");

	/* print the input port mappings */
	port = index = 0;
	for (i = node->num_input_pins - 1; i >= 0; i--) {
		/* Check that the input pin is driven */
		if (node->input_pins[i]->net->num_driver_pins == 0 && node->input_pins[i]->net != syn_netlist->zero_net &&
		    node->input_pins[i]->net != syn_netlist->one_net && node->input_pins[i]->net != syn_netlist->pad_net) {
			warning_message(NETLIST, node->loc, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
			add_fanout_pin_to_net(syn_netlist->zero_net, node->input_pins[i]);
		} else if (node->input_pins[i]->net->num_driver_pins > 1) {
			error_message(NETLIST, node->loc, "Multiple (%d) driver pins not supported in hard block definition\n",
				      node->input_pins[i]->net->num_driver_pins);
		}

		if (node->input_port_sizes[port] == 1) {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " %s", node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " %s", node->input_pins[i]->net->driver_pins[0]->node->name);
		} else {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " %s", node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " %s", node->input_pins[i]->net->driver_pins[0]->node->name);
		}

		fprintf(out, "%s", buffer);

		index++;
		if (node->input_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	/* print the output port mappings */
	port = index = 0;
	for (i = 0; i < node->num_output_pins; i++) {
		if (node->output_port_sizes[port] != 1)
			odin_sprintf(buffer, " %s", node->output_pins[i]->name);
		else
			odin_sprintf(buffer, " %s", node->output_pins[i]->name);

		fprintf(out, "%s", buffer);

		index++;
		if (node->output_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	fprintf(out, "\n");

	auto width = node->cell->parameters.at(Yosys::RTLIL::ID::WIDTH).as_int();
	Yosys::RTLIL::SigSpec mask = node->cell->parameters.at(Yosys::RTLIL::ID::LUT);
	std::string one_("1");
	std::string zero_("0");
	for (int i = 0; i < (1 << width); i++)
		if (mask[i] == Yosys::RTLIL::State::S1) {
			for (int j = width - 1; j >= 0; j--) {
				fprintf(out, (i >> j) & 1 ? one_.c_str() : zero_.c_str());
			}
			fprintf(out, " 1\n");
		}

	fprintf(out, "\n");

	return;
}

void define_sop_block(nnode_t *node, FILE *out)
{
	int i;
	int index, port;
	char buffer[MAX_BUF];

	/* Assert that every hard block has at least an input and output */
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

	fprintf(out, "\n.names ");

	/* print the input port mappings */
	port = index = 0;
	for (i = 0; i < node->num_input_pins; i++) {
		/* Check that the input pin is driven */
		if (node->input_pins[i]->net->num_driver_pins == 0 && node->input_pins[i]->net != syn_netlist->zero_net &&
		    node->input_pins[i]->net != syn_netlist->one_net && node->input_pins[i]->net != syn_netlist->pad_net) {
			warning_message(NETLIST, node->loc, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
			add_fanout_pin_to_net(syn_netlist->zero_net, node->input_pins[i]);
		} else if (node->input_pins[i]->net->num_driver_pins > 1) {
			error_message(NETLIST, node->loc, "Multiple (%d) driver pins not supported in hard block definition\n",
				      node->input_pins[i]->net->num_driver_pins);
		}

		if (node->input_port_sizes[port] == 1) {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " %s", node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " %s", node->input_pins[i]->net->driver_pins[0]->node->name);
		} else {
			if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
				odin_sprintf(buffer, " %s", node->input_pins[i]->net->driver_pins[0]->name);
			else
				odin_sprintf(buffer, " %s", node->input_pins[i]->net->driver_pins[0]->node->name);
		}

		fprintf(out, "%s", buffer);

		index++;
		if (node->input_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	/* print the output port mappings */
	port = index = 0;
	for (i = 0; i < node->num_output_pins; i++) {
		if (node->output_port_sizes[port] != 1)
			odin_sprintf(buffer, " %s", node->output_pins[i]->name);
		else
			odin_sprintf(buffer, " %s", node->output_pins[i]->name);

		fprintf(out, "%s", buffer);

		index++;
		if (node->output_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	fprintf(out, "\n");

	auto width = node->cell->parameters.at(Yosys::RTLIL::ID::WIDTH).as_int();
	auto depth = node->cell->parameters.at(Yosys::RTLIL::ID::DEPTH).as_int();
	std::vector<Yosys::State> table = node->cell->parameters.at(Yosys::RTLIL::ID::TABLE).bits;
	while (Yosys::GetSize(table) < 2 * width * depth)
		table.push_back(Yosys::State::S0);
	for (int i = 0; i < depth; i++) {
		for (int j = 0; j < width; j++) {
			bool pat0 = table.at(2 * width * i + 2 * j + 0) == Yosys::State::S1;
			bool pat1 = table.at(2 * width * i + 2 * j + 1) == Yosys::State::S1;
			if (pat0 && !pat1)
				fprintf(out, "0");
			else if (!pat0 && pat1)
				fprintf(out, "1");
			else
				fprintf(out, "-");
		}
		fprintf(out, " 1\n");
	}

	fprintf(out, "\n");

	return;
}

void define_required_yosys_params(nnode_t *node, FILE *out)
{
	if (node->cell != NULL and !node->cell->module->get_blackbox_attribute()) {
		for (auto &param : node->cell->parameters) {
			fprintf(out, "%s %s ", ".param", Yosys::log_id(param.first));
			if (param.second.flags & Yosys::RTLIL::CONST_FLAG_STRING) {
				std::string str = param.second.decode_string();
				fprintf(out, "\"");
				for (char ch : str)
					if (ch == '"' || ch == '\\')
						fprintf(out, "\\%c", ch);
					else if (ch < 32 || ch >= 127)
						fprintf(out, "\\%03o", ch);
					else
						fprintf(out, "%c", ch);
				fprintf(out, "\"\n");
			} else
				fprintf(out, "%s\n", param.second.as_string().c_str());
		}
	}
	fprintf(out, "\n");
	return;
}

void output_hard_blocks(FILE *out)
{
	t_model_ports *hb_ports;
	t_model *hard_blocks;
	char buffer[MAX_BUF];
	int i;

	oassert(out != NULL);
	hard_blocks = Arch.models;
	while (hard_blocks != NULL) {
		if (hard_blocks->used == 1) /* Hard Block is utilized */
		{
			// IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in add_the_blackbox_for_adds and
			// add_the_blackbox_for_mults)
			if (strcmp(hard_blocks->name, "adder") == 0 || strcmp(hard_blocks->name, "multiply") == 0) {
				hard_blocks = hard_blocks->next;
				break;
			}

			fprintf(out, "\n.model %s\n", hard_blocks->name);
			fprintf(out, ".inputs");
			hb_ports = hard_blocks->inputs;
			while (hb_ports != NULL) {
				for (i = 0; i < hb_ports->size; i++) {
					if (hb_ports->size == 1)
						odin_sprintf(buffer, " %s", hb_ports->name);
					else
						odin_sprintf(buffer, " %s[%d]", hb_ports->name, i);

					fprintf(out, "%s", buffer);
				}
				hb_ports = hb_ports->next;
			}

			fprintf(out, "\n.outputs");
			hb_ports = hard_blocks->outputs;
			while (hb_ports != NULL) {
				for (i = 0; i < hb_ports->size; i++) {
					if (hb_ports->size == 1)
						odin_sprintf(buffer, " %s", hb_ports->name);
					else
						odin_sprintf(buffer, " %s[%d]", hb_ports->name, i);

					fprintf(out, "%s", buffer);
				}
				hb_ports = hb_ports->next;
			}

			fprintf(out, "\n.blackbox\n.end\n\n");
		}
		hard_blocks = hard_blocks->next;
	}

	return;
}

void output_hard_blocks_yosys(Yosys::Design *design)
{
	t_model_ports *hb_ports;
	t_model *hard_blocks;

	hard_blocks = Arch.models;
	while (hard_blocks != NULL) {
		if (hard_blocks->used == 1) /* Hard Block is utilized */
		{
			// IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in add_the_blackbox_for_adds and
			// add_the_blackbox_for_mults)
			if (strcmp(hard_blocks->name, "adder") == 0 || strcmp(hard_blocks->name, "multiply") == 0) {
				hard_blocks = hard_blocks->next;
				break;
			}

			Yosys::RTLIL::Module *module = nullptr;

			Yosys::hashlib::dict<Yosys::RTLIL::IdString, std::pair<int, bool>> wideports_cache;

			module = new Yosys::RTLIL::Module;
			module->name = Yosys::RTLIL::escape_id(hard_blocks->name);

			if (design->module(module->name))
				Yosys::log_error("Duplicate definition of module %s!\n", Yosys::log_id(module->name));
			design->add(module);

			hb_ports = hard_blocks->inputs;
			while (hb_ports != NULL) {
				for (int i = 0; i < hb_ports->size; i++) {
					std::string w_name;
					if (hb_ports->size == 1)
						w_name = hb_ports->name;
					else
						w_name = Yosys::stringf("%s[%d]", hb_ports->name, i);

					Yosys::RTLIL::Wire *wire = to_wire(w_name, module);
					wire->port_input = true;

					std::pair<Yosys::RTLIL::IdString, int> wp = wideports_split(w_name);
					if (!wp.first.empty() && wp.second >= 0) {
						wideports_cache[wp.first].first = std::max(wideports_cache[wp.first].first, wp.second + 1);
						wideports_cache[wp.first].second = true;
					}
				}

				hb_ports = hb_ports->next;
			}

			// fprintf(out, "\n.outputs");
			hb_ports = hard_blocks->outputs;
			while (hb_ports != NULL) {
				for (int i = 0; i < hb_ports->size; i++) {
					std::string w_name;
					if (hb_ports->size == 1)
						w_name = hb_ports->name;
					else
						w_name = Yosys::stringf("%s[%d]", hb_ports->name, i);

					Yosys::RTLIL::Wire *wire = to_wire(w_name, module);
					wire->port_output = true;

					std::pair<Yosys::RTLIL::IdString, int> wp = wideports_split(w_name);
					if (!wp.first.empty() && wp.second >= 0) {
						wideports_cache[wp.first].first = std::max(wideports_cache[wp.first].first, wp.second + 1);
						wideports_cache[wp.first].second = false;
					}
				}

				hb_ports = hb_ports->next;
			}

			handle_wideports_cache(&wideports_cache, module);

			module->fixup_ports();
			wideports_cache.clear();

			module->attributes[Yosys::ID::blackbox] = Yosys::RTLIL::Const(1);
		}

		hard_blocks = hard_blocks->next;
	}

	return;
}

void instantiate_hard_block(nnode_t *node, short mark, netlist_t * /*netlist*/)
{
	int i, port, index;

	port = index = 0;
	/* Give names to the output pins */
	for (i = 0; i < node->num_output_pins; i++) {
		if (node->output_pins[i]->name == NULL)
			node->output_pins[i]->name = make_full_ref_name(node->name, NULL, NULL, node->output_pins[i]->mapping, i);
		// node->output_pins[i]->name = make_full_ref_name(node->name, NULL, NULL, node->output_pins[i]->mapping,
		// (configuration.elaborator_type == elaborator_e::_YOSYS) ? i : -1); //@TODO

		index++;
		if (node->output_port_sizes[port] == index) {
			index = 0;
			port++;
		}
	}

	node->traverse_visited = mark;
	return;
}

int hard_block_port_size(t_model *hb, char *pname)
{
	t_model_ports *tmp;

	if (hb == NULL)
		return 0;

	/* Indicates that the port size is different for this hard block
	 *  depending on the instance of the hard block. May want to extend
	 *  this list of blocks in the future.
	 */
	if ((strcmp(hb->name, SINGLE_PORT_RAM_string) == 0) || (strcmp(hb->name, DUAL_PORT_RAM_string) == 0)) {
		return -1;
	}

	tmp = hb->inputs;
	while (tmp != NULL)
		if ((tmp->name != NULL) && (strcmp(tmp->name, pname) == 0))
			return tmp->size;
		else
			tmp = tmp->next;

	tmp = hb->outputs;
	while (tmp != NULL)
		if ((tmp->name != NULL) && (strcmp(tmp->name, pname) == 0))
			return tmp->size;
		else
			tmp = tmp->next;

	return 0;
}

enum PORTS hard_block_port_direction(t_model *hb, char *pname)
{
	t_model_ports *tmp;

	if (hb == NULL)
		return ERR_PORT;

	tmp = hb->inputs;
	while (tmp != NULL)
		if ((tmp->name != NULL) && (strcmp(tmp->name, pname) == 0))
			return tmp->dir;
		else
			tmp = tmp->next;

	tmp = hb->outputs;
	while (tmp != NULL)
		if ((tmp->name != NULL) && (strcmp(tmp->name, pname) == 0))
			return tmp->dir;
		else
			tmp = tmp->next;

	return ERR_PORT;
}
