contract Decompiled {
    event Event_0xFA7B8EAB7DA67F412CC9575ED43464468F9BFBAE89D1675917346CA6D8FE3C92(address indexed arg0);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDBF36A107DA19E49527A7176A1BABF963B4B0FF8CDE35EE35D6CD8F1F9AC7E1D(address indexed arg0);

    function public_0x45fca874() public {
        // block_0:
        require(msg.data.length > 163);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_3:
        require(false /* TODO: evm.eq.i5.i */);
        // block_5:
        require(false /* TODO: evm.eq.i */);
        // block_6:
        require(false /* TODO: evm.gt.i */);
        // block_7:
        require(false /* TODO: evm.slt.i.i */);
        // block_8:
        require(false /* TODO: evm.gt.i.i */);
        // block_9:
        require(!false /* TODO: evm.gt7.i.i.not */);
        // block_13:
        // block_14:
        revert(); // empty
        if (false /* TODO: evm.eq.i16.i */) {
          // goto block_18
        }
        else {
          // goto block_14
        }
        while (false /* TODO: evm.lt.i.i.not */) {
          // block_20:
          if (!false /* TODO: evm.eq.i.i9 */) {
            // goto block_22
          }
          continue;
        }
        // block_22:
        revert(); // empty
    }

    function public_0x64f2c861() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_3:
        require(false /* TODO: evm.eq.i5.i */);
        // block_5:
        require(false /* TODO: evm.eq.i */);
        // block_8:
        return 0 /* TODO: unresolved value: private__0x45a_0x45a */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0xa5b341b3() public {
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
        require(!false /* TODO: evm.gt7.i.i.not */);
        // block_10:
        require(false /* TODO: evm.gt11.i */);
        // block_11:
        require(false /* TODO: evm.slt.i5.i */);
        // block_12:
        require(false /* TODO: evm.gt.i10.i */);
        // block_13:
        // block_14:
        revert(); // empty
        if (false /* TODO: evm.gt7.i20.i.not */) {
          // goto block_14
        }
        else {
          // goto block_20
        }
        while (false /* TODO: evm.lt.i.i.not */) {
          // block_22:
          if (!false /* TODO: evm.eq.i.i8 */) {
            // goto block_24
          }
          continue;
        }
        // block_24:
        revert(); // empty
    }

    function public_0xafc66222() public {
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
        require(!false /* TODO: evm.gt7.i.i.not */);
        // block_10:
        require(false /* TODO: evm.gt11.i */);
        // block_11:
        require(false /* TODO: evm.slt.i5.i */);
        // block_12:
        require(false /* TODO: evm.gt.i10.i */);
        // block_13:
        // block_14:
        revert(); // empty
        if (false /* TODO: evm.gt7.i20.i.not */) {
          // goto block_14
        }
        else {
          // goto block_20
        }
        while (false /* TODO: evm.lt.i.i.i.not */) {
          // block_22:
          if (!false /* TODO: evm.eq.i.i.i */) {
            // goto block_24
          }
          continue;
        }
        // block_24:
        revert(); // empty
        while (false /* TODO: evm.lt.i.i.not */) {
          // block_27:
          if (!false /* TODO: evm.eq.i.i8 */) {
            // goto block_29
          }
          continue;
        }
        // block_29:
        revert(); // empty
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function notdec_solidity_selector_inline_body() public {
        // block_0:
    }

    function notdec_solidity_selector_inline_body_1(uint256 arg0) public {
        // block_0:
        if (false /* TODO: evm.branch.cond70.not */) {
          // block_2:
          revert(); // returndata_forward
        }
        else {
          // block_1:
        }
    }

    function cancelOwnershipHandover() public {
        // block_0:
        emit Event_0xFA7B8EAB7DA67F412CC9575ED43464468F9BFBAE89D1675917346CA6D8FE3C92(msg.sender); // TODO: recover event signature
    }

    function completeOwnershipHandover(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function ownershipHandoverExpiresAt(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function renounceOwnership() public {
        // block_0:
        private__0x541_0x541(33257);
    }

    function requestOwnershipHandover() public {
        // block_0:
        emit Event_0xDBF36A107DA19E49527A7176A1BABF963B4B0FF8CDE35EE35D6CD8F1F9AC7E1D(msg.sender); // TODO: recover event signature
    }

    function setDelegate(address arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        private__0x642_0x642(uint256(uint160(arg0)), 33389);
    }

    function withdraw(address arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
    }

    function private__0x541_0x541(uint256 arg0) internal {
        // block_0:
    }

    function private__0x642_0x642(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (arg0 << uint256(96) == 0) {
          // block_2:
        }
        else {
          // block_1:
        }
    }
}
