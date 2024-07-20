#ifndef _NOTDEC_UTILS_RANGE_H_
#define _NOTDEC_UTILS_RANGE_H_

#include <cstdint>
namespace notdec {

/// \brief A class for manipulating integer value ranges.
///
/// The idea is to have a representation of common sets of
/// values that a varnode might take on in analysis so that
/// the representation can be manipulated symbolically to
/// some extent.  The representation is a circular range
/// (determined by a half-open interval [left,right)), over
/// the integers mod 2^n,  where mask = 2^n-1.
/// The range can support a step, if some of the
/// least significant bits of the mask are set to zero.
///
/// The class then can
///   - Generate ranges based on a pcode condition:
///      -    x < 2      =>   left=0  right=2  mask=sizeof(x)
///      -    5 >= x     =>   left=5  right=0  mask=sizeof(x)
///
///   - Intersect and union ranges, if the result is another range
///   - Pull-back a range through a transformation operation
///   - Iterate
///
///   \code
///     val = range.getMin();
///     do {
///     } while(range.getNext(val));
///   \endcode
class CircleRange {
  uint64_t left;               ///< Left boundary of the open range [left,right)
  uint64_t right;              ///< Right boundary of the open range [left,right)
  uint64_t mask;               ///< Bit mask defining the size (modulus) and stop of the range
  bool isempty;                ///< \b true if set is empty
  int32_t step;                ///< Explicit step size
  static const char arrange[]; ///< Map from raw overlaps to normalized overlap code
  void normalize(void);        ///< Normalize the representation of full sets
  void complement(void);       ///< Set \b this to the complement of itself
  bool convertToBoolean(void); ///< Convert \b this to boolean.
  static bool newStride(uint64_t mask, int32_t step, int32_t oldStep, uint32_t rem, uint64_t &myleft, uint64_t &myright);
  static bool newDomain(uint64_t newMask, int32_t newStep, uint64_t &myleft, uint64_t &myright);
  static char encodeRangeOverlaps(uint64_t op1left, uint64_t op1right, uint64_t op2left,
                                  uint64_t op2right); ///< Calculate overlap code
public:
  CircleRange(void) { isempty = true; } ///< Construct an empty range
  CircleRange(uint64_t lft, uint64_t rgt, int32_t size,
              int32_t stp);                ///< Construct given specific boundaries.
  CircleRange(bool val);                   ///< Construct a boolean range
  CircleRange(uint64_t val, int32_t size); ///< Construct range with single value
  void setRange(uint64_t lft, uint64_t rgt, int32_t size,
                int32_t step);                 ///< Set directly to a specific range
  void setRange(uint64_t val, int32_t size);   ///< Set range with a single value
  void setFull(int32_t size);                  ///< Set a completely full range
  bool isEmpty(void) const { return isempty; } ///< Return \b true if \b this range is empty
  bool isFull(void) const {
    return ((!isempty) && (step == 1) && (left == right));
  } ///< Return \b true if \b this contains all possible values
  bool isSingle(void) const {
    return (!isempty) && (right == ((left + step) & mask));
  }                                                             ///< Return \b true if \b this contains single value
  uint64_t getMin(void) const { return left; }                  ///< Get the left boundary of the range
  uint64_t getMax(void) const { return (right - step) & mask; } ///< Get the right-most integer contained in the range
  uint64_t getEnd(void) const { return right; }                 ///< Get the right boundary of the range
  uint64_t getMask(void) const { return mask; }                 ///< Get the mask
  uint64_t getSize(void) const;                                 ///< Get the size of this range
  int32_t getStep(void) const { return step; }                  ///< Get the step for \b this range
  int32_t getMaxInfo(void) const;                               ///< Get maximum information content of range
  bool operator==(const CircleRange &op2) const;                ///< Equals operator
  bool getNext(uint64_t &val) const {
    val = (val + step) & mask;
    return (val != right);
  }                                              ///< Advance an integer within the range
  bool contains(const CircleRange &op2) const;   ///< Check containment of another range in \b this.
  bool contains(uint64_t val) const;             ///< Check containment of a specific integer.
  int32_t intersect(const CircleRange &op2);     ///< Intersect \b this with another range
  bool setNZMask(uint64_t nzmask, int32_t size); ///< Set the range based on a putative mask.
  int32_t circleUnion(const CircleRange &op2);   ///< Union two ranges.
  bool minimalContainer(const CircleRange &op2,
                        int32_t maxStep);        ///< Construct minimal range that contains both \b this and another range
  int32_t invert(void);                          ///< Convert to complementary range
  void setStride(int32_t newStep, uint64_t rem); ///< Set a new step on \b this range.
};

extern uint64_t uintbmasks[];
/// \param size is the desired size in bytes
/// \return a value appropriate for masking off the first \e size bytes
inline int64_t calc_mask(int32_t size) { return uintbmasks[((uint32_t)size) < 8 ? size : 8]; }

} // namespace notdec

#endif
