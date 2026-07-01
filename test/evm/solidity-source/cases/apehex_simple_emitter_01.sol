// SPDX-License-Identifier: UNLICENSED

pragma solidity ^0.8.12;

contract SimpleEmitter {
    constructor() {}

    event TestEvent(address indexed caller);

    function emitTestEvent() external {
        emit TestEvent(msg.sender);
    }
}
