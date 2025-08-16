#pragma once
#include <string>

namespace journal {
struct Txn { std::string id; int price{0}, deposit{0}, change{0}, quarters{0}, dispensed{0}; std::string phase; std::string reason; };
bool ensure_data_dir();
bool append(const Txn& t);
bool load_last(Txn& out);
std::string to_json(const Txn& t);
}
