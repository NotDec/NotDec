pragma solidity 0.8.26;

// Two static arguments decoded by an outlined private helper.  The helper
// returns the decoded words in reverse stack order (tuple element 0 is the
// second calldata word), so the result annotation has to follow the
// insertvalue chain instead of assuming tuple order matches argument order.
contract AbiDecodeResultEntry {
    uint256 public slot_0;
    uint256 public slot_1;

    function two_args(uint256 arg0, uint256 arg1) public {
        slot_0 = arg1;
        slot_1 = arg0;
    }
}
