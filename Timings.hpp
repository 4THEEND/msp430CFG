#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>


struct Timings
{
    std::vector<int> trace;

    Timings(): trace() {};
    
    static std::vector<int> parseCSV(std::istream& str){
        std::vector<int> result{};
        std::string line{};
        std::getline(str, line);

        std::stringstream lineStream(line);
        std::string cell;

        while(std::getline(lineStream,cell, ',')){
            result.push_back(std::stoi(cell));
        }
        return result;
    };

    void loadTrace(std::string filename){
        std::ifstream file(filename);
        if (!file){
            std::cerr << "Failed to open file: " << filename << "\n";
            return;
        }
        trace = parseCSV(file);
        std::cout << "Sucessfully imported the csv trace! (length: " << trace.size() << ")\n";
    };

    int trace_length(){ return trace.size(); };

    int operator[](int idx) {
        return trace[idx];
    };
};
