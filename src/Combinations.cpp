#include "Combinations.h"

#include "pugixml.hpp"

#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <string_view>

namespace {

static std::vector<int> days_count = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

} // namespace

struct Date
{
    int year = 0;
    int month = 0;
    int day = 1;

    Date() = default;

    Date(const std::tm & t)
        : year(t.tm_year)
        , month(t.tm_mon)
        , day(t.tm_mday)
    {
    }

    Date & operator++()
    {
        int k = (((1900 + year) % 4 == 0 && (1900 + year) % 100 != 0) || (1900 + year) % 400 == 0 ? 1 : 0);

        if (month == 1) { // february
            if (day == days_count[month] + k) {
                ++month;
                day = 1;
            }
            else {
                ++day;
            }
        }
        else {
            if (day == days_count[month]) {
                if (month == 11) {
                    ++year;
                    month = 0;
                }
                else {
                    ++month;
                }
                day = 1;
            }
            else {
                ++day;
            }
        }
        return *this;
    }
};

bool operator!=(const Date & t1, const Date & t2)
{
    return t2.year != t1.year || t1.month != t2.month || t1.day != t2.day;
}

bool operator==(const Date & t1, const Date & t2)
{
    return !(t1 != t2);
}

bool operator==(const Component & lhs, const Component & rhs)
{
    return lhs.type == rhs.type && lhs.expiration == rhs.expiration && std::abs(lhs.strike - rhs.strike) < std::numeric_limits<double>::epsilon() && std::abs(lhs.ratio - rhs.ratio) < std::numeric_limits<double>::epsilon();
}

bool operator<(const Date & t1, const Date & t2)
{
    return std::tie(t1.year, t1.month, t1.day) <
            std::tie(t2.year, t2.month, t2.day);
}

bool operator>(const Date & t1, const Date & t2)
{
    return std::tie(t1.year, t1.month, t1.day) >
            std::tie(t2.year, t2.month, t2.day);
}

bool operator<=(const Date & t1, const Date & t2)
{
    return !(t1 > t2);
}

bool Combinations::Combination::Checker::check_dates(const Date & t1, const Date & t2, char duration, int count)
{
    auto month1 = t1.year * 12 + t1.month;
    auto month2 = t2.year * 12 + t2.month;
    Date min, max;
    if (t1 < t2) {
        min = t1;
        max = t2;
    }
    else {
        min = t2;
        max = t1;
    }
    switch (duration) {
    case 'd':
        for (int i = 0; i < count; ++i) {
            ++min;
        }
        return min == max;
    case 'm':
        if (max.month == 2) {
            return min.day > 29 && std::abs(month1 - month2) - 1 == count;
        }
        return t1.day == t2.day && std::abs(month1 - month2) == count;
    case 'y':
        return t1.day == t2.day && t1.month == t2.month && std::abs(t1.year - t2.year) == count;
    case 'q':
        return (month2 - month1 == count * 3) || (month2 - month1 == count * 3 + 1 && t1.day > days_count[t2.month - 1]);
    }
    return false;
}

