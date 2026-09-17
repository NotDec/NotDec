pragma solidity 0.8.26;

// Minimal one-argument public setter.  The ABI decoder first checks
// msg.data.length >= 36 and only then loads the word at calldata offset 4.
contract CalldataMinSizeEntry {
    uint256 public slot_0;

    function calldata_min_size(uint256 arg0) public {
        require(msg.data.length >= 36);
        slot_0 = arg0;
    }
}
