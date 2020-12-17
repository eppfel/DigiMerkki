#include "painlessmesh/plugin.hpp"

#define INVITATION_PKG 31
#define EXCHANGE_PKG 32
#define ABORT_PKG 33

using namespace painlessmesh;

/* Package to invite node in the mesh to start an exchange */
class InvitationPackage : public plugin::BroadcastPackage
{
public:
    // Each package has to be identified by a unique ID
    // Values <30 are reserved for painlessMesh default messages, so using 31 for
    // this package
    InvitationPackage() : plugin::BroadcastPackage(INVITATION_PKG) {}

    InvitationPackage(uint32_t fromID) : plugin::BroadcastPackage(INVITATION_PKG) {
        from = fromID;
    }

    // Convert json object into a InvitationPackage
    InvitationPackage(JsonObject jsonObj) : plugin::BroadcastPackage(jsonObj) {}

    // Convert InvitationPackage to json object
    JsonObject addTo(JsonObject &&jsonObj) const
    {
        jsonObj = plugin::BroadcastPackage::addTo(std::move(jsonObj));
        return jsonObj;
    }

    // Memory to reserve for converting this object to json
    size_t jsonObjectSize() const
    {
        return JSON_OBJECT_SIZE(noJsonFields);
    }
};

/* Package that handles the exchange of a picture */
class AbortPackage : public plugin::SinglePackage
{
public:
    // Each package has to be identified by a unique ID
    // Values <30 are reserved for painlessMesh default messages, so using 32 for
    // this package
    AbortPackage() : plugin::SinglePackage(ABORT_PKG) {}

    AbortPackage(uint32_t fromID, uint32_t destID) : plugin::SinglePackage(ABORT_PKG)
    {
        from = fromID;
        dest = destID;
    }

    // Convert json object into a AbortPackage
    AbortPackage(JsonObject jsonObj) : plugin::SinglePackage(jsonObj) {}

    // Convert AbortPackage to json object
    JsonObject addTo(JsonObject &&jsonObj) const
    {
        jsonObj = plugin::SinglePackage::addTo(std::move(jsonObj));
        return jsonObj;
    }

    // Memory to reserve for converting this object to json
    size_t jsonObjectSize() const
    {
        return JSON_OBJECT_SIZE(noJsonFields);
    }
};

/* Package that handles the exchange of a picture */
class ExchangePackage : public plugin::SinglePackage
{
public:
    static const size_t PROGRESS_START = 0;
    static const size_t PROGRESS_COMPLETE = 1;
    size_t progress;
    size_t picture;
    uint32_t starttime;

    // Each package has to be identified by a unique ID
    // Values <30 are reserved for painlessMesh default messages, so using 32 for
    // this package
    ExchangePackage() : plugin::SinglePackage(EXCHANGE_PKG) {}

    ExchangePackage(uint32_t fromID, uint32_t destID, size_t pictureID) : plugin::SinglePackage(EXCHANGE_PKG)
    {
        from = fromID;
        dest = destID;
        picture = pictureID;
    }

    // Convert json object into a ExchangePackage
    ExchangePackage(JsonObject jsonObj) : plugin::SinglePackage(jsonObj)
    {
        picture = jsonObj["picture"].as<size_t>();
        progress = jsonObj["progress"].as<size_t>();
        starttime = jsonObj["starttime"].as<uint32_t>();
    }

    // Convert ExchangePackage to json object
    JsonObject addTo(JsonObject &&jsonObj) const
    {
        jsonObj = plugin::SinglePackage::addTo(std::move(jsonObj));
        jsonObj["picture"] = picture;
        jsonObj["progress"] = progress;
        jsonObj["starttime"] = starttime;
        return jsonObj;
    }

    // Memory to reserve for converting this object to json
    size_t jsonObjectSize() const
    {
        return JSON_OBJECT_SIZE(noJsonFields + 3);
    }
};
