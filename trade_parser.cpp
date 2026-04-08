#include <fstream>
#include <iostream>
#include <string>
#include "trade.h"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage : ./parse [FILE_NAME] \n";
        return 1;
    }

    char* name = argv[1];
    std::string path = "./file_output/" + std::string(name);
    std::ifstream file(path.data(), std::ios::binary);

    if (!file)
    {
        std::cerr << "error in opening file \n";
        return 1;
    }

    matching_engine::Trade trade;
    int count{};
    while(file.read(reinterpret_cast<char*>(&trade), sizeof(trade)))
    {
        count++;
        std::cout<<"Trade id : " << trade.trade_id << "\n";
    }
    std::cout<<"Total trades: " << count << "\n";
}

