#ifndef FTPSERVER_UTIL_H
#define FTPSERVER_UTIL_H

#define NUM_DIVISIONS 20

#include <iostream>

class Util {
public:
    template<typename T>
    static void showProgressBar(T current, T total) {
        if (total == 0) return;

        const double ratio = static_cast<double>(current) / static_cast<double>(total);
        const int filled = static_cast<int>(ratio * NUM_DIVISIONS);
        const int notFilled = NUM_DIVISIONS - filled;

        std::cout << "\r[";
        for (int i = 0; i < filled; ++i) std::cout << "#";
        for (int i = 0; i < notFilled; i++) std::cout << "-";
        std::cout << "] " << static_cast<int>(ratio * 100) << "%" << std::flush;
    }
};

#endif //FTPSERVER_UTIL_H
