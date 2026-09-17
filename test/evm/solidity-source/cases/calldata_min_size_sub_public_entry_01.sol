pragma solidity 0.8.26;

// Three static arguments.  The guard checks (msg.data.length - 4) >= 96 and
// the body reads the ABI word at calldata offset 68, i.e. argument index 2.
contract CalldataMinSizeSubEntry {
    uint256 public slot_0;

    function calldata_min_size_sub(address arg0, address arg1, uint256 arg2) public {
        require(msg.data.length - 4 >= 96);
        slot_0 = arg2;
    }
}
