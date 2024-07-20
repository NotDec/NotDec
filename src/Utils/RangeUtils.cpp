#include "Utils/RangeUtils.h"

namespace notdec {

typedef uint64_t uint8;
typedef int64_t int8;
typedef uint32_t uint4;
typedef int32_t int4;
typedef uint16_t uint2;
typedef int16_t int2;
typedef uint8_t uint1;
typedef int8_t int1;
typedef int8 intb;   /* This is a signed big integer */
typedef uint8 uintb; /* This is an unsigned big integer */

const char CircleRange::arrange[] = "gcgbegdagggggggeggggcgbggggggggcdfgggggggegdggggbgggfggggcgbegda";
uintb uintbmasks[9] = {
    0, 0xff, 0xffff, 0xffffff, 0xffffffff, 0xffffffffffLL, 0xffffffffffffLL, 0xffffffffffffffLL, 0xffffffffffffffffLL};

/// Count the number of more significant zero bits before the most significant
/// one bit in the representation of the given value;
/// \param val is the given value
/// \return the number of zero bits
int4 count_leading_zeros(uintb val)

{
  if (val == 0)
    return 8 * sizeof(uintb);
  uintb mask = ~((uintb)0);
  int4 maskSize = 4 * sizeof(uintb);
  mask &= (mask << maskSize);
  int4 bit = 0;

  do {
    if ((mask & val) == 0) {
      bit += maskSize;
      maskSize >>= 1;
      mask |= (mask >> maskSize);
    } else {
      maskSize >>= 1;
      mask &= (mask << maskSize);
    }
  } while (maskSize != 0);
  return bit;
}

/// The least significant bit is index 0.
/// \param val is the given value
/// \return the index of the least significant set bit, or -1 if none are set
int4 leastsigbit_set(uintb val)

{
  if (val == 0)
    return -1;
  int4 res = 0;
  int4 sz = 4 * sizeof(uintb);
  uintb mask = ~((uintb)0);
  do {
    mask >>= sz;
    if ((mask & val) == 0) {
      res += sz;
      val >>= sz;
    }
    sz >>= 1;
  } while (sz != 0);
  return res;
}

/// The least significant bit is index 0.
/// \param val is the given value
/// \return the index of the most significant set bit, or -1 if none are set
int4 mostsigbit_set(uintb val)

{
  if (val == 0)
    return -1;
  int4 res = 8 * sizeof(uintb) - 1;
  int4 sz = 4 * sizeof(uintb);
  uintb mask = ~((uintb)0);
  do {
    mask <<= sz;
    if ((mask & val) == 0) {
      res -= sz;
      val <<= sz;
    }
    sz >>= 1;
  } while (sz != 0);
  return res;
}

/// Treat \b val as a constant of size \b sz.
/// Scanning across the bits of \b val return the number of transitions (from 0->1 or 1->0)
/// If there are 2 or less transitions, this is an indication of a bit flag or a mask
/// \param val is the given value
/// \param sz is the size to treat the value as
/// \return the number of transitions
int4 bit_transitions(uintb val, int4 sz)

{
  int4 res = 0;
  int4 last = val & 1;
  int4 cur;
  for (int4 i = 1; i < 8 * sz; ++i) {
    val >>= 1;
    cur = val & 1;
    if (cur != last) {
      res += 1;
      last = cur;
    }
    if (val == 0)
      break;
  }
  return res;
}

/// All the instantiations where left == right represent the same set. We
/// normalize the representation so we can compare sets easily.
void CircleRange::normalize(void)

{
  if (left == right) {
    if (step != 1)
      left = left % step;
    else
      left = 0;
    right = left;
  }
}

/// This method \b only works if \b step is 1
void CircleRange::complement(void)

{
  if (isempty) {
    left = 0;
    right = 0;
    isempty = false;
    return;
  }
  if (left == right) {
    isempty = true;
    return;
  }
  uintb tmp = left;
  left = right;
  right = tmp;
}

/// If the original range contained
///   - 0 and 1   => the new range is [0,2)
///   - 0 only    => the new range is [0,1)
///   - 1 only    => the new range is [1,2)
///   - neither 0 or 1  =>  the new range is empty
///
/// \return \b true if the range contains both 0 and 1
bool CircleRange::convertToBoolean(void)

{
  if (isempty)
    return false;
  bool contains_zero = contains(0);
  bool contains_one = contains(1);
  mask = 0xff;
  step = 1;
  if (contains_zero && contains_one) {
    left = 0;
    right = 2;
    isempty = false;
    return true;
  } else if (contains_zero) {
    left = 0;
    right = 1;
    isempty = false;
  } else if (contains_one) {
    left = 1;
    right = 2;
    isempty = false;
  } else
    isempty = true;
  return false;
}

/// \brief  Recalculate range based on new stride
///
/// Restrict a left/right specified range to a new stride, given the step and
/// remainder it needs to match. This assumes the specified range is not empty.
/// \param mask is the domain mask
/// \param step is the new stride
/// \param oldStep is the original step (always smaller)
/// \param rem is the given remainder to match
/// \param myleft is a reference to the left boundary of the specified range
/// \param myright is a reference to the right boundary of the specified range
/// \return \b true if result is empty
bool CircleRange::newStride(uintb mask, int4 step, int4 oldStep, uint4 rem, uintb &myleft, uintb &myright)

{
  if (oldStep != 1) {
    uint4 oldRem = (uint4)(myleft % oldStep);
    if (oldRem != (rem % oldStep))
      return true; // Step is completely off
  }
  bool origOrder = (myleft < myright);
  uint4 leftRem = (uint4)(myleft % step);
  uint4 rightRem = (uint4)(myright % step);
  if (leftRem > rem)
    myleft += rem + step - leftRem;
  else
    myleft += rem - leftRem;

  if (rightRem > rem)
    myright += rem + step - rightRem;
  else
    myright += rem - rightRem;
  myleft &= mask;
  myright &= mask;

  bool newOrder = (myleft < myright);
  if (origOrder != newOrder)
    return true;

  return false; // not empty
}

/// \brief Make \b this range fit in a new domain
///
/// Truncate any part of the range outside of the new domain.
/// If the original range is completely outside of the new domain,
/// return \b true (empty). Step information is preserved.
/// \param newMask is the mask for the new domain
/// \param newStep is the step associated with the range
/// \param myleft is a reference to the left edge of the range to fit
/// \param myright is a reference to the right edge of the range to fit
/// \return \b true if the truncated domain is empty
bool CircleRange::newDomain(uintb newMask, int4 newStep, uintb &myleft, uintb &myright)

{
  uintb rem;
  if (newStep != 1)
    rem = myleft % newStep;
  else
    rem = 0;
  if (myleft > newMask) {
    if (myright > newMask) { // Both bounds out of range of newMask
      if (myleft < myright)
        return true; // Old range is completely out of bounds of new mask
      myleft = rem;
      myright = rem; // Old range contained everything in newMask
      return false;
    }
    myleft = rem; // Take everything up to left edge of new range
  }
  if (myright > newMask) {
    myright = rem; // Take everything up to right edge of new range
  }
  if (myleft == myright) {
    myleft = rem; // Normalize the everything
    myright = rem;
  }
  return false; // not empty
}

/// Give specific left/right boundaries and step information.
/// The first element in the set is given left boundary. The sequence
/// then proceeds by the given \e step up to (but not including) the given
/// right boundary.  Care should be taken to make sure the remainders of the
/// left and right boundaries modulo the step are equal.
/// \param lft is the left boundary of the range
/// \param rgt is the right boundary of the range
/// \param size is the domain size in bytes (1,2,4,8,..)
/// \param stp is the desired step (1,2,4,8,..)
CircleRange::CircleRange(uintb lft, uintb rgt, int4 size, int4 stp)

{
  mask = calc_mask(size);
  step = stp;
  left = lft;
  right = rgt;
  isempty = false;
}

/// The range contains only a single integer, 0 or 1, depending on the boolean parameter.
/// \param val is the boolean parameter
CircleRange::CircleRange(bool val)

{
  mask = 0xff;
  step = 1;
  left = val ? 1 : 0;
  right = val + 1;
  isempty = false;
}

/// A size specifies the number of bytes (*8 to get number of bits) in the mask.
/// The stride is assumed to be 1.
/// \param val is is the single value
/// \param size is the size of the mask in bytes
CircleRange::CircleRange(uintb val, int4 size)

{
  mask = calc_mask(size);
  step = 1;
  left = val;
  right = (left + 1) & mask;
  isempty = false;
}

/// \param lft is the left boundary of the range
/// \param rgt is the right boundary of the range
/// \param size is the size of the range domain in bytes
/// \param stp is the step/stride of the range
void CircleRange::setRange(uintb lft, uintb rgt, int4 size, int4 stp)

{
  mask = calc_mask(size);
  left = lft;
  right = rgt;
  step = stp;
  isempty = false;
}

/// A size specifies the number of bytes (*8 to get number of bits) in the mask.
/// The stride is assumed to be 1.
/// \param val is is the single value
/// \param size is the size of the mask in bytes
void CircleRange::setRange(uintb val, int4 size)

{
  mask = calc_mask(size);
  step = 1;
  left = val;
  right = (left + 1) & mask;
  isempty = false;
}

/// Make a range of values that holds everything.
/// \param size is the size (in bytes) of the range
void CircleRange::setFull(int4 size)

{
  mask = calc_mask(size);
  step = 1;
  left = 0;
  right = 0;
  isempty = false;
}

/// \return the number of integers contained in this range
uintb CircleRange::getSize(void) const

{
  if (isempty)
    return 0;
  uintb val;
  if (left < right)
    val = (right - left) / step;
  else {
    // Mask范围内，减去这个反的范围，然后除以step，最后加1
    val = (mask - (left - right) + step) / step;
    if (val == 0) { // This is an overflow, when all uintb values are in the range
      val = mask;   // We lie by one, which shouldn't matter for our jumptable application
      if (step > 1) {
        val = val / step;
        val += 1;
      }
    }
  }
  return val;
}

/// In this context, the information content of a value is the index (+1) of the
/// most significant non-zero bit (of the absolute value). This routine returns
/// the maximum information across all values in the range.
/// \return the maximum information
int4 CircleRange::getMaxInfo(void) const

{
  uintb halfPoint = mask ^ (mask >> 1);
  if (contains(halfPoint))
    return 8 * sizeof(uintb) - count_leading_zeros(halfPoint);
  int4 sizeLeft, sizeRight;
  if ((halfPoint & left) == 0)
    sizeLeft = count_leading_zeros(left);
  else
    sizeLeft = count_leading_zeros(~left & mask);
  if ((halfPoint & right) == 0)
    sizeRight = count_leading_zeros(right);
  else
    sizeRight = count_leading_zeros(~right & mask);
  int4 size1 = 8 * sizeof(uintb) - (sizeRight < sizeLeft ? sizeRight : sizeLeft);
  return size1;
}

/// \param op2 is the specific range to test for containment.
/// \return \b true if \b this contains the interval \b op2
bool CircleRange::contains(const CircleRange &op2) const

{
  if (isempty)
    return op2.isempty;
  if (op2.isempty)
    return true;
  if (step > op2.step) {
    // This must have a smaller or equal step to op2 or containment is impossible
    // except in the corner case where op2 consists of a single element (its step is meaningless)
    if (!op2.isSingle())
      return false;
  }
  if (left == right)
    return true;
  if (op2.left == op2.right)
    return false;
  if (left % step != op2.left % step)
    return false; // Wrong phase
  if (left == op2.left && right == op2.right)
    return true;

  char overlapCode = encodeRangeOverlaps(left, right, op2.left, op2.right);

  if (overlapCode == 'c')
    return true;
  if (overlapCode == 'b' && (right == op2.right))
    return true;
  return false;

  // Missing one case where op2.step > this->step, and the boundaries don't show containment,
  // but there is containment because the lower step size UP TO right still contains the edge points
}

/// Check if a specific integer is a member of \b this range.
/// \param val is the specific integer
/// \return \b true if it is contained in \b this
bool CircleRange::contains(uintb val) const

{
  if (isempty)
    return false;
  if (step != 1) {
    if ((left % step) != (val % step))
      return false; // Phase is wrong
  }
  if (left < right) {
    if (val < left)
      return false;
    if (right <= val)
      return false;
  } else if (right < left) {
    if (val < right)
      return true;
    if (val >= left)
      return true;
    return false;
  }
  return true;
}

/// Set \b this to the union of \b this and \b op2 as a single interval.
/// Return 0 if the result is valid.
/// Return 2 if the union is two pieces.
/// If result is not zero, \b this is not modified.
/// \param op2 is the range to union with
/// \return the result code
int4 CircleRange::circleUnion(const CircleRange &op2)

{
  if (op2.isempty)
    return 0;
  if (isempty) {
    *this = op2;
    return 0;
  }
  if (mask != op2.mask)
    return 2; // Cannot do proper union with different domains
  uintb aRight = right;
  uintb bRight = op2.right;
  int4 newStep = step;
  if (step < op2.step) {
    if (isSingle()) {
      newStep = op2.step;
      aRight = (left + newStep) & mask;
    } else
      return 2;
  } else if (op2.step < step) {
    if (op2.isSingle()) {
      newStep = step;
      bRight = (op2.left + newStep) & mask;
    } else
      return 2;
  }
  uintb rem;
  if (newStep != 1) {
    rem = left % newStep;
    if (rem != (op2.left % newStep))
      return 2;
  } else
    rem = 0;
  if ((left == aRight) || (op2.left == bRight)) {
    left = rem;
    right = rem;
    step = newStep;
    return 0;
  }

  char overlapCode = encodeRangeOverlaps(left, aRight, op2.left, bRight);
  switch (overlapCode) {
  case 'a': // order (l r op2.l op2.r)
  case 'f': // order (op2.l op2.r l r)
    if (aRight == op2.left) {
      right = bRight;
      step = newStep;
      return 0;
    }
    if (left == bRight) {
      left = op2.left;
      right = aRight;
      step = newStep;
      return 0;
    }
    return 2; // 2 pieces;
  case 'b':   // order (l op2.l r op2.r)
    right = bRight;
    step = newStep;
    return 0;
  case 'c': // order (l op2.l op2.r r)
    right = aRight;
    step = newStep;
    return 0;
  case 'd': // order (op2.l l r op2.r)
    left = op2.left;
    right = bRight;
    step = newStep;
    return 0;
  case 'e': // order (op2.l l op2.r r)
    left = op2.left;
    right = aRight;
    step = newStep;
    return 0;
  case 'g': // either impossible or covers whole circle
    left = rem;
    right = rem;
    step = newStep;
    return 0; // entire circle is covered
  }
  return -1; // Never reach here
}

/// Turn \b this into a range that contains both the original range and
/// the other given range. The resulting range may contain values that were in neither
/// of the original ranges (not a strict union). But the number of added values will be
/// minimal. This method will create a range with step if the input ranges hold single values
/// and the distance between them is a power of 2 and less or equal than a given bound.
/// \param op2 is the other given range to combine with \b this
/// \param maxStep is the step bound that can be induced for a container with two singles
/// \return \b true if the container is everything (full)
bool CircleRange::minimalContainer(const CircleRange &op2, int4 maxStep)

{
  if (isSingle() && op2.isSingle()) {
    uintb min, max;
    if (getMin() < op2.getMin()) {
      min = getMin();
      max = op2.getMin();
    } else {
      min = op2.getMin();
      max = getMin();
    }
    uintb diff = max - min;
    if (diff > 0 && diff <= maxStep) {
      if (leastsigbit_set(diff) == mostsigbit_set(diff)) {
        step = (int4)diff;
        left = min;
        right = (max + step) & mask;
        return false;
      }
    }
  }

  uintb aRight = right - step + 1; // Treat original ranges as having step=1
  uintb bRight = op2.right - op2.step + 1;
  step = 1;
  mask |= op2.mask;
  uintb vacantSize1, vacantSize2;

  char overlapCode = encodeRangeOverlaps(left, aRight, op2.left, bRight);
  switch (overlapCode) {
  case 'a': // order (l r op2.l op2.r)
    vacantSize1 = left + (mask - bRight) + 1;
    vacantSize2 = op2.left - aRight;
    if (vacantSize1 < vacantSize2) {
      left = op2.left;
      right = aRight;
    } else {
      right = bRight;
    }
    break;
  case 'f': // order (op2.l op2.r l r)
    vacantSize1 = op2.left + (mask - aRight) + 1;
    vacantSize2 = left - bRight;
    if (vacantSize1 < vacantSize2) {
      right = bRight;
    } else {
      left = op2.left;
      right = aRight;
    }
    break;
  case 'b': // order (l op2.l r op2.r)
    right = bRight;
    break;
  case 'c': // order (l op2.l op2.r r)
    right = aRight;
    break;
  case 'd': // order (op2.l l r op2.r)
    left = op2.left;
    right = bRight;
    break;
  case 'e': // order (op2.l l op2.r r)
    left = op2.left;
    right = aRight;
    break;
  case 'g':   // order (l op2.r op2.l r)
    left = 0; // Entire circle is covered
    right = 0;
    break;
  }
  normalize();
  return (left == right);
}

/// Convert range to its complement.  The step is automatically converted to 1 first.
/// \return the original step size
int4 CircleRange::invert(void)

{
  int4 res = step;
  step = 1;
  complement();
  return res;
}

/// Set \b this to the intersection of \b this and \b op2 as a
/// single interval if possible.
/// Return 0 if the result is valid
/// Return 2 if the intersection is two pieces
/// If result is not zero, \b this is not modified
/// \param op2 is the second range
/// \return the intersection code
int4 CircleRange::intersect(const CircleRange &op2)

{
  int4 retval, newStep;
  uintb newMask, myleft, myright, op2left, op2right;

  if (isempty)
    return 0; // Intersection with empty is empty
  if (op2.isempty) {
    isempty = true;
    return 0;
  }
  myleft = left;
  myright = right;
  op2left = op2.left;
  op2right = op2.right;
  if (step < op2.step) {
    newStep = op2.step;
    uint4 rem = (uint4)(op2left % newStep);
    if (newStride(mask, newStep, step, rem, myleft, myright)) { // Increase the smaller stride
      isempty = true;
      return 0;
    }
  } else if (op2.step < step) {
    newStep = step;
    uint4 rem = (uint4)(myleft % newStep);
    if (newStride(op2.mask, newStep, op2.step, rem, op2left, op2right)) {
      isempty = true;
      return 0;
    }
  } else
    newStep = step;
  newMask = mask & op2.mask;
  if (mask != newMask) {
    if (newDomain(newMask, newStep, myleft, myright)) {
      isempty = true;
      return 0;
    }
  } else if (op2.mask != newMask) {
    if (newDomain(newMask, newStep, op2left, op2right)) {
      isempty = true;
      return 0;
    }
  }
  if (myleft == myright) { // Intersect with this everything
    left = op2left;
    right = op2right;
    retval = 0;
  } else if (op2left == op2right) { // Intersect with op2 everything
    left = myleft;
    right = myright;
    retval = 0;
  } else {
    char overlapCode = encodeRangeOverlaps(myleft, myright, op2left, op2right);
    switch (overlapCode) {
    case 'a': // order (l r op2.l op2.r)
    case 'f': // order (op2.l op2.r l r)
      isempty = true;
      retval = 0; // empty set
      break;
    case 'b': // order (l op2.l r op2.r)
      left = op2left;
      right = myright;
      if (left == right)
        isempty = true;
      retval = 0;
      break;
    case 'c': // order (l op2.l op2.r r)
      left = op2left;
      right = op2right;
      retval = 0;
      break;
    case 'd': // order (op2.l l r op2.r)
      left = myleft;
      right = myright;
      retval = 0;
      break;
    case 'e': // order (op2.l l op2.r r)
      left = myleft;
      right = op2right;
      if (left == right)
        isempty = true;
      retval = 0;
      break;
    case 'g': // order (l op2.r op2.l r)
      if (myleft == op2right) {
        left = op2left;
        right = myright;
        if (left == right)
          isempty = true;
        retval = 0;
      } else if (op2left == myright) {
        left = myleft;
        right = op2right;
        if (left == right)
          isempty = true;
        retval = 0;
      } else
        retval = 2; // 2 pieces
      break;
    default:
      retval = 2; // Will never reach here
      break;
    }
  }
  if (retval != 0)
    return retval;
  mask = newMask;
  step = newStep;
  return 0;
}

/// Try to create a range given a value that is not necessarily a valid mask.
/// If the mask is valid, range is set to all possible values that whose non-zero
/// bits are contained in the mask. If the mask is invalid, \b this range is  not modified.
/// \param nzmask is the putative mask
/// \param size is a maximum size (in bytes) for the mask
/// \return \b true if the mask is valid
bool CircleRange::setNZMask(uintb nzmask, int4 size)

{
  int4 trans = bit_transitions(nzmask, size);
  if (trans > 2)
    return false; // Too many transitions to form a valid range
  bool hasstep = ((nzmask & 1) == 0);
  if ((!hasstep) && (trans == 2))
    return false; // Two sections of non-zero bits
  isempty = false;
  if (trans == 0) {
    mask = calc_mask(size);
    if (hasstep) { // All zeros
      step = 1;
      left = 0;
      right = 1; // Range containing only zero
    } else {     // All ones
      step = 1;
      left = 0;
      right = 0; // Everything
    }
    return true;
  }
  int4 shift = leastsigbit_set(nzmask);
  step = 1;
  step <<= shift;
  mask = calc_mask(size);
  left = 0;
  right = (nzmask + step) & mask;
  return true;
}

/// This method changes the step for \b this range, i.e. elements are removed.
/// The boundaries of the range do not change except for the remainder modulo the new step.
/// \param newStep is the new step amount
/// \param rem is the desired phase (remainder of the values modulo the step)
void CircleRange::setStride(int4 newStep, uintb rem)

{
  bool iseverything = (!isempty) && (left == right);
  if (newStep == step)
    return;
  uintb aRight = right - step;
  step = newStep;
  if (step == 1)
    return; // No remainder to fill in
  uintb curRem = left % step;
  left = (left - curRem) + rem;
  curRem = aRight % step;
  aRight = (aRight - curRem) + rem;
  right = aRight + step;
  if ((!iseverything) && (left == right))
    isempty = true;
}

} // namespace notdec
