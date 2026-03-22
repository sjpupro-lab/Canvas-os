#include "QueryDispatcher.h"

namespace canvasos {

void QueryDispatcher::registerHandler(
    const std::string &method,
    QueryHandler handler) {
  handlers_[method] = std::move(handler);
}

std::string QueryDispatcher::dispatch(const std::string &jsonRequest) const {
  const auto key = jsonRequest.find("\"method\"");
  if (key == std::string::npos) {
    return R"({"ok":false,"error":"invalid JSON"})";
  }

  const auto colon = jsonRequest.find(':', key);
  const auto firstQuote = jsonRequest.find('"', colon == std::string::npos ? key : colon);
  if (firstQuote == std::string::npos) {
    return R"({"ok":false,"error":"invalid JSON"})";
  }

  const auto secondQuote = jsonRequest.find('"', firstQuote + 1);
  if (secondQuote == std::string::npos) {
    return R"({"ok":false,"error":"invalid JSON"})";
  }

  std::string method = jsonRequest.substr(firstQuote + 1, secondQuote - firstQuote - 1);
  auto it = handlers_.find(method);
  if (it == handlers_.end()) {
    return R"({"ok":false,"error":"unknown method"})";
  }

  return it->second(jsonRequest);
}

}  // namespace canvasos
