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
    * When network is disconnected during connection attempt, the operation is immediately aborted. Tested in UnitTestConnect::test10.
    * Suspending keep alive ping for session expiry period is tested in UnitTestConnect::test11.
    * Suspending subscribe operation for session expiry period is tested in UnitTestSubscribe::test5.
    * Suspending unsubscribe operation for session expiry period is tested in UnitTestUnsubscribe::test5.
    * Suspending publish operation for session expiry period is tested in UnitTestPublish::test14 and UnitTestPublish::test15.
    * Suspending message reception for session expiry period is tested in UnitTestReceive::test6 and UnitTestReceive::test7.
- [MQTT-3.1.2-24]: The Server MUST NOT send packets exceeding Maximum Packet Size to the Client
    * Spec: If a Client receives a packet whose size exceeds this limit, this is a Protocol Error, the Client uses
    DISCONNECT with Reason Code 0x95 (Packet too large)
    * Tested in UnitTestReceive::test8.
- [MQTT-3.1.2-25]: Where a Packet is too large to send, the Server MUST discard it without sending it and then behave as if
    it had completed sending that Application Message.
    * Server specific.
- [MQTT-3.1.2-26]: The Server MUST NOT send a Topic Alias in a PUBLISH packet to the Client greater than Topic Alias Maximum.
    * Server specific
- [MQTT-3.1.2-27]: If Topic Alias Maximum is absent or zero, the Server MUST NOT send any Topic Aliases
    to the Client 
    * Server specific
- [MQTT-3.1.2-28]:  A value of 0 (for "Request Response Information" in CONNECT) indicates that the Server MUST NOT return Response Information.
    * Server specific
- [MQTT-3.1.2-29]: If the value of Request Problem Information is 0, the Server MAY return a Reason String or User
    Properties on a CONNACK or DISCONNECT packet, but MUST NOT send a Reason String or User
    Properties on any packet other than PUBLISH, CONNACK, or DISCONNECT 
    * Spec: If the value is 0 (of Request Problem Information in CONNECT) and the Client receives a Reason String or 
    User Properties in a packet other than PUBLISH, CONNACK, or DISCONNECT, it uses a DISCONNECT packet with Reason Code 0x82 (Protocol Error)
    * Rejecting "Reason String" in SUBACK message is tested in UnitTestSubscribe::test7.
    * Rejecting "User Properties" in SUBACK message is tested in UnitTestSubscribe::test8.
    * Rejecting "Reason String" in UNSUBACK message is tested in UnitTestUnsubscribe::test7.
    * Rejecting "User Properties" in UNSUBACK message is tested in UnitTestUnsubscribe::test8.
    * Rejecting "Reason String" in PUBACK message is tested in UnitTestPublish::test16.
    * Rejecting "User Properties" in PUBACK message is tested in UnitTestPublish::test17.    
    * Rejecting "Reason String" in PUBREC message is tested in UnitTestPublish::test18.
    * Rejecting "User Properties" in PUBREC message is tested in UnitTestPublish::test19.        
    * Rejecting "Reason String" in PUBREL message is tested in UnitTestReceive::test9.
    * Rejecting "User Properties" in PUBREL message is tested in UnitTestReceive::test10.        
    * Rejecting "Reason String" in PUBCOMP message is tested in UnitTestPublish::test20.
    * Rejecting "User Properties" in PUBCOMP message is tested in UnitTestPublish::test21.        
    * Rejecting "Reason String" in AUTH message is tested in UnitTestConnect::test12.
    * Rejecting "User Properties" in AUTH message is tested in UnitTestConnect::test13.            
- [MQTT-3.1.2-30]: If a Client sets an Authentication Method in the CONNECT, the Client MUST NOT send any packets other
    than AUTH or DISCONNECT packets until it has received a CONNACK packet.
    * Inability to send any other packet is tested in UnitTestConnect::test6.
    * Sending AUTH packet is tested in UnitTestConnect::test3.
- [MQTT-3.1.3-1]: The Payload of the CONNECT packet contains one or more length-prefixed fields, whose presence is
    determined by the flags in the Variable Header. These fields, if present, MUST appear in the order Client
    Identifier, Will Properties, Will Topic, Will Payload, User Name, Password
    * Part of protocol definition.
    * Presence of fields is tested in UnitTestConnect::test2.
- [MQTT-3.1.3-2]: The ClientID MUST be used by Clients and by Servers to identify state that they hold
    relating to this MQTT Session between the Client and the Server
    * The selection of the client ID is up to the using application.
- [MQTT-3.1.3-3]: The ClientID MUST be present and is the first field in the CONNECT packet Payload
    * Part of the protocol definition.
- [MQTT-3.1.3-4]: The ClientID MUST be a UTF-8 Encoded String
    * The client ID configuration is passed as a zero terminated string, UTF-8 encoding is the responsibility of the application.
