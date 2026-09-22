contract Decompiled {
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_1;
    uint256 public slot_2;
    mapping(uint256 => uint256) public slot_3;

    function public_0x37a25dc2() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
    }

    function public_0x7fa98ede() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
    }

    function public_0xba49f371() public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(false /* TODO: evm.gt.i */);
        // block_2:
        require(false /* TODO: evm.slt7.i */);
        // block_3:
        // block_7:
        revert(); // empty
        if (false /* TODO: evm.gt27.i.not */) {
          // goto block_7
        }
        while (true) {
          // block_4:
          // block_5:
          if (!false /* TODO: evm.lt33.i.not */) {
            break;
          }
          continue;
        }
        // block_11:
        // TODO: recover remaining body
    }

    function public_0xeeeeeeee() public {
        // block_0:
        require(slot_3[uint256(uint160(tx.origin))] != 0);
        // block_1:
    }

    function coin() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function dust() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function renounceOwnership() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0); // TODO: recover event signature
          slot_0 = slot_0 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function save(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0)) {
          // block_14:
          revert(); // error_string, selector=0x8c379a0
        }
        // block_2:
        // block_13:
        revert(); // returndata_forward
        if (false /* TODO: evm.iszero.i.i.not */) {
          // goto block_13
        }
        while (true) {
          // block_3:
          // block_4:
          if (!false /* TODO: evm.lt43.i.i.not */) {
            break;
          }
          continue;
        }
        // block_5:
        require(!false /* TODO: evm.iszero55.i.i.not */);
        // block_6:
        if (false /* TODO: evm.iszero61.i.i.not */) {
          // block_10:
          revert(); // returndata_forward
        }
        else {
          // block_7:
          if (address(this).balance == 0) {
            // block_20:
          }
          // block_8:
          if (!false /* TODO: evm.iszero77.i.i.not */) {
            // goto block_20
          }
          // goto block_9
          // block_9:
          revert(); // returndata_forward
        }
    }

    function swap() public {
        // block_0:
        if (false /* TODO: evm.iszero.i.not */) {
          // block_13:
          revert(); // returndata_forward
        }
        if (!false /* TODO: evm.iszero.i.not */) {
          // goto block_1
        }
        // goto block_13
        // block_1:
        if (false /* TODO: evm.iszero.i.i.not */) {
          // goto block_12
        }
        while (true) {
          // block_2:
          // block_3:
          if (!false /* TODO: evm.lt43.i.i.not */) {
            break;
          }
          continue;
        }
        // block_4:
        require(!false /* TODO: evm.iszero55.i.i.not */);
        // block_5:
        if (false /* TODO: evm.iszero61.i.i.not */) {
          // block_9:
          revert(); // returndata_forward
        }
        else {
          // block_6:
          if (address(this).balance == 0) {
            // block_18:
          }
          // block_7:
          if (!false /* TODO: evm.iszero77.i.i.not */) {
            // goto block_18
          }
          // goto block_8
          // block_8:
          revert(); // returndata_forward
        }
        // block_12:
        revert(); // returndata_forward
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x83a_0x83a(uint256(uint160(arg0)), 39405);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_4:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_0 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_0 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x83a_0x83a(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x894_0x894(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 2);
        // block_1:
    }

    function private__0x97e_0x97e(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }
}
