pragma solidity 0.8.26;

// Same layout as abi_decode_result_public_entry_01, but the second word is
// decoded by a nested word helper that receives the outer helper's base formal
// instead of a constant.  The result annotation has to compose the callee's
// base-relative offset with the outer call-site base (4) before it can name arg0.
contract AbiDecodeNestedResultEntry {
    uint256 public slot_0;
    uint256 public slot_1;

    function two_args(uint256 arg0, uint256 arg1) public {
        slot_0 = arg1;
        slot_1 = arg0;
    }
}
