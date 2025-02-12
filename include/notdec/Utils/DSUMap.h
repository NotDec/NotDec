
#include <map>
#include <vector>
namespace notdec {

/// Map with Disjoint Set Union. Multiple keys can be mapped to the same value.
/// Merge operation: move all keys that are mapped to one value to another new
/// value.
template <class K, class V> class DSUMap {
  std::map<K, V> M;
  std::map<V, std::vector<K>> Rev;

public:
  void merge(V From, V To) {
    if (From == To) {
      return;
    }
    if (Rev.count(From) == 0) {
      return;
    }
    for (auto Key : Rev[From]) {
      // assert(M.at(Key) == From);
      M.insert_or_assign(Key, To);
      Rev[To].push_back(Key);
    }
    Rev.erase(From);
  }
  const std::map<V, std::vector<K>>& rev() const { return Rev; }

  template <class... Args>
  auto emplace(Args &&...args)
      -> decltype(M.emplace(std::forward<Args>(args)...)) {
    auto [It, Success] = M.emplace(std::forward<Args>(args)...);
    if (Success) {
      Rev[It->second].push_back(It->first);
    }
    return {It, Success};
  }

  auto insert(K Key, V Val) -> decltype(M.insert({Key, Val})) {
    auto [It, Success] = M.insert({Key, Val});
    if (Success) {
      Rev[It->second].push_back(It->first);
    }
    return {It, Success};
  }

  auto begin() { return M.begin(); }
  auto end() { return M.end(); }

  auto count(K Key) -> decltype(M.count(Key)) { return M.count(Key); }
  auto at(K Key) -> decltype(M.at(Key)) { return M.at(Key); }
  auto at(K Key) const -> const decltype(M.at(Key)) { return M.at(Key); }
};

} // namespace notdec
