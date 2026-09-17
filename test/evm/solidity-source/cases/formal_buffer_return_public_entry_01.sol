pragma solidity 0.8.26;

// The outlined wrapper receives the ABI return buffer as a formal, stores one
// word through it and returns it.  The matcher has to accept an inttoptr of the
// formal itself, not only a calloc-backed or constant buffer.
contract FormalBufferReturnEntry {
    function word_return_formal_buffer(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        return arg0;
    }
}