- [MQTT-3.1.3-5]: The Server MUST allow ClientID’s which are between 1 and 23 UTF-8 encoded bytes in length, and that
    contain only the characters "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    * Spec: The Server MAY allow ClientID’s that contain more than 23 encoded bytes. The Server MAY allow
        ClientID’s that contain characters not included in the list given above.
    * Server specific, no client side limitation.
- [MQTT-3.1.3-6]: A Server MAY allow a Client to supply a ClientID that has a length of zero bytes, however if it does so the
    Server MUST treat this as a special case and assign a unique ClientID to that Client
    * Server specific.
    * Client ID allocated by the server is reported in the connect response callback.
- [MQTT-3.1.3-7]: It MUST then process the CONNECT packet as if the Client had provided that unique ClientID, and MUST
    return the Assigned Client Identifier in the CONNACK packet
    * Server specific.
    * Reporting allocated client id is tested in UnitTestConnect::test3.
- [MQTT-3.1.3-8]: If the Server rejects the ClientID it MAY respond to the CONNECT packet with a CONNACK using
    Reason Code 0x85 (Client Identifier not valid) as described in section 4.13 Handling errors, and then it
    MUST close the Network Connection.
    * Client side tested in UnitTestConnect::test14.
- [MQTT-3.1.3-9]: If a new Network Connection to this Session is made before the
    Will Delay Interval has passed, the Server MUST NOT send the Will Message
    * Server specific.
- [MQTT-3.1.3-11]:  The Will Topic MUST be a UTF-8 Encoded String
    * UTF-8 encoding is responsibility of the application.
    * Will topic configuration is accepted via zero terminated C-string.
- [MQTT-3.1.3-12]: If the User Name Flag is set to 1, the User Name is the next field in the Payload. The User Name MUST
    be a UTF-8 Encoded String.
    * UTF-8 encoding is responsibility of the application.
    * The user name configuration is accepted via zero terminated C-string.
- [MQTT-3.1.4-1]: The Server MUST validate that the CONNECT packet matches the format described in section
    3.1 and close the Network Connection if it does not match.
    * Spec: The Server MAY send a CONNACK with a Reason Code of 0x80 or greater as described in section 4.13 before closing
        the Network Connection.
    * Server specific.
    * Client library doesn't allow sending malformed CONNECT packet.
    * Reception of DISCONNECT instead of CONNACK is tested in UnitTestConnect::test15.
- [MQTT-3.1.4-2]: The Server MAY check that the contents of the CONNECT packet meet any further restrictions and
    SHOULD perform authentication and authorization checks. If any of these checks fail, it MUST
    close the Network Connection.
    * Spec: Before closing the Network Connection, it MAY send an appropriate CONNACK response with a Reason Code of 0x80 or greater.
    * Network disconnection during incomplete connect (without CONNACK) is tested in UnitTestConnect::test10.
    * Network disconnection after rejecting CONNACK is tested in UnitTestConnect::test16.
- [MQTT-3.1.4-3]: If the ClientID represents a Client already connected to the Server, the Server sends a
    DISCONNECT packet to the existing Client with Reason Code of 0x8E (Session taken over) as
    described in section 4.13 and MUST close the Network Connection of the existing Client
    * Client reporting disconnection is tested in 
- [MQTT-3.1.4-4]: The Server MUST perform the processing of Clean Start that is described in section 3.1.2.4
    * Server specific
- [MQTT-3.1.4-5]: The Server MUST acknowledge the CONNECT packet with a CONNACK packet containing a
    0x00 (Success) Reason Code
    * Server specific.
- [MQTT-3.1.4-6]:  If the Server rejects the CONNECT, it MUST NOT process any data sent by the Client after the CONNECT packet except AUTH
    packets
    * Spec: Clients are allowed to send further MQTT Control Packets immediately after sending a CONNECT
        packet; Clients need not wait for a CONNACK packet to arrive from the Server.
    * The client library prevents sending any packet until connection is finalized (CONNACK is received).
- [MQTT-3.2.0-1]: The Server MUST send a CONNACK with a 0x00 (Success) Reason Code before sending any
    Packet other than AUTH.
    * Server specific.
    * ??? Client behaviour is not determined.
- [MQTT-3.2.0-2]:  The Server MUST NOT send more than one CONNACK in a Network Connection.
    * Server specific.
    * Client ignores unexpected CONNACK.
- [MQTT-3.2.2-1]: Byte 1 (of the CONNACK) is the "Connect Acknowledge Flags". Bits 7-1 are reserved and MUST be set to 0.
    * Part of the protocol definition.
- [MQTT-3.2.2-2]: If the Server accepts a connection with Clean Start set to 1, the Server MUST set Session Present to 0 in
    the CONNACK packet in addition to setting a 0x00 (Success) Reason Code in the CONNACK packet
    * Server specific.
- [MQTT-3.2.2-3]: If the Server accepts a connection with Clean Start set to 0 and the Server has Session State for the
    ClientID, it MUST set Session Present to 1 in the CONNACK packet, otherwise it MUST set Session
    Present to 0 in the CONNACK packet. In both cases it MUST set a 0x00 (Success) Reason Code in the
    CONNACK packet.
    * Server specific
