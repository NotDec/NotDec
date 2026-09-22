contract Decompiled {
    event Event_0x2F658B440C35314F52658EA8A740E05B284CDC84DC9AE01E891F21B8933E7CAD();
    event Event_0xBC7CD75A20EE27FD9ADEBAB32041F755214DBC6BFFA90CC0225B39DA2E5C2D3B(uint256 indexed arg0);

    function public_0x205943be() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x1ea_0x1ea */;
    }

    function public_0x31b4a693() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x3ea_0x3ea */;
    }

    function public_0x3e68b094() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1242514114293498355016058918240726101543223323250);
    }

    function public_0x8fb36037() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x64e_0x64e */ & uint256(0) - (uint256(1) << uint256(224));
    }

    function public_0xad3cb1cc() public {
        // block_0:
    }

    function authority() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x684_0x684 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function proxiableUUID() public returns (uint256 ret0) {
        // block_0:
        private__0x970_0x970(1461);
        return 24440054405305269366569402256811496959409073762505157381672968839269610695612;
    }

    function setAuthority(address arg0) public {
        // block_0:
    }

    function upgradeToAndCall(address arg0, bytes calldata arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(false /* TODO: evm.gt */);
        // block_2:
        require(false /* TODO: evm.slt7 */);
        // block_3:
        require(!false /* TODO: evm.gt28.not */);
        // block_4:
    }

    function private__0x79d_0x79d(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
    }

    function private__0x58d_0x58d(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x8a3_0x8a3(arg1, 1438);
    }

    function private__0x8a3_0x8a3(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0x970_0x970(uint256 arg0) internal {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(802583580350737526696323585927781080810584509339) == uint256(uint160(address(this)))) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // custom_error_candidate, selector=0xe07c8dba, args=0
        }
    }

    function private__0xfa6_0xfa6(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6, uint256 arg7, uint256 arg8) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(224);
    }
}
