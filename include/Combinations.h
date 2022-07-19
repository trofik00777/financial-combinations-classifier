#pragma once

#include "Component.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

struct Date;

class Combinations
{
public:
    Combinations() = default;

    bool load(const std::filesystem::path & resource);

    std::string classify(const std::vector<Component> & components, std::vector<int> & order) const;

    struct Combination
    {
        struct Leg
        {
            InstrumentType m_type;
            std::variant<double, char> m_ratio;
            std::variant<bool, char, int> m_strike = false;
            std::variant<bool, char, std::string, int> m_expiration = false;

            bool equals(const Component & comp) const
            {
                if (m_type != comp.type) {
                    return false;
                }

                if (std::holds_alternative<char>(m_ratio)) {
                    char sign = std::get<char>(m_ratio);
                    if ((sign == '+' && comp.ratio <= 0) || (sign == '-' && comp.ratio >= 0)) {
                        return false;
                    }
                }
                else {
                    double weight = std::get<double>(m_ratio);
                    if (std::abs(weight - comp.ratio) >= std::numeric_limits<double>::epsilon()) {
                        return false;
                    }
                }

                return true;
            }
        };

        struct Checker
        {
            virtual bool check(const Combinations::Combination &, const std::vector<Component> &, std::vector<int> &) = 0;
            bool check_dates(const Date & t1, const Date & t2, char duration, int count);

            template <class T>
            bool check_order_vec(const std::vector<T> & vec, const T & neutral_elem)
            {
                T prev = neutral_elem;

                for (const T & num : vec) {
                    if (num != neutral_elem) {
                        if (num <= prev) {
                            return false;
                        }
                        prev = num;
                    }
                }
                return true;
            }

            virtual ~Checker() = default;
        };
        struct CheckerFixed : Checker
        {
            bool check(const Combinations::Combination & combination, const std::vector<Component> & components, std::vector<int> & order) override;
        };
        struct CheckerMultiple : CheckerFixed
        {
            bool check(const Combinations::Combination & combination, const std::vector<Component> & components, std::vector<int> & order) override;
        };
        struct CheckerMore : Checker
        {
            bool check(const Combinations::Combination & combination, const std::vector<Component> & components, std::vector<int> & order) override;
        };

        std::string m_name;
        std::string m_short_name;
        std::string m_id;
        std::unique_ptr<Checker> checker;
        unsigned mincount;

        std::vector<Leg> m_legs;
    };

private:
    std::vector<Combination> m_combinations;
};
