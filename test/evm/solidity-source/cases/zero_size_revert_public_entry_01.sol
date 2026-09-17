pragma solidity 0.8.26;

// The guard reverts with a non-zero memory offset but a zero size.  EVM REVERT
// returns exactly size bytes, so this is an empty revert and has to be
// classified as such; otherwise the Solidity backend cannot fold the guard to
// require(arg0 != 0).
contract ZeroSizeRevertEntry {
    uint256 public slot_0;

    function guard(uint256 arg0) public {
        require(arg0 != 0);
        slot_0 = arg0;
    }
}
