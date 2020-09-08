//
// Generated file, do not edit! Created by nedtool 5.4 from Modules/messages/HeterogeneousMessage.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include "HeterogeneousMessage_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp


// forward
template<typename T, typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec);

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// operator<< for std::vector<T>
template<typename T, typename A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec)
{
    out.put('{');
    for(typename std::vector<T,A>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) {
            out.put(','); out.put(' ');
        }
        out << *it;
    }
    out.put('}');
    
    char buf[32];
    sprintf(buf, " (size=%u)", (unsigned int)vec.size());
    out.write(buf, strlen(buf));
    return out;
}

Register_Class(HeterogeneousMessage)

HeterogeneousMessage::HeterogeneousMessage(const char *name, short kind) : ::omnetpp::cPacket(name,kind)
{
    this->applId = 0;
    this->nodeId = 0;
    this->networkId = 0;
    this->sendingTime = 0;
}

HeterogeneousMessage::HeterogeneousMessage(const HeterogeneousMessage& other) : ::omnetpp::cPacket(other)
{
    copy(other);
}

HeterogeneousMessage::~HeterogeneousMessage()
{
}

HeterogeneousMessage& HeterogeneousMessage::operator=(const HeterogeneousMessage& other)
{
    if (this==&other) return *this;
    ::omnetpp::cPacket::operator=(other);
    copy(other);
    return *this;
}

void HeterogeneousMessage::copy(const HeterogeneousMessage& other)
{
    this->sourceAddress = other.sourceAddress;
    this->destinationAddress = other.destinationAddress;
    this->appName = other.appName;
    this->applId = other.applId;
    this->nodeId = other.nodeId;
    this->networkId = other.networkId;
    this->sendingTime = other.sendingTime;
}

void HeterogeneousMessage::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cPacket::parsimPack(b);
    doParsimPacking(b,this->sourceAddress);
    doParsimPacking(b,this->destinationAddress);
    doParsimPacking(b,this->appName);
    doParsimPacking(b,this->applId);
    doParsimPacking(b,this->nodeId);
    doParsimPacking(b,this->networkId);
    doParsimPacking(b,this->sendingTime);
}

void HeterogeneousMessage::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->sourceAddress);
    doParsimUnpacking(b,this->destinationAddress);
    doParsimUnpacking(b,this->appName);
    doParsimUnpacking(b,this->applId);
    doParsimUnpacking(b,this->nodeId);
    doParsimUnpacking(b,this->networkId);
    doParsimUnpacking(b,this->sendingTime);
}

const char * HeterogeneousMessage::getSourceAddress() const
{
    return this->sourceAddress.c_str();
}

void HeterogeneousMessage::setSourceAddress(const char * sourceAddress)
{
    this->sourceAddress = sourceAddress;
}

const char * HeterogeneousMessage::getDestinationAddress() const
{
    return this->destinationAddress.c_str();
}

void HeterogeneousMessage::setDestinationAddress(const char * destinationAddress)
{
    this->destinationAddress = destinationAddress;
}

const char * HeterogeneousMessage::getAppName() const
{
    return this->appName.c_str();
}

void HeterogeneousMessage::setAppName(const char * appName)
{
    this->appName = appName;
}

int HeterogeneousMessage::getApplId() const
{
    return this->applId;
}

void HeterogeneousMessage::setApplId(int applId)
{
    this->applId = applId;
}

int HeterogeneousMessage::getNodeId() const
{
    return this->nodeId;
}

void HeterogeneousMessage::setNodeId(int nodeId)
{
    this->nodeId = nodeId;
}

int HeterogeneousMessage::getNetworkId() const
{
    return this->networkId;
}

void HeterogeneousMessage::setNetworkId(int networkId)
{
    this->networkId = networkId;
}

::omnetpp::simtime_t HeterogeneousMessage::getSendingTime() const
{
    return this->sendingTime;
}

