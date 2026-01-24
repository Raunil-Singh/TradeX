#include <random>
#include <map>
#include <vector>
#include <cmath>

constexpr int ORDERS{5'000'000};
enum TYPE{BUY, SELL};


std::uniform_int_distribution<> quantityDistribution(1, 1000);
std::uniform_int_distribution<> buySellDistribution(0, 1);
std::uniform_real_distribution<> priceDistribution(99.00, 101.00);
std::mt19937 rGen(std::random_device{}());


std::vector<int> quantityVector(ORDERS, quantityDistribution(rGen));
std::vector<double> priceVector(ORDERS, priceDistribution(rGen));
std::vector<int> type(ORDERS, buySellDistribution(rGen));

int main(){
    std::map<double, int> buyOrderMap;
    std::map<double, int> sellOrderMap;
    for(int i = 0; i < ORDERS; i++){
        double bucket = (static_cast<int>(std::round(priceVector[i] * 100))) / 100.0;
        if(type[i] == BUY){
            buyOrderMap[bucket] += quantityVector[i];
        }
        else{
            sellOrderMap[bucket] += quantityVector[i];
        }
    }
}
/*Dont know how to round off doubles to within two decimal precision to create an aggregated array*/
