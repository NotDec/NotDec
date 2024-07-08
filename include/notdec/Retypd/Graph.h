#ifndef _NOTDEC_RETYPD_GRAPH_H_
#define _NOTDEC_RETYPD_GRAPH_H_

#include "Retypd/Schema.h"
#include <llvm/ADT/StringRef.h>
#include <variant>

/*

/// This file contains the graph used for saturation and transducer in Appendix
D.
///

#[derive(PartialEq, Eq, Hash, Clone, PartialOrd, Ord)]
pub enum EdgeLabel {
    One,
    Forget { capability: LabelOrBase },
    Recall { capability: LabelOrBase },
}

impl Display for EdgeLabel {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            EdgeLabel::One => write!(f, "_1_"),
            EdgeLabel::Forget { capability } => write!(f, "forget {}",
capability), EdgeLabel::Recall { capability } => write!(f, "recall {}",
capability),
        }
    }
}

impl Debug for EdgeLabel {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self)
    }
}

#[derive(PartialEq, Eq, Hash, Clone)]
pub enum SideMark {
    None,
    Left,
    Right,
}

impl Display for SideMark {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            SideMark::None => Ok(()),
            SideMark::Left => write!(f, "L:"),
            SideMark::Right => write!(f, "R:"),
        }
    }
}

#[derive(PartialEq, Eq, Hash, Clone)]
pub enum LayerMark {
    None,
    Forget,
}

impl Display for LayerMark {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            LayerMark::None => Ok(()),
            LayerMark::Forget => write!(f, "F: "),
        }
    }
}

impl Debug for LayerMark {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self)
    }
}

#[derive(PartialEq, Eq, Hash, Clone)]
pub struct Node {
    pub base: DerivedTypeVariable,
    pub suffix_variance: Variance,
    pub sidemark: SideMark,
    pub layer: LayerMark,
}

impl Display for Node {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "{}{}{}.{}",
            self.layer, self.sidemark, self.base, self.suffix_variance
        )
    }
}

impl Debug for Node {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self)
    }
}

impl Node {
    pub fn new(base: DerivedTypeVariable, suffix_variance: Variance) -> Self {
        Node {
            base,
            suffix_variance,
            sidemark: SideMark::None,
            layer: LayerMark::None,
        }
    }
    pub fn forget_once(&self) -> Option<(FieldLabel, Node)> {
        if self.base.fields.is_empty() {
            return None;
        }
        let mut base = self.base.clone();
        let last = base.fields.pop().unwrap();
        let variance = self.suffix_variance.combine(&last.variance());
        let node = Node {
            base,
            suffix_variance: variance,
            sidemark: self.sidemark.clone(),
            layer: self.layer.clone(),
        };
        Some((last, node))
    }
    pub fn set_new_layer(&mut self) -> &mut Self {
        assert!(self.layer == LayerMark::None);
        self.layer = LayerMark::Forget;
        self
    }
}
*/

namespace notdec::retypd {

struct One {};
struct ForgetLabel {
  FieldLabel label;
};
struct ForgetBase {
  llvm::StringRef base;
};
struct RecallLabel {
  FieldLabel label;
};
struct RecallBase {
  llvm::StringRef base;
};
using EdgeLabel =
    std::variant<One, ForgetLabel, ForgetBase, RecallLabel, RecallBase>;

std::string toString(EdgeLabel label);
inline bool isBase(EdgeLabel label) {
  return std::holds_alternative<ForgetBase>(label) ||
         std::holds_alternative<RecallBase>(label);
}
inline bool isLabel(EdgeLabel label) {
  return std::holds_alternative<ForgetLabel>(label) ||
         std::holds_alternative<RecallLabel>(label);
}

} // namespace notdec::retypd

#endif
