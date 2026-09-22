contract Decompiled {
    event Event_0xBC7CD75A20EE27FD9ADEBAB32041F755214DBC6BFFA90CC0225B39DA2E5C2D3B(uint256 indexed arg0);
    event Event_0x5A3E66EFAA1E445EBD894728A69D6959842EA1E97BD79B892797106E270EFCD9();

    uint256 public slot_0;
    uint256 public slot_1;

    function fallback() public {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1e8_0x1e8(140);
        require(temp_0 & uint256(2) ** uint256(160) + uint256(type(uint256).max) != 0);
        // block_1:
        if (false /* TODO: evm.iszero4 */) {
          // block_2:
          revert(); // returndata_forward
        }
        else {
          // block_3:
        }
    }

    function implementation() public returns (uint256 ret0) {
        // block_0:
        return private__0x1e8_0x1e8(9865) & uint256(2) ** uint256(160) + uint256(type(uint256).max);
    }

    function proxyOwner() public returns (uint256 ret0) {
        // block_0:
        return private__0x1f7_0x1f7(9740) & uint256(2) ** uint256(160) + uint256(type(uint256).max);
    }

    function proxyType() public returns (uint256 ret0) {
        // block_0:
        return 2;
    }

    function transferProxyOwnership(address arg0) public {
        // block_0:
        private__0x301_0x301(uint256(uint160(arg0)) & uint256(2) ** uint256(160) + uint256(type(uint256).max), 9983);
    }

    function upgradeToAndCall(address arg0, bytes calldata arg1) public {
        // block_0:
    }

    function upgradeTo(address arg0) public {
        // block_0:
        private__0x206_0x206(uint256(uint160(arg0)) & uint256(2) ** uint256(160) + uint256(type(uint256).max), 9799);
    }

    function upgradeabilityOwner() public returns (uint256 ret0) {
        // block_0:
        return private__0x2f2_0x2f2(9924) & uint256(2) ** uint256(160) + uint256(type(uint256).max);
    }

    function private__0x301_0x301(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(uint256(2) ** uint256(160) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == uint256(2) ** uint256(160) + uint256(type(uint256).max) & private__0x1f7_0x1f7(777));
        // block_1:
        require(arg0 & uint256(2) ** uint256(160) + uint256(type(uint256).max) != 0);
        // block_2:
        emit Event_0x5A3E66EFAA1E445EBD894728A69D6959842EA1E97BD79B892797106E270EFCD9(); // TODO: recover event signature
    }

    function private__0x1e8_0x1e8(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return uint256(2) ** uint256(160) + uint256(type(uint256).max) & slot_0;
    }

    function private__0x1f7_0x1f7(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2f2_0x2f2(513);
    }

    function private__0x206_0x206(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(uint256(2) ** uint256(160) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == uint256(2) ** uint256(160) + uint256(type(uint256).max) & private__0x1f7_0x1f7(526));
        // block_1:
        require((slot_0 ^ arg0) & uint256(2) ** uint256(160) + uint256(type(uint256).max) != 0);
        // block_2:
        emit Event_0xBC7CD75A20EE27FD9ADEBAB32041F755214DBC6BFFA90CC0225B39DA2E5C2D3B(arg0 & uint256(2) ** uint256(160) + uint256(type(uint256).max)); // TODO: recover event signature
    }

    function private__0x2f2_0x2f2(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return uint256(2) ** uint256(160) + uint256(type(uint256).max) & slot_1;
    }
}
