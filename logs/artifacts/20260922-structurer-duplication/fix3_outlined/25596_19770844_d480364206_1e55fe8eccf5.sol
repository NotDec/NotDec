contract Decompiled {
    event Event_0xFA7B8EAB7DA67F412CC9575ED43464468F9BFBAE89D1675917346CA6D8FE3C92(address indexed arg0);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDBF36A107DA19E49527A7176A1BABF963B4B0FF8CDE35EE35D6CD8F1F9AC7E1D(address indexed arg0);

    function public_0x45fca874() public {
        // block_0:
    }

    function public_0x64f2c861() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x45a_0x45a */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0xa5b341b3() public {
        // block_0:
    }

    function public_0xafc66222() public {
        // block_0:
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
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function ownershipHandoverExpiresAt(address arg0) public returns (uint256 ret0) {
        // block_0:
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
    }

    function transferOwnership(address arg0) public {
        // block_0:
        private__0x642_0x642(uint256(uint160(arg0)), 33389);
    }

    function withdraw(address arg0, address arg1) public {
        // block_0:
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
