contract Decompiled {
    event Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xBC7CD75A20EE27FD9ADEBAB32041F755214DBC6BFFA90CC0225B39DA2E5C2D3B(uint256 indexed arg0);
    event Event_0xBB0618986A335F0057C69C62D057D27E28AA5A6FF177F65A32C3649EAC39D763();
    event Event_0xC7F505B2F371AE2175EE4913F4499E1F2633A7B5936321EED1CDAEB6115181D2();

    uint256 public slot_0;
    uint256[] public slot_1;
    uint256 public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;

    function public_0x48b0b3f3() public {
        // block_0:
        uint256 temp_0;
        if ((uint256(uint160(msg.sender)) ^ slot_1.length / uint256(256) ** uint256(0)) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_1:
          temp_0 = private__0x376f_0x376f(slot_5, 1, 3437);
          if (false /* TODO: evm.gt.not */) {
            // block_9:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            if (temp_0 > slot_2) {
              // block_8:
              revert(); // error_string, selector=0x8c379a0
            }
            // block_3:
            slot_5 = temp_0;
            if (!false /* TODO: evm.eq26 */) {
              // block_4:
            }
            // block_5:
            if (!false /* TODO: evm.branch.cond37.not */) {
              // goto block_6
            }
            // goto block_7
            // block_7:
            revert(); // error_string, selector=0x8c379a0
          }
        }
        else {
          // block_10:
          revert(); // error_string, selector=0x8c379a0
        }
        // block_6:
        emit Event_0xBB0618986A335F0057C69C62D057D27E28AA5A6FF177F65A32C3649EAC39D763(); // TODO: recover event signature
        // TODO: recover remaining body
    }

    function public_0x6e85637c() public {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x376f_0x376f(slot_5, 1, 4311);
        if (temp_0 > slot_2) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          slot_5 = temp_0;
          emit Event_0xBB0618986A335F0057C69C62D057D27E28AA5A6FF177F65A32C3649EAC39D763(); // TODO: recover event signature
        }
    }

    function public_0x9d348b98() public {
        // block_0:
        if (false /* TODO: evm.gt.not */) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
        }
    }

    function public_0xad3cb1cc() public {
        // block_0:
    }

    function public_0xbd172ece() public {
        // block_0:
    }

    function approve(address arg0, uint256 arg1) public {
        // block_0:
        private__0x8f7_0x8f7(arg1, uint256(uint160(arg0)), 724);
    }

    function balanceOf(address arg0) public {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1702_0x1702(4514);
        if (uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x89c62b64
        }
        else {
          // block_1:
        }
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function getApproved(uint256 arg0) public {
        // block_0:
    }

    function initialize(address arg0, address arg1) public {
        // block_0:
    }

    function isApprovedForAll(address arg0, address arg1) public {
        // block_0:
    }

    function issueLimit() public {
        // block_0:
    }

    function minter() public {
        // block_0:
    }

    function name() public {
        // block_0:
    }

    function ownerOf(uint256 arg0) public {
        // block_0:
    }

    function owner() public {
        // block_0:
    }

    function priceLimit() public {
        // block_0:
    }

    function proxiableUUID() public {
        // block_0:
        private__0x1d55_0x1d55(4164);
    }

    function recipient() public {
        // block_0:
    }

    function renounceOwnership() public {
        // block_0:
        private__0x1260_0x1260(1257);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        private__0xa0f_0xa0f(arg2, uint256(uint160(arg1)), uint256(uint160(arg0)), 806);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2, bytes calldata arg3) public {
        // block_0:
    }

    function setApprovalForAll(address arg0, bool arg1) public {
        // block_0:
    }

    function setBaseURI(string calldata arg0) public {
        // block_0:
    }

    function supportsInterface(bytes4 arg0) public {
        // block_0:
    }

    function symbol() public {
        // block_0:
    }

    function tokenURI(uint256 arg0) public {
        // block_0:
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
    }

    function transferOwnership(address arg0) public {
        // block_0:
        private__0x1539_0x1539(uint256(uint160(arg0)), 1799);
    }

    function updateMinter(address arg0) public {
        // block_0:
        if ((slot_1.length / uint256(256) ** uint256(0) ^ uint256(uint160(arg0))) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
        }
    }

    function updateRecipient(address arg0) public {
        // block_0:
        if ((slot_0 / uint256(256) ** uint256(0) ^ uint256(uint160(arg0))) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          slot_0 = (uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)) * uint256(256) ** uint256(0) | slot_0 & ~(uint256(256) ** uint256(0) * uint256(1461501637330902918203684832716283019655932542975));
        }
    }

    function upgradeToAndCall(address arg0, bytes calldata arg1) public {
        // block_0:
    }

    function private__0x13a4_0x13a4(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0x17fd_0x17fd(5039);
        temp_1 = private__0x1702_0x1702(7909);
        if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x5b08ba18
        }
        else {
          // block_1:
          emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(temp_0 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        }
    }

    function private__0x1702_0x1702(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return (type(uint256).max - 57565344758593361083622655074698908060916614153676412076385051584998898429183);
    }

    function private__0x172a_0x172a(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2370_0x2370(arg0, 5942);
        if (temp_0 == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x7e273289
        }
        else {
          // block_1:
          return temp_0;
        }
    }

    function private__0x17b2_0x17b2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1702_0x1702(6077);
        return slot_4 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x101b_0x101b(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x1c2b_0x1c2b(arg1, 4140);
    }

    function private__0x106d_0x106d(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0x1088_0x1088(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x172a_0x172a(arg0, 4243);
    }

    function private__0x1260_0x1260(uint256 arg0) internal {
        // block_0:
    }

    function private__0x13f3_0x13f3(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
    }

    function private__0x148b_0x148b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1702_0x1702(5270);
        return slot_5 / uint256(256) ** uint256(0) & uint256(255);
    }

    function private__0x1539_0x1539(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (arg0 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x1e4fbdf7
        }
        else {
          // block_1:
        }
    }

    function private__0x17fd_0x17fd(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return uint256(uint160(msg.sender));
    }

    function private__0x1805_0x1805(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x1a44_0x1a44(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return (type(uint256).max - 6888066478505441749850992418581994297142195019068002000074442631546382030335);
    }

    function private__0x1a6c_0x1a6c(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x2693_0x2693(arg0, arg1, 6782);
    }

    function private__0x1a82_0x1a82(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x26d0_0x26d0(arg0, 6803);
    }

    function private__0x1a96_0x1a96(uint256 arg0) internal {
        // block_0:
    }

    function private__0x1aa0_0x1aa0(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x2756_0x2756(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x1c2b_0x1c2b(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0x1d55_0x1d55(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(address(this))) & uint256(1461501637330902918203684832716283019655932542975) == 985840296161217147145085221403707081373090926276) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // custom_error_candidate, selector=0xe07c8dba, args=0
        }
    }

    function private__0x1eb3_0x1eb3(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return (type(uint256).max - 50618728597856113392845064616540981989246464065957701405731832765476386532607);
    }

    function private__0x2e15_0x2e15(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(26959946667150639794667015087019630673637144422540572481103610249215) == 0);
        // block_1:
    }

    function private__0x2e6e_0x2e6e(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 != 0 ? 1 : 0;
    }

    function private__0x2e89_0x2e89(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x2eaf_0x2eaf(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x2eea_0x2eea(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(31) & uint256((type(uint256).max - 31));
    }

    function private__0x2370_0x2370(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1702_0x1702(9083);
        return slot_2 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x2693_0x2693(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1702_0x1702(9893);
    }

    function private__0x26d0_0x26d0(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (arg0 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x1e4fbdf7
        }
        else {
          // block_1:
        }
    }

    function private__0x3629_0x3629(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal returns (uint256 ret0) {
        // block_0:
        private__0x2f88_0x2f88(arg3, arg0, 13886);
        private__0x34b0_0x34b0(arg2, arg0 + uint256(32), 13899);
        private__0x2f88_0x2f88(arg1, arg0 + uint256(64), 13912);
        return arg0 + uint256(96);
    }

    function private__0x367e_0x367e(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x3688_0x3688(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x367e_0x367e(arg0, 13982) & uint256(18446744073709551615);
    }

    function private__0x36b9_0x36b9(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x2b69_0x2b69(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x2f76_0x2f76(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x2f88_0x2f88(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x2f97_0x2f97(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x2f88_0x2f88(arg1, arg0, 12204);
        return arg0 + uint256(32);
    }

    function private__0x2fb2_0x2fb2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x2fbc_0x2fbc(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x2fb2_0x2fb2(arg0, 12229));
        // block_1:
    }

    function private__0x3015_0x3015(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x2f76_0x2f76(arg0, 12318));
        // block_1:
    }

    function private__0x32b7_0x32b7(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x3392_0x3392(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x33ab_0x33ab(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x34b0_0x34b0(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x34bf_0x34bf(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x34b0_0x34b0(arg1, arg0, 13524);
        return arg0 + uint256(32);
    }

    function private__0x3720_0x3720(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2eaf_0x2eaf(arg0 + uint256(32), 26, 14090);
        return temp_0 + uint256(32);
    }

    function private__0x376f_0x376f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0x2fb2_0x2fb2(arg0, 14202);
        temp_1 = private__0x2fb2_0x2fb2(arg1, 14213);
        return temp_0 + temp_1;
    }

    function private__0x37ef_0x37ef(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2eaf_0x2eaf(arg0 + uint256(32), 32, 14297);
        return temp_0 + uint256(32);
    }

    function private__0x3881_0x3881(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2eaf_0x2eaf(arg0 + uint256(32), 33, 14443);
        return temp_0 + uint256(64);
    }

    function private__0x38a1_0x38a1(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x38ac_0x38ac(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0x38d2_0x38d2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x38a1_0x38a1(arg0, 0, 14524);
        private__0x38ac_0x38ac(temp_0, 14535);
        return temp_0;
    }

    function private__0x3933_0x3933(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2eaf_0x2eaf(arg0 + uint256(32), 25, 14621);
        return temp_0 + uint256(32);
    }

    function private__0x3953_0x3953(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x3964_0x3964(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x3953_0x3953(arg2, arg1, 14704);
        private__0x32b7_0x32b7(arg0, temp_0, arg1, 14717);
        return temp_0 + private__0x2eea_0x2eea(arg1, 14726);
    }

    function private__0x3991_0x3991(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6, uint256 arg7) internal returns (uint256 ret0) {
        // block_0:
        private__0x2f88_0x2f88(arg4, arg0 + uint256(32), 14779);
        private__0x34b0_0x34b0(arg3, arg0 + uint256(64), 14792);
        private__0x2f88_0x2f88(arg2, arg0 + uint256(96), 14805);
        private__0x34b0_0x34b0(arg1, arg0 + uint256(128), 14818);
        return private__0x3964_0x3964(arg6, arg5, arg0 + uint256(160), 14764);
    }

    function private__0x3a5f_0x3a5f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2eaf_0x2eaf(arg0 + uint256(32), 33, 14921);
        return temp_0 + uint256(64);
    }

    function private__0x3a94_0x3a94(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 + uint256(31)) / uint256(32);
    }

    function private__0x3aa4_0x3aa4(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 << arg0;
    }

    function private__0x3b03_0x3b03(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2fb2_0x2fb2(private__0x367e_0x367e(private__0x2fb2_0x2fb2(arg0, 15124), 15129), 15134);
    }

    function private__0x3b59_0x3b59(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x3b94_0x3b94(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (!(arg1 < 32)) {
          // block_2:
        }
        // block_1:
    }

    function private__0x3cfd_0x3cfd(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2fb2_0x2fb2(private__0x367e_0x367e(arg0, 15635), 15640);
    }

    function private__0x3d2e_0x3d2e(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6, uint256 arg7) internal returns (uint256 ret0) {
        // block_0:
        private__0x2f88_0x2f88(arg4, arg0 + uint256(32), 15704);
        private__0x34b0_0x34b0(arg3, arg0 + uint256(64), 15717);
        private__0x2f88_0x2f88(arg2, arg0 + uint256(96), 15730);
        return private__0x3964_0x3964(arg6, arg5, arg0 + uint256(160), 15689);
    }

    function private__0x3dfc_0x3dfc(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2eaf_0x2eaf(arg0 + uint256(32), 53, 15846);
        return temp_0 + uint256(64);
    }

    function private__0x3eee_0x3eee(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2eaf_0x2eaf(arg0 + uint256(32), 36, 16088);
        return temp_0 + uint256(64);
    }

    function private__0x3f0e_0x3f0e(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x3392_0x3392(arg0, 16151));
        // block_1:
    }

    function private__0x4068_0x4068(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0x2f88_0x2f88(arg2, arg0, 16509);
        private__0x34b0_0x34b0(arg1, arg0 + uint256(32), 16522);
        return arg0 + uint256(64);
    }

    function private__0x732_0x732(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == (type(uint256).max - 57591534043048631271602713168962198724648561309353517944097662808962438791167) || arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == 41326688264978518566617017154793646184809722027297161851759941631668658372608 || arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == 904250603428552709895185118199468575982109441609966099573332780532423983104 ? 1 : 0;
    }

    function private__0x8db_0x8db(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        private__0x172a_0x172a(arg0, 2278);
        return private__0x17b2_0x17b2(arg0, 2288);
    }

    function private__0x8f7_0x8f7(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x1805_0x1805(private__0x17fd_0x17fd(2308), arg0, arg1, 2313);
    }

    function private__0xa0f_0xa0f(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }
}
