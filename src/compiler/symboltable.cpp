#include "symboltable.h"

namespace frontend {

SymbolTable::SymbolTable() {
  // start with a single global scope
  scopes_.clear();
  scopes_.emplace_back();
}

size_t SymbolTable::push_scope(std::string name) {
  (void)name; // name currently unused, kept for diagnostics later
  scopes_.emplace_back();
  return scopes_.size() - 1;
}

void SymbolTable::pop_scope() {
  if (scopes_.size() > 1) {
    scopes_.pop_back();
  }
}

bool SymbolTable::insert(std::shared_ptr<SymbolEntry> entry) {
  if (!entry) return false;
  auto &scope = scopes_.back();
  auto it = scope.find(entry->name);
  if (it != scope.end()) return false; // already exists in current scope
  entry->scope_index = scopes_.size() - 1;
  scope.emplace(entry->name, std::move(entry));
  return true;
}

std::optional<std::shared_ptr<SymbolEntry>> SymbolTable::lookup(const std::string &name, bool current_scope_only) const {
  if (scopes_.empty()) return std::nullopt;
  if (current_scope_only) {
    auto &scope = scopes_.back();
    auto it = scope.find(name);
    if (it != scope.end()) return it->second;
    return std::nullopt;
  }

  for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
    const auto &scope = scopes_[static_cast<size_t>(i)];
    auto it = scope.find(name);
    if (it != scope.end()) return it->second;
  }
  return std::nullopt;
}

void SymbolTable::clear() {
  scopes_.clear();
  scopes_.emplace_back();
}

} // namespace frontend
