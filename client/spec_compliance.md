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
    the Server MUST be a CONNECT packet