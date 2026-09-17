pragma solidity 0.8.26;

// Environment builtins stored into state.  Word-valued members print directly;
// the address-valued ones (block.coinbase, tx.origin, address(this)) need the
// uint256(uint160(...)) conversion that recovered state variables expect.
contract EnvBuiltinsEntry {
    uint256 public slot_0;
    uint256 public slot_1;
    uint256 public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;

    function env(uint256 arg0) public {
        slot_0 = block.timestamp;
        slot_1 = block.number;
        slot_2 = tx.gasprice;
        slot_3 = block.chainid;
        slot_4 = address(this).balance;
        slot_5 = uint256(uint160(address(block.coinbase)));
        slot_6 = uint256(uint160(tx.origin));
        slot_7 = uint256(uint160(address(this)));
    }
}
