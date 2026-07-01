contract Decompiled {
    event Event_0xAB77F9000C19702A713E62164A239E3764DDE2BA5265C7551F9A49E0D304530D(address indexed arg0);

    function public_0x3fd958b7() public {
        // block_0:
        emit Event_0xAB77F9000C19702A713E62164A239E3764DDE2BA5265C7551F9A49E0D304530D(msg.sender); // TODO: recover event signature
        // TODO: recover remaining body
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }
}