-  [MQTT-3.2.2-4]: If the Client does not have Session State and receives Session Present set to 1 it MUST close
    the Network Connection.
    * Client API returns protocol error for the connection operation.
    * It's up to the application to close connection.
    * Tested in UnitTestConnect::test18.
- [MQTT-3.2.2-5]: If the Client does have Session State and receives Session Present set to 0 it MUST discard its
    Session State if it continues with the Network Connection
    * Clearing previous subscriptions is tested in UnitTestConnect::test19.
    * Client library terminates all pending Qos1 and Qos2 publishes on explicit disconnection request.
- [MQTT-3.2.2-6]: If a Server sends a CONNACK packet containing a non-zero Reason Code it MUST set Session Present
    to 0
    * Server specific
    * Wrong set of the session present bit doesn't influence client library operation.
- [MQTT-3.2.2-7]:  If a Server sends a CONNACK packet containing a Reason code of 128 or greater it MUST then close the Network Connection.
    * Server specific
    * Tested in UnitTestConnect::test20
- [MQTT-3.2.2-8]: The Server sending the CONNACK packet MUST use one of the Connect Reason Code values.
    * Server specific.
    * Client doesn't filter the reported reason code, just reports it as-is to the application.
- [MQTT-3.2.2-9]: If a Server does not support QoS 1 or QoS 2 PUBLISH packets it MUST send a Maximum QoS in the
    CONNACK packet specifying the highest QoS it supports
    * Tested in UnitTestConnect::test2 and UnitTestConnect::test21.
- [MQTT-3.2.2-10]: A Server that does not support QoS 1 or QoS 2 PUBLISH packets MUST still accept SUBSCRIBE packets containing a Requested QoS
    of 0, 1 or 2.
    * Server specific.
- [MQTT-3.2.2-11]: If a Client receives a Maximum QoS from a Server, it MUST NOT send PUBLISH packets at a QoS level
    exceeding the Maximum QoS level specified
    * Client rejects attempt to send message with QoS too high.
    * Tested in UnitTestPublish::test22.
- [MQTT-3.2.2-12]: If a Server receives a CONNECT packet containing a Will QoS that exceeds its capabilities, it MUST
    reject the connection. It SHOULD use a CONNACK packet with Reason Code 0x9B (QoS not supported)
    as described in section 4.13 Handling errors, and MUST close the Network Connection
    * Server specific
    * Client side is not aware of Will QoS limit during connection, connection request will be rejected.
    * Tested in UnitTestConnect::test22.
- [MQTT-3.2.2-13]: If a Server receives a CONNECT packet containing a Will Message with the Will Retain set to 1, and it
    does not support retained messages, the Server MUST reject the connection request. It SHOULD send
    CONNACK with Reason Code 0x9A (Retain not supported) and then it MUST close the Network
    Connection
    * Server specific
    * Tested in UnitTestConnect::test23.
- [MQTT-3.2.2-14]: A Client receiving Retain Available set to 0 from the Server MUST NOT send a PUBLISH packet with the
    RETAIN flag set to 1.
    * Client rejecting publish configuration with retain set when unavailable.
    * Tested in UnitTestPublish::test23.
- [MQTT-3.2.2-15]: The Client MUST NOT send packets exceeding Maximum Packet Size to the Server.
    * Client will reject "send" request with "Bad Parameter" error code.
    * Tested in UnitTestPublish::test24.
- [MQTT-3.2.2-16]: If the Client connects using a zero length Client Identifier, the Server MUST respond with a CONNACK
    containing an Assigned Client Identifier. The Assigned Client Identifier MUST be a new Client Identifier
    not used by any other Session currently in the Server.
    * Server specific.
    * Client doesn't store the client id, just passes it to the application.
    * Client doesn't respond to the incorrect behaviour of the broker (which doesn't allocate new client id).
    * Client requires "clean start" flags set for empty client id. Tested in UnitTestConnect::test24.
- [MQTT-3.2.2-17]:  The Client MUST NOT send a Topic Alias in a PUBLISH packet to the Server greater than this (Topic Alias Maximum) value.
    * Client exposes API to allocate topic aliases.
    * Topic aliases allocation is allowed only after connect
    * Topic alias allocation is rejected once the broker's limit is reached.
    * Tested in UnitTestConnect::test25.
- [MQTT-3.2.2-18]: If Topic Alias Maximum is absent or 0, the Client MUST NOT send any Topic Aliases on to the Server.
    * 0 limit works the same way as with [MQTT-3.2.2-17]
    * Tested in UnitTestConnect::test26.
- [MQTT-3.2.2-19]: The Server MUST NOT send this property (Reason String) if it would increase the size of the CONNACK packet 
    beyond the Maximum Packet Size specified by the Client.
    * Server specific.
- [MQTT-3.2.2-20]: The Server MUST NOT send this property (User Property) if it would increase the size of the CONNACK packet 
    beyond the Maximum Packet Size specified by the Client.
    * Server specific.
- TODO: implment and test wildcard subscription disabled check.

