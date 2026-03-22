#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace canvasos {

using QueryHandler = std::function<std::string(const std::string &)>;

class QueryDispatcher {
 public:
  void registerHandler(const std::string &method, QueryHandler handler);
  std::string dispatch(const std::string &jsonRequest) const;

 private:
  std::unordered_map<std::string, QueryHandler> handlers_;
};

}  // namespace canvasos
