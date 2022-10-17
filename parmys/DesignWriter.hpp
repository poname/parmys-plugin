#ifndef __DESIGN_WRITER_H__
#define __DESIGN_WRITER_H__

#include "odin_types.h"

void define_logical_function_yosys(nnode_t *node, Yosys::Module *module);
void master_dot_names_header(Yosys::Module *module, nnode_t *node, Yosys::RTLIL::Cell *cell);
void update_design(Yosys::Design *design, const netlist_t *netlist);
void define_MUX_function_yosys(nnode_t *node, Yosys::Module *module);
void define_FF_yosys(nnode_t *node, Yosys::Module *module);

#endif //__DESIGN_WRITER_H__