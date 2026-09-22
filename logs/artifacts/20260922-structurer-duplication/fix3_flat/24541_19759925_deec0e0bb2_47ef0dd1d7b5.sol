contract Decompiled {
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xC7F505B2F371AE2175EE4913F4499E1F2633A7B5936321EED1CDAEB6115181D2();
    event Event_0x9ABF3B8EB9CCBEE081375E7709BF8FB8358CF56760995D73DA9A26B9CE285B68();
    event Event_0x1A2300999B4959E261A233E8ED4AB5FC3BDB62FCCAD4B8F623F995DFDA127C90();
    event Event_0xBC7CD75A20EE27FD9ADEBAB32041F755214DBC6BFFA90CC0225B39DA2E5C2D3B(uint256 indexed arg0);

    function public_0x82c3b965(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_4:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(arg1 < 2);
        // block_8:
        emit Event_0x9ABF3B8EB9CCBEE081375E7709BF8FB8358CF56760995D73DA9A26B9CE285B68(); // TODO: recover event signature
    }

    function public_0xad3cb1cc(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        while (false /* TODO: evm.lt10 */) {
          // block_6:
        }
        // block_7:
    }

    function public_0xe210cc1b(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        return 0 /* TODO: unresolved value: evm_sload */ & uint256(255) != 0 ? 1 : 0;
    }

    function initialize(address arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        if (false /* TODO: evm.eq */) {
          // block_7:
        }
        // block_8:
        if (false /* TODO: spec.select */) {
          // goto block_9
        }
        // goto block_11
        // block_9:
        if (false /* TODO: _0x1b0_0x1.in.reload */) {
          revert(); // custom_error_candidate, selector=0xf92ee8a9
        }
        // block_11:
        if (false /* TODO: evm.iszero */) {
          // block_14:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_12:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          emit Event_0xC7F505B2F371AE2175EE4913F4499E1F2633A7B5936321EED1CDAEB6115181D2(); // TODO: recover event signature
        }
        // block_13:
        // TODO: recover remaining body
    }

    function owner(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return 0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function proxiableUUID(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        if (uint256(uint160(address(this))) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1272660150542499859911417848986211539901598186637)) {
          // block_6:
          return 24440054405305269366569402256811496959409073762505157381672968839269610695612;
        }
        else {
          // block_5:
          revert(); // custom_error_candidate, selector=0xe07c8dba
        }
    }

    function renounceOwnership(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg3); // TODO: recover event signature
    }

    function transferOwnership(address arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_7:
          revert(); // custom_error_candidate, selector=0x1e4fbdf7, args=1
        }
        else {
          // block_8:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
    }

    function upgradeToAndCall(address arg0, bytes calldata arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_2:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        require(!false /* TODO: evm.gt */);
        // block_6:
        require(false /* TODO: evm.slt5.not */);
        // block_8:
        require(!false /* TODO: evm.gt16 */);
        // block_10:
        if (uint256(uint160(address(this))) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(1272660150542499859911417848986211539901598186637)) {
          // block_12:
          revert(); // custom_error_candidate, selector=0xe07c8dba
        }
        // block_11:
        if (!false /* TODO: evm.eq27.not */) {
          // goto block_12
        }
        // goto block_13
        // block_13:
        // block_14:
        if (false /* TODO: evm.gt37 */) {
          // block_15:
        }
        // block_16:
        // block_18:
        if (false) {
          // goto block_29
        }
        // goto block_19
        // block_19:
        // block_21:
        // block_23:
        emit Event_0xBC7CD75A20EE27FD9ADEBAB32041F755214DBC6BFFA90CC0225B39DA2E5C2D3B(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_27:
        // block_26:
        // block_29:
        revert(); // custom_error_candidate, selector=0x4c9c8ce3, args=1
    }

    function withdraw(address arg0, uint256 arg1, address arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 96);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(uint256(uint160(arg2)) == uint256(uint160(arg2)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max));
        // block_8:
        if (false /* TODO: evm.iszero */) {
          revert(); // error_string, selector=0x8c379a0, string_length=49
        }
        // block_10:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_18:
          if (false /* TODO: evm.iszero52 */) {
            // block_20:
            revert(); // error_string, selector=0x8c379a0, string_length=11
          }
          // block_19:
          // block_21:
          emit Event_0x1A2300999B4959E261A233E8ED4AB5FC3BDB62FCCAD4B8F623F995DFDA127C90(); // TODO: recover event signature
        }
        // block_11:
        if (false /* TODO: evm.iszero28.not */) {
          // goto block_21
        }
        // goto block_12
        // block_12:
        // block_14:
        require(false /* TODO: evm.branch.cond45.not */);
        // block_16:
        if (!false /* TODO: evm.iszero40.not */) {
          // goto block_21
        }
        // goto block_17
        // block_17:
        revert(); // custom_error_candidate, selector=0x5274afe7, args=1
        // block_13:
        revert(); // empty
    }

    function private__0xa28_0xa28(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0xa60_0xa60(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 + uint256(31) & uint256(36893488147419103200)) + uint256(32);
    }
}