bool Combinations::Combination::CheckerFixed::check(const Combinations::Combination & combination, const std::vector<Component> & components, std::vector<int> & order)
{
    if (components.size() != combination.mincount) {
        return false;
    }
    order.resize(combination.mincount);

    std::vector<int> permut(combination.mincount);
    std::iota(permut.begin(), permut.end(), 0);

    std::vector<double> pluses(9, -1); // -4 -3 -2 -1 0 1 2 3 4
    std::map<char, double> chars;
    Date t;
    std::vector<Date> pluses_dates(9, t);
    std::map<char, Date> dates;

    Date prev_expiration;

    do {
        bool is_good = true;
        std::fill(pluses.begin(), pluses.end(), -1);
        std::fill(pluses_dates.begin(), pluses_dates.end(), t);
        chars.clear();
        dates.clear();
        prev_expiration = t;
        for (std::size_t i = 0; i < combination.mincount; ++i) {
            const Component & comp = components[permut[i]];
            const auto & curr_leg = combination.m_legs[i];
            Date comp_date = comp.expiration;
            if (!curr_leg.equals(comp)) {
                is_good = false;
                break;
            }

            if (comp.type == InstrumentType::O || comp.type == InstrumentType::P || comp.type == InstrumentType::C) {
                int count_pluses = 0;
                if (std::holds_alternative<bool>(curr_leg.m_strike)) {
                    count_pluses = 0;
                }
                else if (std::holds_alternative<char>(curr_leg.m_strike)) {
                    char ch = std::get<char>(curr_leg.m_strike);
                    const auto [found, inserted] = chars.try_emplace(ch, comp.strike);
                    if (!inserted) {
                        if (std::abs(found->second - comp.strike) > std::numeric_limits<double>::epsilon()) {
                            is_good = false;
                            break;
                        }
                    }
                }
                else {
                    count_pluses = std::get<int>(curr_leg.m_strike);
                }

                count_pluses += 4;

                if (count_pluses - 4 != 0 && pluses[count_pluses] != -1) {
                    if (std::abs(pluses[count_pluses] - comp.strike) > std::numeric_limits<double>::epsilon()) {
                        is_good = false;
                        break;
                    }
                }
                else {
                    pluses[count_pluses] = comp.strike;
                }
            }

            int count_pluses = 0;
            if (std::holds_alternative<bool>(curr_leg.m_expiration)) {
                prev_expiration = comp_date;
            }
            else if (std::holds_alternative<char>(curr_leg.m_expiration)) {
                char ch = std::get<char>(curr_leg.m_expiration);
                const auto [found, inserted] = dates.try_emplace(ch, comp_date);
                if (!inserted) {
                    if (found->second != comp_date) {
                        is_good = false;
                        break;
                    }
                }
                prev_expiration = comp_date;
            }
            else if (std::holds_alternative<int>(curr_leg.m_expiration)) {
                count_pluses = std::get<int>(curr_leg.m_expiration);
                prev_expiration = comp_date;
            }
            else {
                std::string str = std::get<std::string>(curr_leg.m_expiration);
                int count = 1;
                if (str[0] >= '0' && str[0] <= '9') {
                    count = atoi(str.c_str());
                }
                if (!check_dates(prev_expiration, comp_date, str[str.size() - 1], count)) {
                    is_good = false;
                    break;
                }
            }

            count_pluses += 4;
            if (count_pluses - 4 != 0 && pluses_dates[count_pluses] != t) {
                if (pluses_dates[count_pluses] != comp_date) {
                    is_good = false;
                    break;
                }
            }
            else {
                pluses_dates[count_pluses] = comp_date;
            }
        }

        if (is_good && check_order_vec(pluses, -1.0) && check_order_vec(pluses_dates, t)) {
            for (std::size_t i = 0; i < order.size(); ++i) {
                order[i] = permut[i] + 1;
            }
            return true;
        }
    } while (std::next_permutation(permut.begin(), permut.end()));
    order.clear();

    return false;
}

bool Combinations::Combination::CheckerMultiple::check(const Combinations::Combination & combination, const std::vector<Component> & components, std::vector<int> & order)
{
    int coef = (components.size() / combination.mincount);
    if (coef * combination.mincount != components.size()) {
        return false;
    }

    std::vector<Component> new_comps;
    std::vector<std::vector<std::size_t>> conv_indexes;

    for (std::size_t i = 0; i < components.size(); ++i) {
        bool flag = true;
        for (std::size_t j = 0; j < new_comps.size(); ++j) {
            if (components[i] == new_comps[j]) {
                flag = false;
                conv_indexes[j].push_back(i);

                break;
            }
        }

        if (flag) {
            new_comps.push_back(components[i]);
            conv_indexes.push_back({i});
        }
    }
    std::vector<int> for_answ;

    bool answer = Combinations::Combination::CheckerFixed::check(combination, new_comps, for_answ);

    if (!answer) {
        order.clear();
        return false;
    }

    order.resize(components.size());
    int for_answ_size = for_answ.size();
    for (int i = 0; i < for_answ_size; ++i) {
        int offset = 0;
        for (const auto j : conv_indexes[i]) {
            order[j] = for_answ[i] + offset;
            offset += for_answ_size;
        }
    }
    return true;
}

