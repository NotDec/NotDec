pragma solidity 0.8.26;

// Same as revert_error_string_01, except the Error(string) selector word is
// stored as the already-folded ABI constant instead of an evm_shl call.  This
// is the shape InstCombine produces, and it has to classify as error_string
// rather than falling through to encoded_candidate.
contract RevertErrorStringFoldedSelector {
    function run() public {
        require(false, "short");
    }
}
