#include "tinyformat.h"
#include <locale>

namespace {

char decimal_point_1 = '.';
char thousands_sep_1 = ',';
std::string decimal_point_2;
std::string thousands_sep_2;
std::string grouping_1;

struct my_numpunct : std::numpunct<char> {
    char do_decimal_point() const override {
        return decimal_point_1;
    }
    char do_thousands_sep() const override {
        return thousands_sep_1;
    }
    std::string do_grouping() const override {
        return grouping_1;
    }
};

std::locale locale_1(std::locale::classic(), new my_numpunct);

}

namespace tinyformat {

void setNumericFormat(const std::string& decimal_point, const std::string& thousands_sep, const std::string& grouping) {
    if (decimal_point.size() == 1) {
        decimal_point_1 = decimal_point[0];
        decimal_point_2.clear();
    } else {
        decimal_point_1 = 1; // ad-hoc
        decimal_point_2 = decimal_point;
    }

    std::string grouping_0;

    for (auto c : grouping) {
        if (c == 0 || c == '0') {
            grouping_0.clear();
            break;
        }
        grouping_0.push_back((c >= '1' && c <= '9') ? (c - '0') : c);
    }

    if (thousands_sep.empty() || grouping_0.empty()) {
        thousands_sep_1 = ',';
        thousands_sep_2.clear();
        grouping_1.clear();
    } else {
        if (thousands_sep.size() == 1) {
            thousands_sep_1 = thousands_sep[0];
            thousands_sep_2.clear();
        } else {
            thousands_sep_1 = 2; // ad-hoc
            thousands_sep_2 = thousands_sep;
        }
        grouping_1 = grouping_0;
    }
}

namespace detail {

void imbue(std::ostream& o) {
    o.imbue(locale_1);
}

bool needReplace() {
    return !decimal_point_2.empty() || !thousands_sep_2.empty();
}

std::string doReplace(const std::string& src) {
    std::string ret;

    for (auto c : src) {
        if (c == 1) ret += decimal_point_2; // ad-hoc
        else if (c == 2) ret += thousands_sep_2; // ad-hoc
        else ret.push_back(c);
    }

    return ret;
}

}

}
