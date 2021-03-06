#ifndef PROTOMESH_NETWORK_HPP
#define PROTOMESH_NETWORK_HPP

#include <algorithm>
#include <utility>
#include <vector>
#include <tuple>
#include <list>
#include <ierp/RouteCache.hpp>

using namespace std;

#include "iarp/RoutingTable.hpp"
#include "iarp/Advertisement.hpp"
#include "ierp/RouteDiscovery.hpp"
#include "ierp/RouteCache.hpp"
#include "Message.hpp"
#include "CredentialsStore.hpp"

#include "flatbuffers/flatbuffers.h"
#include "communication/message_generated.h"
#include "communication/deliveryFailure_generated.h"
#include "communication/iarp/advertisement_generated.h"
#include "communication/ierp/routeDiscovery_generated.h"
#include "communication/ierp/routeDiscoveryAcknowledgement_generated.h"

#define Datagram vector<uint8_t>
#define DatagramPacket tuple<MessageTarget, Datagram>
#define Datagrams vector<DatagramPacket>

/// Note that the route length is defined in zones so the actual hop count would be MAXIMUM_ROUTE_LENGTH * ZONE_RADIUS
#define MAXIMUM_ROUTE_LENGTH 20
/// Note that the zone radius is inclusive thus including the origin and destination.
/// e.g. A -> x -> y -> B would be a radius of 4
#define ZONE_RADIUS 4

namespace ProtoMesh::communication {

    class MessageTarget {
    public:
        enum class Type {
            SINGLE,
            BROADCAST
        };

        Type type;
        cryptography::UUID target;

        explicit MessageTarget(Type type, cryptography::UUID target = cryptography::UUID::Empty())
                : type(type), target(target) {};

        static MessageTarget broadcast() {
            return MessageTarget(Type::BROADCAST);
        }

        static MessageTarget single(cryptography::UUID target) {
            return MessageTarget(Type::SINGLE, target);
        }
    };



    class Network {
#ifdef UNIT_TESTING
    public:
#endif
        cryptography::UUID deviceID;
        cryptography::asymmetric::KeyPair deviceKeys;
        Routing::IARP::RoutingTable routingTable;
        Routing::IERP::RouteCache routeCache;

        CredentialsStore credentials;

        /// Incoming payloads that are not part of the communication layer
        vector<Datagram> incomingBuffer;
        /// Datagrams waiting to be dispatched (wrapped in a Message)
        vector<DatagramPacket> outgoingQueue;
        /// Payloads waiting for a queue to be available (not wrapped in a Message yet)
        unordered_map<cryptography::UUID, vector<Datagram>> routingQueue;

        enum class MessageSendError {
            TARGET_PUBLIC_KEY_UNKNOWN,
            TARGET_UNREACHABLE
        };

        /// Datagram processing
        Datagrams processAdvertisement(const Datagram &datagram);
        Datagrams processRouteDiscovery(const Datagram &datagram);
        Datagrams processRouteDiscoveryAcknowledgement(const Datagram &datagram);
        Datagrams processDeliveryFailure(const Datagram &datagram);
        Datagrams processMessageDatagram(const Datagram &datagram);

        /// Processing helpers
        Datagrams rebroadcastRouteDiscovery(Routing::IERP::RouteDiscovery routeDiscovery);
        Datagrams dispatchRouteDiscoveryAcknowledgement(Routing::IERP::RouteDiscovery routeDiscovery);

        /// Others
        Datagrams discoverDevice(cryptography::UUID device);
        Result<DatagramPacket, MessageSendError> sendMessageLocalTo(cryptography::UUID target, const Datagram &payload);

    public:

        explicit Network(cryptography::UUID deviceID, cryptography::asymmetric::KeyPair deviceKeys, REL_TIME_PROV_T timeProvider)
                : deviceID(deviceID), deviceKeys(deviceKeys), routingTable(std::move(timeProvider), ZONE_RADIUS) {};

        cryptography::asymmetric::KeyPair getKeys() { return this->deviceKeys; }

        Datagrams processDatagram(const Datagram &datagram);

        /// Note that the payload parameter may not be wrapped in a message.
        void queueMessageTo(cryptography::UUID target, const Datagram &payload);
    };

}


#endif //PROTOMESH_NETWORK_HPP
