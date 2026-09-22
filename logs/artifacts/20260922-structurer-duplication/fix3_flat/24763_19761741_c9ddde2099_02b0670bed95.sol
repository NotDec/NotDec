contract Decompiled {
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(address indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_0;
    uint256 public slot_2;
    uint256 public slot_3;
    uint256 public slot_5;

    function allowance(address arg0, address arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_8:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function approve(address arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        if (uint256(uint160(msg.sender)) == 0) {
          // block_7:
          revert(); // custom_error_candidate, selector=0xe602df05, args=1
        }
        else {
          // block_8:
          if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == 0) {
            // block_9:
            revert(); // custom_error_candidate, selector=0x94280d62, args=1
          }
          else {
            // block_10:
            emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(msg.sender, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))); // TODO: recover event signature
            return 1;
          }
        }
    }

    function balanceOf(address arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function decimals(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return 18;
    }

    function name(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg4);
        // block_4:
        if (!(slot_3 & uint256(1) == 0)) {
          // goto block_6
        }
        // block_5:
        // goto block_9
        // block_6:
        while (false /* TODO: evm.lt23 */) {
          // block_8:
        }
        // block_9:
        // TODO: recover remaining body
    }

    function owner(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function renounceOwnership(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        private__0x7bf_0x7bf(795);
        slot_5 = slot_5 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5, arg3); // TODO: recover event signature
    }

    function symbol(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        if (!false /* TODO: evm.branch.cond3.not.not */) {
          // goto block_6
        }
        // block_5:
        // goto block_10
        // block_6:
        while (false /* TODO: evm.lt23 */) {
          // block_8:
        }
        // block_9:
        // block_10:
        // TODO: recover remaining body
    }

    function totalSupply(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return slot_2;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 96);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_8:
        if (!false /* TODO: evm.branch.cond5.not */) {
          // block_9:
          if (false /* TODO: evm.lt */) {
            // block_10:
            revert(); // custom_error_candidate, selector=0xfb8f41b2
          }
          // block_11:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_12:
            revert(); // custom_error_candidate, selector=0xe602df05, args=1
          }
          // block_13:
          if (uint256(uint160(msg.sender)) == 0) {
            // block_14:
            revert(); // custom_error_candidate, selector=0x94280d62, args=1
          }
          // block_15:
        }
        // block_16:
        private__0x6e1_0x6e1(uint256(uint160(arg0)), uint256(uint160(arg1)), arg2, 31545);
        return 1;
    }

    function transferOwnership(address arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        private__0x7bf_0x7bf(201);
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == 0) {
          // block_7:
          revert(); // custom_error_candidate, selector=0x1e4fbdf7, args=1
        }
        else {
          // block_8:
          slot_5 = slot_5 & uint256(79228162514264337593543950335) << uint256(160) | (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0));
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))); // TODO: recover event signature
        }
    }

    function transfer(address arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        private__0x6e1_0x6e1(uint256(uint160(msg.sender)), uint256(uint160(arg0)), arg1, 31472);
        return 1;
    }

    function private__0x6e1_0x6e1(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_1:
          revert(); // custom_error_candidate, selector=0x96c6fd1e, args=1
        }
        else {
          // block_2:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // custom_error_candidate, selector=0xec442f05, args=1
          }
          else {
            // block_4:
            if (slot_0[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
              // block_5:
              revert(); // custom_error_candidate, selector=0xe450d38c, args=3
            }
            else {
              // block_6:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            }
          }
        }
    }

    function private__0x7bf_0x7bf(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_2:
        }
        else {
          // block_1:
          revert(); // custom_error_candidate, selector=0x118cdaa7, args=1
        }
    }
}
