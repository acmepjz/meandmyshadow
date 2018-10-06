#include "tinyformat.h"
#include <locale>

namespace tinyformat {

std::locale locale = std::locale::classic();

void setLocale(const char* std_name) {
    try {
        tfm::locale = std::locale(std_name);
    } catch (...) {
        std::cerr << "ERROR: Failed to create std::locale for '" << std_name << "'!" << std::endl;
        tfm::locale = std::locale::classic();
    }
}

void imbue(std::ostream& o) {
    o.imbue(tfm::locale);
}

}
