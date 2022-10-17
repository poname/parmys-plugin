
ifeq ($(ENABLE_ODIN_TECHMAP),1)

OBJS += passes/odintechmap/adders.o
OBJS += passes/odintechmap/enum_str.o
OBJS += passes/odintechmap/MixingOptimization.o
OBJS += passes/odintechmap/read_xml_config_file.o
OBJS += passes/odintechmap/odin_error.o
OBJS += passes/odintechmap/odin_util.o
OBJS += passes/odintechmap/netlist_statistic.o
OBJS += passes/odintechmap/netlist_utils.o
OBJS += passes/odintechmap/netlist_check.o
OBJS += passes/odintechmap/netlist_cleanup.o
OBJS += passes/odintechmap/node_creation_library.o
OBJS += passes/odintechmap/multipliers.o
OBJS += passes/odintechmap/subtractions.o
OBJS += passes/odintechmap/HardSoftLogicMixer.o
OBJS += passes/odintechmap/odin_ii.o
OBJS += passes/odintechmap/netlist_create_from_ast.o
OBJS += passes/odintechmap/string_cache.o
OBJS += passes/odintechmap/partial_map.o
OBJS += passes/odintechmap/hard_blocks.o
OBJS += passes/odintechmap/parse_making_ast.o
OBJS += passes/odintechmap/BlockMemories.o
OBJS += passes/odintechmap/memories.o
OBJS += passes/odintechmap/hierarchy_util.o
#OBJS += passes/odintechmap/ast_elaborate.o
OBJS += passes/odintechmap/ast_util.o
OBJS += passes/odintechmap/implicit_memory.o
OBJS += passes/odintechmap/Multiplexer.o
OBJS += passes/odintechmap/scope_util.o
OBJS += passes/odintechmap/Shift.o
OBJS += passes/odintechmap/netlist_visualizer.o
OBJS += passes/odintechmap/GenericReader.o
OBJS += passes/odintechmap/GenericWriter.o
OBJS += passes/odintechmap/simulate_blif.o
#OBJS += passes/odintechmap/parse_making_ast.o
OBJS += passes/odintechmap/GenericIO.o
OBJS += passes/odintechmap/Verilog.o
OBJS += passes/odintechmap/VerilogReader.o
OBJS += passes/odintechmap/VerilogWriter.o
OBJS += passes/odintechmap/BLIFReader.o
OBJS += passes/odintechmap/BLIFWriter.o
OBJS += passes/odintechmap/BLIFElaborate.o
OBJS += passes/odintechmap/Hashtable.o
OBJS += passes/odintechmap/LogicalOps.o
OBJS += passes/odintechmap/FlipFlop.o
OBJS += passes/odintechmap/Latch.o
OBJS += passes/odintechmap/CaseEqual.o
OBJS += passes/odintechmap/Modulo.o
OBJS += passes/odintechmap/Power.o
OBJS += passes/odintechmap/Division.o

OBJS += passes/odintechmap/odintechmap.o
OBJS += passes/odintechmap/odinsim.o
OBJS += passes/odintechmap/DesignWriter.o
OBJS += passes/odintechmap/yosys_utils.o
OBJS += passes/odintechmap/logan.o

endif
