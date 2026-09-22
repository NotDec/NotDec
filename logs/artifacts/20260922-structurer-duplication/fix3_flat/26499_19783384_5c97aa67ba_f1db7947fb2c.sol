contract Decompiled {
    event Event_0xA5E1F8B4009110F5525798D04AE2125421A12D0590AA52C13682FF1BD3C492CA(address indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_0;

    function public_0x20709efc() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0[0 /* TODO: unresolved value: evm.calldataload.i.i.load */] & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0x6d61fe70() public {
        // block_0:
        if (msg.data.length > 35) {
          // goto block_1
        }
        // goto block_9
        // block_1:
        if (false /* TODO: evm.gt.i */) {
          // goto block_2
        }
        // goto block_8
        // block_2:
        if (false /* TODO: evm.slt.i.i */) {
          // goto block_3
        }
        // goto block_7
        // block_3:
        if (false /* TODO: evm.gt.i.i */) {
          // goto block_4
        }
        // goto block_6
        // block_4:
        if (false /* TODO: evm.gt7.i.i.not */) {
          // goto block_5
        }
        // goto block_10
        // block_5:
        revert(); // empty
        // block_6:
        revert(); // empty
        // block_7:
        revert(); // empty
        // block_8:
        revert(); // empty
        // block_9:
        revert(); // empty
        // block_10:
        if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0[uint256(uint160(msg.sender))] == 0)) {
          // goto block_14
        }
        // goto block_11
        // block_11:
        if (!false /* TODO: evm.lt.i */) {
          // block_12:
        }
        // block_13:
        emit Event_0xA5E1F8B4009110F5525798D04AE2125421A12D0590AA52C13682FF1BD3C492CA(msg.sender, (0 /* TODO: unresolved value: private.call56.in.reload */ & uint256((type(uint256).max - 79228162514264337593543950335))) >> uint256(96)); // TODO: recover event signature
        // block_14:
        revert(); // custom_error_candidate, selector=0x93360fbf, args=1
    }

    function public_0x8a91b0e3() public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.gt.i */);
        // block_2:
        require(false /* TODO: evm.slt.i.i */);
        // block_3:
        require(false /* TODO: evm.gt.i.i */);
        // block_4:
        require(!false /* TODO: evm.gt7.i.i.not */);
        // block_10:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0[uint256(uint160(msg.sender))] == 0) {
          // block_12:
          revert(); // custom_error_candidate, selector=0xf91bd6f1, args=1
        }
        else {
          // block_11:
        }
    }

    function public_0x97003203() public returns (bool ret0) {
        // block_0:
        if (msg.data.length > 67) {
          // goto block_1
        }
        // goto block_44
        // block_1:
        if (false /* TODO: evm.gt */) {
          // goto block_2
        }
        // goto block_43
        // block_2:
        if (false /* TODO: evm.slt7 */) {
          // goto block_3
        }
        // goto block_42
        // block_3:
        if (false /* TODO: evm.slt.i.i */) {
          // goto block_4
        }
        // goto block_8
        // block_4:
        if (false /* TODO: evm.gt.i.i */) {
          // goto block_5
        }
        // goto block_7
        // block_5:
        if (false /* TODO: evm.sgt.i.i.not */) {
          // goto block_6
        }
        // goto block_9
        // block_6:
        revert(); // empty
        // block_7:
        revert(); // empty
        // block_8:
        revert(); // empty
        // block_9:
        if (false /* TODO: evm.branch.cond.i.not */) {
          // goto block_39
        }
        else {
          // goto block_10
        }
        // block_10:
        if (!false /* TODO: evm.lt.i.i */) {
          // block_11:
        }
        // block_12:
        if (!false /* TODO: evm.branch.cond25.i.not */) {
          // goto block_13
        }
        // goto block_25
        // block_13:
        require(false /* TODO: evm.slt.i7.i */);
        // block_14:
        require(false /* TODO: evm.gt.i12.i */);
        // block_15:
        require(false /* TODO: evm.slt.i.i.i */);
        // block_16:
        // block_19:
        revert(); // empty
        if (false /* TODO: evm.gt15.i.i.i.not */) {
          // goto block_19
        }
        while (false /* TODO: evm.lt21.i.i.i.not */) {
          // block_18:
        }
        // block_23:
        if (!false /* TODO: evm.branch.cond32.i.not */) {
          // goto block_36
        }
        // block_24:
        revert(); // error_string, selector=0x8c379a0
        // block_25:
        require(false /* TODO: evm.slt.i16.i */);
        // block_26:
        require(false /* TODO: evm.gt.i23.i */);
        // block_27:
        require(false /* TODO: evm.slt.i.i26.i */);
        // block_28:
        // block_31:
        revert(); // empty
        if (false /* TODO: evm.gt15.i.i57.i.not */) {
          // goto block_31
        }
        while (false /* TODO: evm.lt21.i.i67.i.not */) {
          // block_30:
        }
        // goto block_36
        // block_35:
        if (false /* TODO: evm.branch.cond47.i.not */) {
          // block_38:
          revert(); // error_string, selector=0x8c379a0
        }
        // block_36:
        if (!false /* TODO: evm.branch.cond56.i.not */) {
          // block_37:
        }
        // goto block_41
        // block_39:
        if (!false /* TODO: evm.branch.cond88.i.not */) {
          // block_40:
        }
        // block_41:
        return 0 /* TODO: unresolved value: private.call1.shrunk.reload */ != 0;
        // block_42:
        revert(); // empty
        // block_43:
        revert(); // empty
        // block_44:
        revert(); // empty
    }

    function public_0xaacbd72a() public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(false /* TODO: evm.gt.i */);
        // block_2:
        require(false /* TODO: evm.slt.i.i */);
        // block_3:
        require(false /* TODO: evm.gt.i.i */);
        // block_4:
        require(!false /* TODO: evm.gt7.i.i.not */);
        // block_8:
        require(false /* TODO: evm.eq.i */);
        // block_9:
        require(false /* TODO: evm.gt17.i */);
        // block_10:
        require(false /* TODO: evm.slt.i2.i */);
        // block_11:
        require(false /* TODO: evm.gt.i7.i */);
        // block_12:
        require(!false /* TODO: evm.gt7.i16.i.not */);
        // block_20:
    }

    function public_0xd68f6025() public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_3:
        require(false /* TODO: evm.gt.i */);
        // block_4:
        require(false /* TODO: evm.slt.i.i */);
        // block_5:
        require(false /* TODO: evm.gt.i.i */);
        // block_6:
        // block_7:
        revert(); // empty
        if (false /* TODO: evm.gt7.i.i.not */) {
          // goto block_7
        }
        while (true) {
          // block_12:
          // block_13:
          if (!false /* TODO: evm.lt.not */) {
            break;
          }
          continue;
        }
        // block_14:
        // TODO: recover remaining body
    }

    function public_0xecd05961() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        return 0 /* TODO: unresolved value: private__0x6b9_0x6b9 */;
    }

    function public_0xf551e2ee() public returns (uint256 ret0) {
        // block_0:
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_11
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_3:
        if (false /* TODO: evm.gt.i */) {
          // goto block_4
        }
        // goto block_10
        // block_4:
        if (false /* TODO: evm.slt.i.i */) {
          // goto block_5
        }
        // goto block_9
        // block_5:
        if (false /* TODO: evm.gt.i.i */) {
          // goto block_6
        }
        // goto block_8
        // block_6:
        if (false /* TODO: evm.gt7.i.i.not */) {
          // goto block_7
        }
        // goto block_12
        // block_7:
        revert(); // empty
        // block_8:
        revert(); // empty
        // block_9:
        revert(); // empty
        // block_10:
        revert(); // empty
        // block_11:
        revert(); // empty
        // block_12:
        if (false /* TODO: evm.branch.cond.i.not */) {
          // goto block_32
        }
        // block_13:
        if (!false /* TODO: evm.lt.i.i */) {
          // block_14:
        }
        // block_15:
        if (!false /* TODO: evm.slt.i.i9 */) {
          // goto block_24
        }
        // block_16:
        require(false /* TODO: evm.gt.i.i10 */);
        // block_17:
        require(false /* TODO: evm.slt.i.i.i */);
        // block_18:
        // block_21:
        revert(); // empty
        if (false /* TODO: evm.gt15.i.i.i.not */) {
          // goto block_21
        }
        while (false /* TODO: evm.lt21.i.i.i.not */) {
          // block_20:
        }
        // goto block_26
        // block_24:
        revert(); // empty
        // block_25:
        if (false /* TODO: evm.branch.cond17.i.not */) {
          // block_31:
          revert(); // error_string, selector=0x8c379a0
        }
        // block_26:
        if (false /* TODO: evm.branch.cond26.i.not */) {
          // block_30:
        }
        else {
          // block_27:
          if (false /* TODO: evm.eq.i */) {
            // block_28:
          }
          else {
            // block_29:
          }
        }
        // goto block_37
        // block_32:
        if (false /* TODO: evm.branch.cond62.i.not */) {
          // block_36:
        }
        else {
          // block_33:
          if (false /* TODO: evm.eq79.i */) {
            // block_34:
          }
          else {
            // block_35:
          }
        }
        // block_37:
        return 0 /* TODO: unresolved value: private.call414.reload */ & uint256(0) - (uint256(1) << uint256(224));
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function isInitialized(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return private__0x603_0x603(uint256(uint160(arg0)), 82979);
    }

    function private__0x603_0x603(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] != 0 ? 1 : 0;
    }

    function private__0x6b9_0x6b9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 == 1 || arg0 == 4 ? 1 : 0;
    }

    function private__0xe63_0xe63(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }
}