void HeterogeneousMessage::setSendingTime(::omnetpp::simtime_t sendingTime)
{
    this->sendingTime = sendingTime;
}

class HeterogeneousMessageDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
  public:
    HeterogeneousMessageDescriptor();
    virtual ~HeterogeneousMessageDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual bool setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
};

Register_ClassDescriptor(HeterogeneousMessageDescriptor)

HeterogeneousMessageDescriptor::HeterogeneousMessageDescriptor() : omnetpp::cClassDescriptor("HeterogeneousMessage", "omnetpp::cPacket")
{
    propertynames = nullptr;
}

HeterogeneousMessageDescriptor::~HeterogeneousMessageDescriptor()
{
    delete[] propertynames;
}

bool HeterogeneousMessageDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<HeterogeneousMessage *>(obj)!=nullptr;
}

const char **HeterogeneousMessageDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *HeterogeneousMessageDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int HeterogeneousMessageDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 7+basedesc->getFieldCount() : 7;
}

unsigned int HeterogeneousMessageDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
    };
    return (field>=0 && field<7) ? fieldTypeFlags[field] : 0;
}

const char *HeterogeneousMessageDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "sourceAddress",
        "destinationAddress",
        "appName",
        "applId",
        "nodeId",
        "networkId",
        "sendingTime",
    };
    return (field>=0 && field<7) ? fieldNames[field] : nullptr;
}

int HeterogeneousMessageDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0]=='s' && strcmp(fieldName, "sourceAddress")==0) return base+0;
    if (fieldName[0]=='d' && strcmp(fieldName, "destinationAddress")==0) return base+1;
    if (fieldName[0]=='a' && strcmp(fieldName, "appName")==0) return base+2;
    if (fieldName[0]=='a' && strcmp(fieldName, "applId")==0) return base+3;
    if (fieldName[0]=='n' && strcmp(fieldName, "nodeId")==0) return base+4;
    if (fieldName[0]=='n' && strcmp(fieldName, "networkId")==0) return base+5;
    if (fieldName[0]=='s' && strcmp(fieldName, "sendingTime")==0) return base+6;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *HeterogeneousMessageDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "string",
        "string",
        "string",
        "int",
        "int",
        "int",
        "simtime_t",
    };
    return (field>=0 && field<7) ? fieldTypeStrings[field] : nullptr;
}

const char **HeterogeneousMessageDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *HeterogeneousMessageDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int HeterogeneousMessageDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    HeterogeneousMessage *pp = (HeterogeneousMessage *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *HeterogeneousMessageDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    HeterogeneousMessage *pp = (HeterogeneousMessage *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string HeterogeneousMessageDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    HeterogeneousMessage *pp = (HeterogeneousMessage *)object; (void)pp;
    switch (field) {
        case 0: return oppstring2string(pp->getSourceAddress());
        case 1: return oppstring2string(pp->getDestinationAddress());
        case 2: return oppstring2string(pp->getAppName());
        case 3: return long2string(pp->getApplId());
        case 4: return long2string(pp->getNodeId());
        case 5: return long2string(pp->getNetworkId());
        case 6: return simtime2string(pp->getSendingTime());
        default: return "";
    }
}

bool HeterogeneousMessageDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    HeterogeneousMessage *pp = (HeterogeneousMessage *)object; (void)pp;
    switch (field) {
        case 0: pp->setSourceAddress((value)); return true;
        case 1: pp->setDestinationAddress((value)); return true;
        case 2: pp->setAppName((value)); return true;
        case 3: pp->setApplId(string2long(value)); return true;
        case 4: pp->setNodeId(string2long(value)); return true;
        case 5: pp->setNetworkId(string2long(value)); return true;
        case 6: pp->setSendingTime(string2simtime(value)); return true;
        default: return false;
    }
}

const char *HeterogeneousMessageDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

void *HeterogeneousMessageDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    HeterogeneousMessage *pp = (HeterogeneousMessage *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}


