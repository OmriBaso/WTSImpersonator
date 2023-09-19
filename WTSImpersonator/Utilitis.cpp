#include "Utilitis.h"



std::string Utilitis::generateRandomString() {
    // Initialize a random engine with a seed based on the current time
    static std::mt19937 engine(static_cast<unsigned int>(std::time(nullptr)));

    // Distribute uniformly from 4 to 6 for length
    std::uniform_int_distribution<int> lengthDist(4, 6);

    // Distribute uniformly from 0 to 25 for small letters and 26 to 51 for capital letters
    std::uniform_int_distribution<int> charDist(0, 51);

    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    int length = lengthDist(engine);

    std::string result;

    for (int i = 0; i < length; ++i) {
        int randomIndex = charDist(engine);
        result += charset[randomIndex];
    }

    return result;
}