pragma solidity 0.8.26;

// Two static ABI return words written into a calloc-backed buffer.  The reader
// already declares returns (uint256 ret0, uint256 ret1); the body now has to
// collect the buffer stores back into return (a, b).
contract MultiWordReturnEntry {
    function two_returns(uint256 arg0, uint256 arg1) public returns (uint256 ret0, uint256 ret1) {
        return (arg0, arg1);
    }
}
