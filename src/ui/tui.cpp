#include "tui.hpp"
#include <httplib.h>
#include <iostream>
#include <sstream>

namespace tui {
void run(int port) {
  httplib::Client cli("127.0.0.1", port);
  std::string line;
  while (true) {
    std::cout << "Price (cents): ";
    if (!std::getline(std::cin, line)) break;
    if (line == "q") break;
    if (line == "o") {
      if (auto res = cli.Post("/command", "{\"open\":40}", "application/json"))
        std::cout << res->body << std::endl;
      continue;
    }
    if (line == "c") {
      if (auto res = cli.Post("/command", "{\"close\":40}", "application/json"))
        std::cout << res->body << std::endl;
      continue;
    }
    int price = 0;
    try { price = std::stoi(line); } catch (...) { std::cout << "bad input\n"; continue; }
    std::cout << "Deposit (cents): ";
    std::string dep;
    if (!std::getline(std::cin, dep)) break;
    int deposit = 0;
    try { deposit = std::stoi(dep); } catch (...) { std::cout << "bad input\n"; continue; }
    std::ostringstream oss;
    oss << "{\"price\":" << price << ",\"deposit\":" << deposit << "}";
    if (auto res = cli.Post("/txn", oss.str(), "application/json"))
      std::cout << res->body << std::endl;
    else
      std::cout << "error" << std::endl;
  }
}
} // namespace tui
