#include <iostream>
#include <string>
using namespace std;

template<typename Source, typename Destination>
struct Into;

template<typename F, typename T>
constexpr T into(const F value) noexcept {
    Into<F, T>::into(value); // just forwarding to the struct
}

template<uint64_t N>
struct Into<cxx::string<N>, std::string> {
static into(const cxx::string<N> & value) {
    auto arbitraryType = value;
    string newString = to_string(arbitraryType);

}

string intToStdString(int source){
    string newString;
    newString = to_string(source);
    return newString;
}
string floatToStdString(float source){
    string newString;
    newString = to_string(source);
    return newString;
}
string charToStdString(char source){
    string newString;
    newString = to_string(source);
    return newString;
}
int main() {
    int exampleInt = 1;
    float exampleFloat = 1.0;
    char exampleChar = 'a';
    string newString = intToStdString(exampleInt);
    string newString2 = floatToStdString(exampleFloat);
    string newString3 = charToStdString(exampleChar);


    return 0;
}


