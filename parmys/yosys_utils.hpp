#ifndef __YOSYS_UTILS_H__
#define __YOSYS_UTILS_H__

#include "odin_types.h"

Yosys::Wire *to_wire(std::string wire_name, Yosys::Module *module);
std::pair<Yosys::RTLIL::IdString, int> wideports_split(std::string name);
const std::string str(Yosys::RTLIL::SigBit sig);
const std::string str(Yosys::RTLIL::IdString id);
void handle_cell_wideports_cache(Yosys::hashlib::dict<Yosys::RTLIL::IdString, Yosys::hashlib::dict<int, Yosys::SigBit>> *cell_wideports_cache,
                                 Yosys::Design *design, Yosys::Module *module, Yosys::Cell *cell);
void handle_wideports_cache(Yosys::hashlib::dict<Yosys::RTLIL::IdString, std::pair<int, bool>> *wideports_cache, Yosys::Module *module);

#endif //__YOSYS_UTILS_H__