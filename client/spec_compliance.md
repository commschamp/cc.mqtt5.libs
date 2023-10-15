- [MQTT-1.5.4-1]: The character data in a UTF-8 Encoded String MUST be well-formed UTF-8 as defined by the Unicode
    specification and restated in RFC 3629. In particular, the character data MUST NOT
    include encodings of code points between U+D800 and U+DFFF.
    * UTF-8 compliance responsibility is moved to the caller side.
- [MQTT-1.5.4-2]: A UTF-8 Encoded String MUST NOT include an encoding of the null character U+0000.
    * The null character is used for string terminations in the API.
- [MQTT-1.5.4-3]: A UTF-8 encoded sequence 0xEF 0xBB 0xBF is always interpreted as U+FEFF ("ZERO WIDTH NO-
    BREAK SPACE") wherever it appears in a string and MUST NOT be skipped over or stripped off by a
    packet receiver.
    * UTF-8 compliance responsibility is moved to the caller side.
- [MQTT-1.5.5-1]: The encoded value (of variable byte integer) MUST use the minimum number of
    bytes necessary to represent the value.
    * Implemented by the COMMS library and the protocol definition.
- [MQTT-1.5.7-1] Both strings (of UTF-8 string pair) MUST comply with the requirements for UTF-8 Encoded Strings.
    * UTF-8 compliance responsibility is moved to the caller side.
-  [MQTT-2.1.3-1]: Where a flag bit is marked as “Reserved”, it is reserved for future use and MUST be set to the value listed.
    * Implemented by the protocol messages definition schema.
- [MQTT-2.2.1-2]: A PUBLISH packet MUST NOT contain a Packet Identifier if its QoS value is set to 0.
    * Implemented by the protocol messages definition schema.
- [MQTT-2.2.1-3]: Each time a Client sends a new SUBSCRIBE, UNSUBSCRIBE,or PUBLISH (where QoS > 0) MQTT
    Control Packet it MUST assign it a non-zero Packet Identifier that is currently unused.
    * Implemented as round-robin allocation for the whole std::uint16_t range, skipping the 0, and 
      keeping track of currently allocating numbers to avoid allocation of the packet ID in use.
    * Not really tested (yet).
- [MQTT-2.2.1-4]: Each time a Server sends a new PUBLISH (with QoS > 0) MQTT Control Packet it MUST assign it a non
    zero Packet Identifier that is currently unused.
    * Server specific, client rejects such Qos2 messages (without dup flag) with "Packet ID in use" reason code in PUBREC message.
    * Tested in UnitTestReceive::test4.
- [MQTT-2.2.1-5]: A PUBACK, PUBREC , PUBREL, or PUBCOMP packet MUST contain the same Packet Identifier as the
    PUBLISH packet that was originally sent     
    * Correct behaviour is tested throughout all the unit / integration testing.
    * Ignoring reception of invalid PUBACK is tested in UnitTestPublish::test11.
    * On reception of invalid PUBREC, sending PUBREL with "Packet Identifier not found" reason code, tested in UnitTestPublish::test12
    * On reception of invalid PUBREL, sending PUBCOMP with "Packet Identifier not found" reason code, tested in UnitTestReceive::test5
    * Ignoring reception of invalid PUBCOMP is tested in UnitTestPublish::test13.
- [MQTT-2.2.1-6]: A SUBACK and UNSUBACK MUST contain the Packet Identifier that was used in the corresponding SUBSCRIBE and UNSUBSCRIBE packet  
    respectively.
    * Ignoring reception of invalid SUBACK is tested in UnitTestSubscribe::test4.
    * Ignoring reception of invalid UNSUBACK is tested in UnitTestUnsubscribe::test4.
- [MQTT-2.2.2-1]:  If there are no properties, this MUST be indicated by including a Property Length of zero.
    * Part of protocol definition.
- [MQTT-3.1.0-1]: After a Network Connection is established by a Client to a Server, the first packet sent from the Client to
    the Server MUST be a CONNECT packet.
    * Implemented as inability to "prepare" any operation (other than "connect") that can send messages to the broker. Tested in
      UnitTestConnect::test6.
- [MQTT-3.1.0-2]: The Server MUST process a second CONNECT packet sent from a Client as a Protocol Error and close the Network
    Connection
    * The client library monitors connection status and doesn't allow sending the connection request a second time.
    * Tested in UnitTestConnect::test7.
- [MQTT-3.1.2-1]: The protocol name MUST be the UTF-8 String "MQTT". If the Server does not want to accept the
    CONNECT, and wishes to reveal that it is an MQTT Server it MAY send a CONNACK packet with
    Reason Code of 0x84 (Unsupported Protocol Version), and then it MUST close the Network Connection.
    * Part of protocol definition, client library doesn't change the string.
    * Tested in UnitTestConnect::test1.
- [MQTT-3.1.2-2]: If the Protocol Version is not 5 and the Server does not want
    to accept the CONNECT packet, the Server MAY send a CONNACK packet with Reason Code 0x84
    (Unsupported Protocol Version) and then MUST close the Network Connection
    * The version "5" is hard-coded as part of protocol definition, client library doesn't change the value.
    * Tested in UnitTestConnect::test1.
- [MQTT-3.1.2-3]: The Server MUST validate that the reserved flag in the CONNECT packet is set to 0.
    * Reserved bits are part of the protocol definition, not changed by the client library.
- [MQTT-3.1.2-4]: If a CONNECT packet is received with Clean Start is set to 1, the Client and Server MUST discard any
    existing Session and start a new Session.
    * Client object allocated by the API call doesn't have any state and requires "Clean Start" bit to be set
      on the first attempt to connect.
    * Tested in UnitTestConnect::test8.
- [MQTT-3.1.2-5]: If a CONNECT packet is received with Clean Start set to 0 and there is a Session associated with the Client
    Identifier, the Server MUST resume communications with the Client based on state from the existing
    Session.
    * Server specific.
- [MQTT-3.1.2-6]: If a CONNECT packet is received with Clean Start set to 0 and there is no Session
    associated with the Client Identifier, the Server MUST create a new Session
    * Server specific.
- [MQTT-3.1.2-7]: If the Will Flag is set to 1 this indicates that a Will Message MUST be stored on the Server and associated
    with the Session
    * Server specific.
- [MQTT-3.1.2-8]: The Will Message MUST be published after the Network
    Connection is subsequently closed and either the Will Delay Interval has elapsed or the Session ends,
    unless the Will Message has been deleted by the Server on receipt of a DISCONNECT packet with
    Reason Code 0x00 (Normal disconnection) or a new Network Connection for the ClientID is opened
    before the Will Delay Interval has elapsed.
    * Server specific.
- [MQTT-3.1.2-9]: If the Will Flag is set to 1, the Will Properties, Will Topic, and Will Payload fields MUST be present in the
    Payload.
    * Part of protocol definition.
    * Tested in UnitTestConnect::test2.
- [MQTT-3.1.2-10]:  The Will Message MUST be removed from the stored Session State in the
    Server once it has been published or the Server has received a DISCONNECT packet with a Reason
    Code of 0x00 (Normal disconnection) from the Client.
    * Server specific.
- [MQTT-3.1.2-11]: If the Will Flag is set to 0, then the Will QoS MUST be set to 0 (0x00).
    * Part of the protocol definition.
    * Tested in UnitTestConnect::test1.
- [MQTT-3.1.2-12]: If the Will Flag is set to 1, the value of Will QoS can be 0 (0x00), 1 (0x01), or 2 (0x02).
    * Client side rejects the configuration attempt with invalid value.
    * Tested in UnitTestConnect::test2.
    * Tested in UnitTestConnect::test9.
- [MQTT-3.1.2-13]: If the Will Flag is set to 0, then Will Retain MUST be set to 0
    * Part of the protocol definition.
    * Tested in UnitTestConnect::test1.
- [MQTT-3.1.2-14]: If the Will Flag is set to 1
    and Will Retain is set to 0, the Server MUST publish the Will Message as a non-retained message.
    * Server specific.
- [MQTT-3.1.2-15]: If the Will Flag is set to 1 and Will Retain is set to 1, the Server MUST publish the Will
    Message as a retained message
    * Server specific.
- [MQTT-3.1.2-16]: If the User Name Flag is set to 0, a User Name MUST NOT be present in the Payload.
    * Part of the protocol definition.
    * Tested in UnitTestConnect::test1.
- [MQTT-3.1.2-17]: If the User Name Flag is set to 1, a User Name MUST be present in the Payload.
    * Part of the protocol definition.
    * Tested in UnitTestConnect::test2.
- [MQTT-3.1.2-18]: If the Password Flag is set to 0, a Password MUST NOT be present in the Payload.
    * Part of the protocol definition.
    * Tested in UnitTestConnect::test1.
- [MQTT-3.1.2-19]: If the Password Flag is set to 1, a Password MUST be present in the Payload.
    * Part of the protocol definition.
    * Tested in UnitTestConnect::test2.
- [MQTT-3.1.2-20]: If Keep Alive is non-zero and in the absence of sending any other MQTT Control Packets, the Client MUST
    send a PINGREQ packet.
    * Tested in UnitTestConnect::test5.
- [MQTT-3.1.2-21]: If the Server returns a Server Keep Alive on the CONNACK packet, the Client MUST use that value
    instead of the value it sent as the Keep Alive.
    * Tested in UnitTestConnect::test2.
- [MQTT-3.1.2-22]: If the Keep Alive value is non-zero and the Server does not receive an MQTT Control Packet from the
    Client within one and a half times the Keep Alive time period, it MUST close the Network Connection to
    the Client as if the network had failed
    * Server specific.
- [MQTT-3.1.2-23]: The Client and Server MUST store the Session State after the Network Connection is closed if the
    Session Expiry Interval is greater than 0
    * Implemented by allowing notification of the network disconnection.
    * When network disconnected during connection attempt, the operation is immediately aborted. Tested in UnitTestConnect::test10.
    * Suspending keep alive ping for session expiry period is tested in UnitTestConnect::test11.
    * Suspending subscribe operation for session expiry period is tested in UnitTestSubscribe::test5.
    * Suspending unsubscribe operation for session expiry period is tested in UnitTestUnsubscribe::test5.
    * Suspending publish operation for session expiry period is tested in UnitTestPublish::test14 and UnitTestPublish::test15.
    * Suspending message reception for session expiry period is !!!! NOT TESTED YET.
- [MQTT-3.1.2-24]: The Server MUST NOT send packets exceeding Maximum Packet Size to the Client
    * Spec: If a Client receives a packet whose size exceeds this limit, this is a Protocol Error, the Client uses
    DISCONNECT with Reason Code 0x95 (Packet too large)
    * !!!! Check is NOT IMPLEMENTED YET.
- [MQTT-3.1.2-25]: Where a Packet is too large to send, the Server MUST discard it without sending it and then behave as if
    it had completed sending that Application Message.
    * Server specific.
