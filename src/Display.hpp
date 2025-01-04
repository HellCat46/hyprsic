//
// Created by hellcat on 4/1/25.
//

#ifndef DISPLAY_H
#define DISPLAY_H

#include "iostream"
#include "math.h"

std::string DisplayBytes(double bytes){
    std::string str;

    if(bytes>pow(1024,3)){ // GigaBytes
        str = std::to_string(bytes/pow(1024,3)) + " GiB";
    }else if(bytes>pow(1024,2)){ // MegaBytes
        str = std::to_string(bytes/pow(1024,2)) + " MiB";
    }else if(bytes>1024){ // KiloBytes
        str = std::to_string(bytes/1024) + " KiB";
    }else { // Bytes
        str = std::to_string(bytes) + " B";
    }

    return str;
}

#endif //DISPLAY_H