bool Combinations::Combination::CheckerMore::check(const Combinations::Combination & combination, const std::vector<Component> & components, std::vector<int> & order)
{
    if (components.size() < combination.mincount) {
        return false;
    }
    order.resize(components.size());

    for (const auto & component : components) {
        for (const auto & leg : combination.m_legs) {
            if (leg.m_type != component.type) {
                if (leg.m_type != InstrumentType::O || (component.type != InstrumentType::C && component.type != InstrumentType::P)) {
                    return false;
                }
            }

            if (std::holds_alternative<double>(leg.m_ratio) && component.ratio != std::get<double>(leg.m_ratio)) {
                return false;
            }
            else if (std::holds_alternative<char>(leg.m_ratio) && (std::get<char>(leg.m_ratio) == '+') != (component.ratio > 0)) {
                return false;
            }
        }
    }

    std::iota(order.begin(), order.end(), 1);
    return true;
}

bool Combinations::load(const std::filesystem::path & resource)
{
    pugi::xml_document document{};

    if (!document.load_file(resource.c_str())) {
        return false;
    }

    const pugi::xml_node & combinations = document.child("combinations");
    for (const pugi::xml_node & combination : combinations) {
        Combination comb{};
        comb.m_name = combination.attribute("name").value();
        comb.m_short_name = combination.attribute("shortname").value();
        comb.m_id = combination.attribute("identifier").value();

        std::vector<Combination::Leg> legs;
        for (const pugi::xml_node & xml_leg : combination.first_child()) {
            Combination::Leg leg{};

            leg.m_type = static_cast<InstrumentType>(xml_leg.attribute("type").value()[0]);
            std::string_view ratio = xml_leg.attribute("ratio").value();
            if (ratio == "+" || ratio == "-") {
                leg.m_ratio = ratio[0];
            }
            else {
                leg.m_ratio = std::atof(ratio.data());
            }

            if (xml_leg.attribute("strike") != nullptr) {
                leg.m_strike = xml_leg.attribute("strike").value()[0];
            }
            else if (xml_leg.attribute("strike_offset") != nullptr) {
                std::string_view strike_offset = xml_leg.attribute("strike_offset").value();
                int count = strike_offset.size();
                count *= (xml_leg.attribute("strike_offset").value()[0] == '+' ? 1 : -1);
                leg.m_strike = count;
            }

            if (xml_leg.attribute("expiration") != nullptr) {
                leg.m_expiration = xml_leg.attribute("expiration").value()[0];
            }
            else if (xml_leg.attribute("expiration_offset") != nullptr) {
                std::string_view expir_str = xml_leg.attribute("expiration_offset").value();

                if (expir_str[0] == '+' || expir_str[0] == '-') {
                    int count = expir_str.size();
                    count *= (expir_str[0] == '+' ? 1 : -1);
                    leg.m_expiration = count;
                }
                else {
                    leg.m_expiration = std::string(expir_str);
                }
            }
            legs.emplace_back(std::move(leg));
        }
        comb.m_legs = std::move(legs);

        std::string cardinality = combination.first_child().attribute("cardinality").value();
        if (cardinality == "fixed") {
            comb.checker = std::make_unique<Combinations::Combination::CheckerFixed>();
            comb.mincount = comb.m_legs.size();
        }
        else if (cardinality == "multiple") {
            comb.checker = std::make_unique<Combinations::Combination::CheckerMultiple>();
            comb.mincount = comb.m_legs.size();
        }
        else if (cardinality == "more") {
            comb.checker = std::make_unique<Combinations::Combination::CheckerMore>();
            comb.mincount = std::atoi(combination.first_child().attribute("mincount").value());
        }
        else {
            std::cerr << "Error while parsing xml cardinality...\n";
            return false;
        }

        m_combinations.emplace_back(std::move(comb));
    }
    return true;
}

std::string Combinations::classify(const std::vector<Component> & components, std::vector<int> & order) const
{
    for (const auto & combination : m_combinations) {
        if (combination.checker->check(combination, components, order)) {
            return combination.m_name;
        }
    }
    order.clear();
    return "Unclassified";
}
