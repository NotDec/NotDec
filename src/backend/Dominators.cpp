
#include "backend/Dominators.h"

namespace notdec::backend {
template <> void CFGDominatorTreeImpl</*IsPostDom=*/true>::anchor() {}

template <> void CFGDominatorTreeImpl</*IsPostDom=*/false>::anchor() {}

} // namespace notdec::backend
