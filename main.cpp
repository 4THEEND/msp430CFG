#include <iostream>
#include <vector>
#include <functional>
#include <map>
#include <memory>
#include <tuple>

#include <argparse/argparse.hpp>

#include "binary_loader.hpp"
#include "cfg.hpp"


std::vector<int> parseCSV(std::istream& str)
{
    std::vector<int> result{};
    std::string line{};
    std::getline(str, line);

    std::stringstream lineStream(line);
    std::string cell;

    while(std::getline(lineStream,cell, ','))
    {
        result.push_back(std::stoi(cell));
    }
    return result;
}


struct Timings
{
    std::vector<int> trace;
    int t_offset;

    Timings(): trace(), t_offset() {};

    inline void reset_offset(){ t_offset = 0; };
    void loadTrace(std::string filename);
};


void Timings::loadTrace(std::string filename){
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }
    trace = parseCSV(file);
    for(int i = 0; i < 10; i++)
        std::cout << trace[i] << "\n";
    std::cout << "Sucessfully imported the csv trace! (length: " << trace.size() << ")\n";
}

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
 

void parse_args(argparse::ArgumentParser& program, int argc, char** argv){
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return;
    }

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

    parse_args(program, argc, argv);

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

    parse_args(program, argc, argv);


    if(files.find(program.get("binary")) != files.end()){
        active_file = program.get("binary");
        std::cout << active_file << " successfully selected\n";
    } else {
        std::cout << "Can't find this binary\n";
    }
}


void disassemble_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("disassemble");

    program.add_argument("-s", "--symbols")
        .help("Symbols where we want to start our recursive disassembly")
        .nargs(0, -1);

    parse_args(program, argc, argv);

    auto symbols_to_start = program.get<std::vector<std::string>>("-s");
    auto it = files.find(active_file);

    if(it == files.end()){
        std::cout << "Unable to select this file\n";
        return;
    }

    std::get<cfg>(it->second).disassemble(symbols_to_start);
    std::cout << "Successfuly disassembled the binary!\n";
}


void export_callback(int argc, char** argv, Files& files, std::string& active_file){
    argparse::ArgumentParser program("export");

    program.add_argument("output_name")
        .help("Name of the output dot file")
        .default_value("cfg.dot");

    parse_args(program, argc, argv);

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
        .scan<'u', unsigned int>();

    program.add_argument("destination")
        .help("Destination address")
        .scan<'u', unsigned int>();

    parse_args(program, argc, argv);

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

    parse_args(program, argc, argv);

    auto it = files.find(active_file);
    if(it == files.end()){
        std::cout << "Unable to select this file\n";
        return;
    }

    std::get<Timings>(it->second).loadTrace(program.get("csv"));
}



int main(int argc, char** argv){
    std::string param{};
    std::string active_file{};
    Files loaded_files{};

    std::map<std::string, std::function<void(int, char**, Files&, std::string&)>> commands{
        {"load", load_callback},
        {"show", show_callback},
        {"select", select_callback},
        {"disassemble", disassemble_callback},
        {"export", export_callback},
        {"add-edge", add_edge_callback},
        {"import", import_callback},
    };

    while(true){
        std::cout << "$ ";
        std::getline(std::cin, param); 

        std::vector<std::string> params(split(param));
        auto it = commands.find(params[0]);

        if(params[0] == "exit"){
            break;
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
