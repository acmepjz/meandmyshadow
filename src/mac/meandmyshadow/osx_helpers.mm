//
//  osx_helpers.m
//  meandmyshadow
//
#include <string>
#import <CoreFoundation/CoreFoundation.h>

std::string get_data_path () {
    NSBundle* myBundle = [NSBundle mainBundle];
    return std::string([[myBundle resourcePath] UTF8String])+"/data/";
}