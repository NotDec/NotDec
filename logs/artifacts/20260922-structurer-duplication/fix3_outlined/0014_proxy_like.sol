contract Decompiled {
    event Event_0x88A5966D370B9919B20F3E2C13FF65706F196A4E32CC2C12BF57088F88525874(address indexed arg0);

    uint256 public slot_0;
    uint256 public slot_1;

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function notdec_solidity_selector_inline_body() public {
        // block_0:
        if (slot_1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == uint256(uint160(msg.sender))) {
          // block_2:
          if (!false /* TODO: evm.eq22 */) {
            // block_3:
          }
        }
        else {
          // block_1:
          emit Event_0x88A5966D370B9919B20F3E2C13FF65706F196A4E32CC2C12BF57088F88525874(msg.sender); // TODO: recover event signature
        }
        // block_4:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function target() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }
}
