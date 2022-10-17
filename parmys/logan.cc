#include "kernel/yosys.h"
#include "kernel/celltypes.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct LoganPass : public Pass {

LoganPass() : Pass("logan", "ODIN_II blif simulator") { }
    void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
        for (auto w : design->top_module()->wires()) {
            log("%s\n", log_id(w->name));
        }
    }
} LoganPass;

PRIVATE_NAMESPACE_END