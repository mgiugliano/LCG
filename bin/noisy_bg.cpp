#include <stdlib.h>
#include <sys/types.h>

#include <vector>
#include <string>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include "types.h"
#include "utils.h"
#include "entity.h"
#include "engine.h"
#include "stimulus_generator.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NOISY_BG_CLONE_VERSION 0.1
#define NOISY_BG_BANNER \
        "\n\tArbitrary Function Stimulator with noisy background\n" \
        "\nAuthor: Daniele Linaro (daniele@tnb.ua.ac.be)\n"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using boost::property_tree::ptree;
using namespace dynclamp;

struct OUoptions {
        std::string sigma, tau, E, G0;
};

struct options {
        useconds_t iti, ibi;
        uint nTrials, nBatches;
        std::vector<std::string> stimulusFiles;
        OUoptions ou[2];
};

bool parseConfigFile(const std::string& configfile, options *opt)
{
        ptree pt;
        int i;
        try {
                read_xml(configfile, pt);
                i = 0;
                BOOST_FOREACH(ptree::value_type &v,
                              pt.get_child("dynamicclamp.entities")) {
                        std::cout << v.first << std::endl;
                        opt->ou[i].sigma = v.second.get<std::string>("sigma");
                        opt->ou[i].tau = v.second.get<std::string>("tau");
                        opt->ou[i].E = v.second.get<std::string>("E");
                        opt->ou[i].G0 = v.second.get<std::string>("G0");
                        if (++i == 2)
                                break;
                }
        } catch(std::exception e) {
                Logger(Critical, "Error while parsing configuration file: %s.\n", e.what());
                return false;
        }
        return true;
}

void parseArgs(int argc, char *argv[], options *opt)
{

        double iti, ibi;
        uint nTrials, nBatches;
        std::string stimfile, stimdir, configfile;
        po::options_description description(
                        NOISY_BG_BANNER
                        "\nAllowed options");
        po::variables_map options;

        try {
                description.add_options()
                        ("help,h", "print help message")
                        ("version,v", "print version number")
                        ("config-file,c", po::value<std::string>(&configfile)->default_value("noisy_bg.xml"), "specify configuration file")
                        ("iti,i", po::value<double>(&iti)->default_value(0.25), "specify inter-trial interval (default: 0.25 sec)")
                        ("ibi,I", po::value<double>(&ibi)->default_value(0.25), "specify inter-batch interval (default: 0.25 sec)")
                        ("ntrials,n", po::value<uint>(&nTrials)->default_value(1), "specify the number of trials (how many times a stimulus is repeated)")
                        ("nbatches,N", po::value<uint>(&nBatches)->default_value(1), "specify the number of trials (how many times a batch of stimuli is repeated)")
                        ("stimfile,f", po::value<std::string>(&stimfile), "specify the stimulus file to use")
                        ("stimdir,d", po::value<std::string>(&stimdir), "specify the directory where stimulus files are located");

                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    

                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }

                if (options.count("version")) {
                        std::cout << fs::path(argv[0]).filename() << " version " << NOISY_BG_CLONE_VERSION << std::endl;
                        exit(0);
                }

                if (!fs::exists(configfile) || !parseConfigFile(configfile,opt)) {
                        std::cout << "Configuration file \"" << configfile << "\" not found or invalid. Aborting...\n";
                        exit(1);
                }

                if (options.count("stimfile")) {
                        if (!fs::exists(stimfile)) {
                                std::cout << "Stimulus file \"" << stimfile << "\" not found. Aborting...\n";
                                exit(1);
                        }
                        opt->stimulusFiles.push_back(stimfile);
                }
                else if (options.count("stimdir")) {
                        if (!fs::exists(stimdir)) {
                                std::cout << "Directory \"" << stimdir << "\" not found. Aborting...\n";
                                exit(1);
                        }
                        for (fs::directory_iterator it(stimdir); it != fs::directory_iterator(); it++) {
                                if (! fs::is_directory(it->status())) {
                                        opt->stimulusFiles.push_back(it->path().string());
                                }
                        }
                }
                else {
                        std::cout << description << "\n";
                        std::cout << "ERROR: you have to specify one of [stimulus file] or [stimulus directory]. Aborting...\n";
                        exit(1);
                }

                opt->iti = (useconds_t) (iti * 1e6);
                opt->ibi = (useconds_t) (ibi * 1e6);
                opt->nTrials = nTrials;
                opt->nBatches = nBatches;

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(2);
        }

}

void runStimulus(const std::string& stimfile, OUoptions *opt)
{
        Logger(Info, "Processing stimulus file [%s].\n", stimfile.c_str());

#ifdef HAVE_LIBCOMEDI

        std::vector<Entity*> entities;
        dictionary parameters;
        double tend;
        int i;

        try {
                // entity[0]
                parameters["compress"] = "false";
                entities.push_back( EntityFactory("H5Recorder", parameters) );
                
                // entity[1]
                parameters.clear();
                parameters["deviceFile"] = "/dev/comedi0";
                parameters["inputSubdevice"] = "0";
                parameters["outputSubdevice"] = "1";
                parameters["readChannel"] = "2";
                parameters["writeChannel"] = "1";
                parameters["inputConversionFactor"] = "20";
                parameters["outputConversionFactor"] = "0.0025";
                parameters["spikeThreshold"] = "-20";
                parameters["V0"] = "-57";
                entities.push_back( EntityFactory("RealNeuron", parameters) );

                // entity[2]
                parameters.clear();
                parameters["filename"] = stimfile;
                entities.push_back( EntityFactory("Stimulus", parameters) );

                // entity[3] & entity[4]
                for (i=0; i<2; i++) {
                        parameters.clear();
                        parameters["sigma"] = opt[i].sigma;
                        parameters["tau"] = opt[i].tau;
                        parameters["E"] = opt[i].E;
                        parameters["G0"] = opt[i].G0;
                        entities.push_back( EntityFactory("OUconductance", parameters) );
                }

                // log everything
                for (i=1; i<entities.size(); i++)
                        entities[i]->connect(entities[0]);

                // connect stimuli to the neuron
                for (i=2; i<entities.size(); i++)
                        entities[i]->connect(entities[1]);

                tend = dynamic_cast<generators::Stimulus*>(entities[2])->duration();

        } catch (const char *msg) {
                Logger(Critical, "Error: %s.\n", msg);
                exit(1);
        }

        Simulate(entities, tend);

        for (uint i=0; i<entities.size(); i++)
                delete entities[i];
#endif
}

int main(int argc, char *argv[])
{
        options opt;
        int i, j, k;

        SetLoggingLevel(Info);

        parseArgs(argc, argv, &opt);

        Logger(Info, NOISY_BG_BANNER);
        Logger(Info, "Number of batches: %d.\n", opt.nBatches);
        Logger(Info, "Number of trials: %d.\n", opt.nTrials);
        Logger(Info, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);
        Logger(Info, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

        for (i=0; i<opt.nBatches; i++) {
                for (j=0; j<opt.stimulusFiles.size(); j++) {
                        for (k=0; k<opt.nTrials; k++) {
                                ResetGlobalTime();
                                runStimulus(opt.stimulusFiles[j], opt.ou);
                                if (k != opt.nTrials-1)
                                        usleep(opt.iti);
                        }
                        if (j != opt.stimulusFiles.size()-1)
                                usleep(opt.iti);
                }
                if (i != opt.nBatches-1)
                        usleep(opt.ibi);
        }

        return 0;
}
