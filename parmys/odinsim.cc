#include "kernel/yosys.h"
#include "kernel/celltypes.h"

#include "odin_ii.h"
#include "odin_globals.h"
#include "simulate_blif.h"
#include "GenericReader.hpp"
#include "netlist_statistic.h"
#include "vtr_path.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct OdinSimPass : public Pass {

    void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_mode* mode) {
    	for (int i = 0; i < mode->num_pb_type_children; i++) {
        	get_physical_luts(pb_lut_list, &mode->pb_type_children[i]);
	    }
	}

	void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_pb_type* pb_type) {
    	if (pb_type) {
        	if (pb_type->class_type == LUT_CLASS) {
            	pb_lut_list.push_back(pb_type);
	        } else {
    	        for (int i = 0; i < pb_type->num_modes; i++) {
        	        get_physical_luts(pb_lut_list, &pb_type->modes[i]);
            	}
	        }
    	}
	}

	void set_physical_lut_size(std::vector<t_logical_block_type>& logical_block_types) {
    	std::vector<t_pb_type*> pb_lut_list;

	    for (t_logical_block_type& logical_block : logical_block_types) {
    	    if (logical_block.index != EMPTY_TYPE_INDEX) {
        	    get_physical_luts(pb_lut_list, logical_block.pb_type);
	        }
    	}
	    for (t_pb_type* pb_lut : pb_lut_list) {
    	    if (pb_lut) {
        	    if (pb_lut->num_input_pins < physical_lut_size || physical_lut_size < 1) {
            	    physical_lut_size = pb_lut->num_input_pins;
	            }
    	    }
	    }
	}

	OdinSimPass() : Pass("odinsim", "ODIN_II blif simulator") { }
	void help() override
	{
		log("\n");
		log("    -a ARCHITECTURE_FILE\n");
		log("        VTR FPGA architecture description file (XML)\n");
		log("\n");
		log("    -b BLIF_FILE\n");
		log("        input BLIF_FILE\n");
		log("\n");
		log("    -L PRIMARY_INPUTS\n");
		log("        list of primary inputs to hold high at cycle 0, and low for all subsequent cycles\n");
		log("\n");
		log("    -H PRIMARY_INPUTS\n");
		log("        list of primary inputs to hold low at cycle 0, and high for all subsequent cycles\n");
		log("\n");
		log("    -t INPUT_VECTOR_FILE\n");
		log("        File of predefined input vectors to simulate\n");
		log("\n");
		log("    -T OUTPUT_VECTOR_FILE\n");
		log("        File of predefined output vectors to check against simulation\n");
		log("\n");
		log("    -j PARALEL_NODE_COUNT\n");
		log("        Number of threads allowed for simulator to use, by default 1\n");
		log("\n");
		log("    -g NUM_VECTORS\n");
		log("        Number of random test vectors to generate\n");
		log("\n");
		log("    -sim_dir SIMULATION_DIRECTORY\n");
		log("        Directory output for simulation, if not specified current directory will be used by default\n");
		log("\n");
    }
    void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
        std::vector<t_physical_tile_type> physical_tile_types;
		std::vector<t_logical_block_type> logical_block_types;

        bool flag_arch_file = false;
        bool flag_sim_hold_low = false;
		bool flag_sim_hold_high = false;

        std::string arch_file_path;
        std::vector<std::string> sim_hold_low;
		std::vector<std::string> sim_hold_high;

        log_header(design, "Starting odinsim pass.\n");

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
            if (args[argidx] == "-a" && argidx+1 < args.size()) {
				arch_file_path = args[++argidx];
				flag_arch_file = true;
				continue;
			}
            if (args[argidx] == "-H" && argidx+1 < args.size()) {
				flag_sim_hold_low = true;
				sim_hold_low.push_back(args[++argidx]);

				while (argidx+1 < args.size() && args[argidx+1][0] != '-') {
					sim_hold_low.push_back(args[++argidx]);
				}

				continue;
			}
			if (args[argidx] == "-L" && argidx+1 < args.size()) {
				flag_sim_hold_high = true;
				sim_hold_high.push_back(args[++argidx]);

				while (argidx+1 < args.size() && args[argidx+1][0] != '-') {
					sim_hold_high.push_back(args[++argidx]);
				}

				continue;
			}
			if (args[argidx] == "-t" && argidx+1 < args.size()) {
				global_args.sim_vector_input_file.set(args[++argidx], argparse::Provenance::SPECIFIED);
				continue;
			}
			if (args[argidx] == "-T" && argidx+1 < args.size()) {
				global_args.sim_vector_output_file.set(args[++argidx], argparse::Provenance::SPECIFIED);
				continue;
			}
			if (args[argidx] == "-j" && argidx+1 < args.size()) {
				global_args.parralelized_simulation.set(atoi(args[++argidx].c_str()), argparse::Provenance::SPECIFIED);
				continue;
			}
			if (args[argidx] == "-sim_dir" && argidx+1 < args.size()) {
				global_args.sim_directory.set(args[++argidx], argparse::Provenance::SPECIFIED);
				continue;
			}
			if (args[argidx] == "-g" && argidx+1 < args.size()) {
				global_args.sim_num_test_vectors.set(atoi(args[++argidx].c_str()), argparse::Provenance::SPECIFIED);
				continue;
			}
			if (args[argidx] == "-b" && argidx+1 < args.size()) {
				global_args.blif_file.set(args[++argidx], argparse::Provenance::SPECIFIED);
				continue;
			}

        }
        extra_args(args, argidx, design);

		if (flag_sim_hold_low) {
			global_args.sim_hold_low.set(sim_hold_low, argparse::Provenance::SPECIFIED);
		}

		if (flag_sim_hold_high) {
			global_args.sim_hold_high.set(sim_hold_high, argparse::Provenance::SPECIFIED);
		}

		//adjust thread count
    	int thread_requested = global_args.parralelized_simulation;
    	int max_thread = std::thread::hardware_concurrency();

    	global_args.parralelized_simulation.set(
        std::max(1, std::min(thread_requested, std::min((CONCURENCY_LIMIT - 1), max_thread))), argparse::Provenance::SPECIFIED);

        if (global_args.blif_file.provenance() == argparse::Provenance::SPECIFIED) {
            configuration.list_of_file_names = {std::string(global_args.blif_file)};
            configuration.input_file_type = file_type_e::_BLIF;
        }

        if (flag_arch_file) {
			log("Architecture: %s\n", vtr::basename(arch_file_path).c_str());

			log("Reading FPGA Architecture file\n");
        	try {
            	XmlReadArch(arch_file_path.c_str(), false, &Arch, physical_tile_types, logical_block_types);
	            set_physical_lut_size(logical_block_types);
    	    } catch (vtr::VtrError& vtr_error) {
        	    log_error("Odin Failed to load architecture file: %s with exit code%d\n", vtr_error.what(), ERROR_PARSE_ARCH);
        	}
		}
        log("Using Lut input width of: %d\n", physical_lut_size);

        /*************************************************************
     	* begin simulation section
     	*/
	 	//global_args.blif_file.set(odin_mapped_blif_output, argparse::Provenance::SPECIFIED);
    	netlist_t* sim_netlist = NULL;
    	if ((global_args.blif_file.provenance() == argparse::Provenance::SPECIFIED && !coarsen_cleanup)
        	|| global_args.interactive_simulation
        	|| global_args.sim_num_test_vectors
        	|| global_args.sim_vector_input_file.provenance() == argparse::Provenance::SPECIFIED) {

			configuration.input_file_type = file_type_e::_BLIF;
            if (global_args.blif_file.provenance() != argparse::Provenance::SPECIFIED) {
                configuration.list_of_file_names = {global_args.output_file};
                my_location.file = 0;
            } else {
                log("BLIF: %s\n", vtr::basename(global_args.blif_file.value()).c_str());
                fflush(stdout);
            }

			try {
            /**
             * The blif file for simulation should follow odin_ii blif style 
             * So, here we call odin_ii's read_blif
             */
				GenericReader generic_reader = GenericReader();
            	sim_netlist = static_cast<netlist_t*>(generic_reader._read());
        	} catch (vtr::VtrError& vtr_error) {
            	log_error("Odin Failed to load BLIF file: %s with exit code:%d \n", vtr_error.what(), ERROR_PARSE_BLIF);
        	}
			
			/* Simulate netlist */
	    	if (sim_netlist && !global_args.interactive_simulation
    	    	&& (global_args.sim_num_test_vectors || (global_args.sim_vector_input_file.provenance() == argparse::Provenance::SPECIFIED))) {
        		log("Netlist Simulation Begin\n");
        		create_directory(global_args.sim_directory);

        		simulate_netlist(sim_netlist);
    		}

    		compute_statistics(sim_netlist, true);

			log("--------------------------------------------------------------------\n");
    	}
		log("odinsim pass finished.\n");
    }
} OdinSimPass;

PRIVATE_NAMESPACE_END