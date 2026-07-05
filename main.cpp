#include <iostream>
#include <vector>
#include <functional>
#include <map>
#include <memory>
#include <tuple>
#include <fstream>

#include <argparse/argparse.hpp>

#include "binary_loader.hpp"
#include "cfg.hpp"
#include "Timings.hpp"


using BinaryContext = std::tuple<std::shared_ptr<BinaryLoader>, cfg, Timings>;
using Files = std::map<std::string, BinaryContext>;


std::vector<std::string> split(std::string str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);

    std::string token;
    while (iss >> token)
        tokens.push_back(token);

    return tokens;
}
 

bool parse_args(argparse::ArgumentParser& program, int argc, char** argv){
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return false;
    }
    return true;
}


void load_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("load");

    program.add_argument("binary")
        .help("Binary to load");

    program.add_argument("--dump")
        .help("Using the dump loader")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-e", "--entry-point")
        .help("The address of the entrypoint (when using the dump loader)")
        .scan<'u', unsigned int>()
        .default_value(0);

    program.add_argument("-o", "--offset")
        .help("The PC offset to consider (when using the dump loader)")
        .scan<'u', unsigned int>()
        .default_value(0);

    if(!parse_args(program, argc, argv))
        return;

    std::shared_ptr<BinaryLoader> my_file{};
    
    if(program["--dump"] == false)
        my_file = std::make_shared<ELFFile>();
    else
        my_file = std::make_shared<DumpFile>(program.get<unsigned int>("-e"), program.get<unsigned int>("-o"));

        
    std::string file_name{ program.get("binary") };

    if(!my_file->loadBinary(file_name))
        return;

    if(!my_file->getEntryOffset())
        return;

    my_file->parseSymbolTable();

    files.insert({file_name, std::make_tuple(my_file, cfg(my_file), Timings())});
    active_file = file_name;

    std::cout << "File sucessfully loaded and active!!\n";
    std::cout << "Type \"show\" to see loaded binaries\n";
}


void show_callback(int argc, char** argv, Files& files, std::string& active_file){
    for(const auto& it : files){
        if(it.first == active_file)
            std::cout << "[*] ";
        else 
            std::cout << "[ ] ";
        std::cout << it.first << "\n";
    }
}


void select_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("select");

    program.add_argument("binary")
        .help("Binary to select");

    if(!parse_args(program, argc, argv))
        return;


    if(files.find(program.get("binary")) != files.end()){
        active_file = program.get("binary");
        std::cout << active_file << " successfully selected\n";
    } else {
        std::cout << "Can't find this binary\n";
    }
}


void disassemble_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("disasm");

    program.add_argument("-s", "--symbols")
        .help("Symbols where we want to start our recursive disassembly")
        .nargs(0, -1);

    program.add_argument("--use-symbols")
        .help("Use this flag when you just want to disasseble from the entrypoint")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--no-add")
        .help("Flag used when you want to reconstruct the entire cfg")
        .default_value(true)
        .implicit_value(false);

    program.add_argument("--entry")
        .help("Begining address for the disassembly")
        .scan<'x', unsigned int>();

    if(!parse_args(program, argc, argv))
        return;

    auto symbols_to_start = program.get<std::vector<std::string>>("-s");
    auto it = files.find(active_file);

    if(it == files.end()){
        std::cout << "Unable to select this file\n";
        return;
    }

    uint32_t val = 0xbeef;
    if (program.present<unsigned int>("--entry")) {
        val = program.get<unsigned int>("--entry");
    }

    std::get<cfg>(it->second).disassemble(symbols_to_start, val, program.get<bool>("--use-symbols"), program.get<bool>("--no-add"));
    std::cout << "Successfuly disassembled the binary!\n";
}


void export_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("export");

    program.add_argument("output_name")
        .help("Name of the output dot file")
        .default_value("cfg.dot");

    if(!parse_args(program, argc, argv))
        return;

    auto it = files.find(active_file);
    if(it == files.end()){
        std::cout << "Unable to select this file\n";
        return;
    }

    std::get<cfg>(it->second).exportCFGToDOT(program.get("output_name"));
}


void add_edge_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("add-edge");

    program.add_argument("source")
        .help("Source address")
        .scan<'x', unsigned int>();

    program.add_argument("destination")
        .help("Destination address")
        .scan<'x', unsigned int>();

    if(!parse_args(program, argc, argv))
        return;

    auto it = files.find(active_file);
    if(it == files.end()){
        std::cout << "Unable to select this file\n";
        return;
    }

    std::get<cfg>(it->second).add_edge(program.get<unsigned int>("source"), program.get<unsigned int>("destination"));
}


void import_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("import");

    program.add_argument("csv")
        .help("CSV of instruction timings to be imported");

    if(!parse_args(program, argc, argv))
        return;

    auto it = files.find(active_file);
    if(it == files.end()){
        std::cout << "Unable to select this file\n";
        return;
    }

    std::get<Timings>(it->second).loadTrace(program.get("csv"));
}


void walkthrough_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("walkthrough");

    program.add_argument("source")
        .help("Begining address for the walkthrough (must be at the start of a BB)")
        .scan<'x', unsigned int>();

    if(!parse_args(program, argc, argv))
        return;

    auto it = files.find(active_file);
    if(it == files.end()){
        std::cout << "Unable to select this file\n";
        return;
    }

    std::get<cfg>(it->second).walkthrough(program.get<unsigned int>("source"), std::get<Timings>(it->second));
}



int main(int argc, char** argv){
    std::string param{};
    std::string active_file{};
    Files loaded_files{};

    std::map<std::string, std::function<void(int, char**, Files&, std::string&)>> commands{
        {"load", load_callback},
        {"show", show_callback},
        {"select", select_callback},
        {"disasm", disassemble_callback},
        {"export", export_callback},
        {"add-edge", add_edge_callback},
        {"import", import_callback},
        {"walkthrough", walkthrough_callback},
    };

    while(true){
        std::cout << "$ ";
        std::getline(std::cin, param); 

        std::vector<std::string> params(split(param));
        if(params.empty())
            continue;

        auto it = commands.find(params[0]);

        if(params[0] == "exit"){
            break;
        } 
        else if(params[0] == "help"){
            std::cout << "Commands avaiable:\n";
            for(const auto& [command, f] : commands)
                std::cout << "-" << command << "\n";
        }
        else if(it != commands.end()){
            std::vector<char*> cstrings;
            cstrings.reserve(params.size());

            for(size_t i = 0; i < params.size(); ++i)
                cstrings.push_back(const_cast<char*>(params[i].c_str()));

            it->second(cstrings.size(), &cstrings[0], loaded_files, active_file);
        } else {
            std::cout << "invalid command!\n";
        }
    }

    return 0;
}
