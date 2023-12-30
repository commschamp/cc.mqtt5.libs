#include "AflFuzz.h"
#include "Logger.h"
#include "ProgramOptions.h"


int main(int argc, const char* argv[]) 
{
    cc_mqtt5_client_afl_fuzz::ProgramOptions opts;
    opts.parseArgs(argc, argv);

    if (opts.helpRequested()) {
        opts.printHelp();
        return -1;
    }

    // TODO: generate input

    cc_mqtt5_client_afl_fuzz::Logger logger;
    if (!logger.open(opts)) {
        return -1;
    }

    cc_mqtt5_client_afl_fuzz::AflFuzz fuzz(opts, logger);
    if (!fuzz.init()) {
        return -1;
    }

    fuzz.run();
    return 0;
}