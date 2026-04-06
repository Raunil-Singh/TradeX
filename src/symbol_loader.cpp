#include "symbol_loader.h"

#include <fstream>
#include <sstream>

static uint64_t calculateLower(uint64_t ltp) {
    return ltp * 90 / 100;     
}

static uint64_t calculateUpper(uint64_t ltp) {
    return ltp * 110 / 100;     
}

std::vector<SymbolInfo> loadSymbolCSV(const std::string& filename) {
    std::vector<SymbolInfo> symbols;

    std::ifstream file(filename);
    std::string line;

    // skip header
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;

        SymbolInfo info;

        std::getline(ss, info.symbol, ',');

        std::getline(ss, token, ',');
        info.symbol_id = std::stoul(token);

        std::getline(ss, token, ',');
        info.ltp = std::stoull(token);

        std::getline(ss, token, ',');
        info.lower_limit = std::stoull(token);

        std::getline(ss, token, ',');
        info.upper_limit = std::stoull(token);

        symbols.push_back(info);
    }

    return symbols;
}

void saveEndOfDayCSV(const std::string& filename, shared_data::MarketState* market_state, const std::vector<SymbolInfo>& symbols) {
    std::ofstream file(filename);

    file << "symbol,symbol_id,ltp,lower_limit,upper_limit\n";

    for (const auto& s : symbols) {
        uint64_t latest_ltp = market_state->last_price[s.symbol_id].load();

        if (latest_ltp == 0) latest_ltp = s.ltp;

        uint64_t lower = calculateLower(latest_ltp);
        uint64_t upper = calculateUpper(latest_ltp);

        file << s.symbol << ","<< s.symbol_id << ","<< latest_ltp << ","<< lower << ","<< upper << "\n";
    }
}